#!/usr/bin/python
# -*- Coding:utf-8 -*-
#
# Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Radek Novacek <rnovacek@redhat.com>
#
# Base class for all tests

import os
import sys
if sys.version_info[0] > 2 or sys.version_info[1] > 6:
    import unittest
else:
    import unittest2 as unittest
import pywbem
import Queue
import socket
import threading
import BaseHTTPServer
import random

def get_interop_namespace():
    """
    Get name of interop namespace.
    """
    cimom = os.environ.get("LMI_CIMOM_BROKER", "tog-pegasus")
    if cimom != "tog-pegasus":
        # We always use interop with sfcbd
        return "root/interop"
    if os.path.isdir("/var/lib/Pegasus/repository/root#PG_InterOp"):
        return "root/PG_InterOp"
    else:
        return "root/interop"

class CIMListener(object):
    """ CIM Listener
    """
    class CIMHandler(BaseHTTPServer.BaseHTTPRequestHandler):
        def do_POST(self):
            data = self.rfile.read(int(self.headers['Content-Length']))
            tt = pywbem.parse_cim(pywbem.xml_to_tupletree(data))
            # Get the instance from CIM-XML, copied from
            # http://sf.net/apps/mediawiki/pywbem/?title=Indications_Tutorial
            insts = [x[1] for x in tt[2][2][0][2][2]]
            for inst in insts:
                self.callback(inst)
            self.send_response(200)
            self.end_headers()

        def log_message(self, format, *p):
            # suppress log messages
            pass

    def __init__(self, callback, http_port=5988):
        self.address = ('', http_port)
        self.CIMHandler.callback = callback
        self.thread = None
        self.server = None

    def start(self):
        BaseHTTPServer.HTTPServer.allow_reuse_address = True
        self.server = BaseHTTPServer.HTTPServer(self.address, self.CIMHandler)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()

    def stop(self):
        if self.server is not None:
            self.server.shutdown()
            self.server.socket.close()
        if self.thread is not None:
            self.thread.join()

    def running(self):
        return self.thread is not None

class TestBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.url = os.environ.get("LMI_CIMOM_URL", "https://localhost:5989")
        cls.username = os.environ.get("LMI_CIMOM_USERNAME", "root")
        cls.password = os.environ.get("LMI_CIMOM_PASSWORD", "")

        cls.bus_name = os.environ.get("LMI_NETWORKING_BUS_NAME", "org.freedesktop.NetworkManager")
        cls.ports = os.environ.get("LMI_NETWORKING_PORTS", "").split(",")
        cls.port = cls.ports[0]
        cls.fake_test = os.environ.get("LMI_NETWORKING_FAKE_NM", "0") == "1"

        if cls.url == "local://pegasus":
            # Special string for local pegasus connection
            cls.wbemconnection = pywbem.PegasusUDSConnection()
        elif cls.url == "local://sfcbd":
            # Special string for local sfcbd connection
            cls.wbemconnection = pywbem.SFCBUDSConnection()
        else:
            cls.wbemconnection = pywbem.WBEMConnection(cls.url, (cls.username, cls.password))

        cls.indication_port = random.randint(12000, 13000)
        cls.indication_queue = Queue.Queue()
        cls.listener = CIMListener(
                callback=cls._process_indication,
                http_port=cls.indication_port)

    @classmethod
    def tearDownClass(cls):
        # stop listening
        cls.listener.stop()

    def setUp(self):
        self.subscribed = {}

    def tearDown(self):
        # put the class to initial state
        # unsubscribe from indications
        if self.subscribed:
            for name in self.subscribed.keys():
                self.unsubscribe(name)

    def subscribe(self, filter_name, query=None, querylang="DMTF:CQL"):
        """
        Create indication subscription for given filter name.
        """
        namespace = get_interop_namespace()
        hostname = socket.gethostname()

        if query is not None:
            # Create filter first
            filterinst = pywbem.CIMInstance('CIM_IndicationFilter')
            filterinst['CreationClassName'] = 'CIM_IndicationFilter'
            filterinst['SystemCreationClassName'] = 'CIM_ComputerSystem'
            filterinst['SystemName'] = hostname
            filterinst['Name'] = filter_name
            filterinst['Query'] = query
            filterinst['QueryLanguage'] = querylang
            filterinst['SourceNamespace'] = "root/cimv2"
            cop = pywbem.CIMInstanceName('CIM_IndicationFilter')
            cop.keybindings = { 'CreationClassName': 'CIM_IndicationFilter',
                'SystemClassName': 'CIM_ComputerSystem',
                'SystemName': hostname,
                'Name': filter_name
            }
            cop.namespace=namespace
            filterinst.path = cop
            indfilter = self.wbemconnection.CreateInstance(filterinst)
        else:
            # the filter is already created, assemble its name
            indfilter = pywbem.CIMInstanceName(
                    classname="CIM_IndicationFilter",
                    namespace=namespace,
                    keybindings={
                        'CreationClassName': 'CIM_IndicationFilter',
                        'SystemClassName': 'CIM_ComputerSystem',
                        'SystemName': hostname,
                        'Name': filter_name})

        # create destination
        destinst = pywbem.CIMInstance('CIM_ListenerDestinationCIMXML')
        destinst['CreationClassName'] = 'CIM_ListenerDestinationCIMXML'
        destinst['SystemCreationClassName'] = 'CIM_ComputerSystem'
        destinst['SystemName'] = hostname
        destinst['Name'] = filter_name
        destinst['Destination'] = "http://localhost:%d" % (self.indication_port)
        destinst['PersistenceType'] = pywbem.Uint16(3) # Transient
        cop = pywbem.CIMInstanceName('CIM_ListenerDestinationCIMXML')
        cop.keybindings = { 'CreationClassName':'CIM_ListenerDestinationCIMXML',
                'SystemClassName':'CIM_ComputerSystem',
                'SystemName':hostname,
                'Name':filter_name }
        cop.namespace = namespace
        destinst.path = cop
        destname = self.wbemconnection.CreateInstance(destinst)

        # create the subscription
        subinst = pywbem.CIMInstance('CIM_IndicationSubscription')
        subinst['Filter'] = indfilter
        subinst['Handler'] = destname
        cop = pywbem.CIMInstanceName('CIM_IndicationSubscription')
        cop.keybindings = { 'Filter': indfilter,
                'Handler': destname }
        cop.namespace = namespace
        subinst.path = cop
        subscription = self.wbemconnection.CreateInstance(subinst)

        self.subscribed[filter_name] = [subscription, destname]

        # start listening
        if not self.listener.running():
            self._start_listening()
        return subscription

    def unsubscribe(self, filter_name):
        """
        Unsubscribe fron given filter.
        """
        _list = self.subscribed.pop(filter_name)
        for instance in _list:
            self.wbemconnection.DeleteInstance(instance)

    def _start_listening(self):
        """ Start listening for incoming indications. """
        self.listener.start()

    @classmethod
    def _process_indication(cls, indication):
        """ Callback to process one indication."""
        cls.indication_queue.put(indication)

    def get_indication(self, timeout):
        """ Wait for an indication for given nr. of seconds and return it."""
        try:
            indication = self.indication_queue.get(timeout=timeout)
        except Queue.Empty:
            raise AssertionError("Timeout when waiting for indicaiton")
        self.indication_queue.task_done()
        return indication

    def wait_for_job(self, job, timeout=10):
        # Job is started, wait for finish
        while 1:
            indication = self.get_indication(timeout)
            if indication["SourceInstance"].classname == "LMI_NetworkJob":
                if indication["SourceInstance"]["InstanceID"] == job["InstanceID"]:
                    return indication["SourceInstance"]

    def wait_for_job_completion(self, job):
        """
        Wait for incoming job changes indications. When we have JobState == 7,
        the job succeeds. Otherwise wait_for_job raises an exception that
        will propagate out of this method
        """
        while 1:
            if self.wait_for_job(job)["JobState"] == 7: # Completed
                return
