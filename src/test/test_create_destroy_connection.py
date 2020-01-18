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
# This test creates and destroys multiple connections.
# It tests creating connections using LMI_CreateIPSetting method of
# LMI_IPNetworkConnectionCapabilities

import sys
import os
import pywbem
import time
import unittest
import IPy

import dbus
from dbus.mainloop.glib import DBusGMainLoop
import gobject

from test_base import TestBase

STATIC, DHCP = range(2)

OBJECT_PATH = "/org/freedesktop/NetworkManager/Settings"
IFACE_SETTINGS = "org.freedesktop.NetworkManager.Settings"
IFACE_CONNECTION = "org.freedesktop.NetworkManager.Settings.Connection"

def intToIPv4(ip):
    return ".".join([str(x) for x in (ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256)])

def bytesToIPv6(ip):
    s = ""
    a = False
    for b in ip:
        s += "%.2x" % b
        if a:
            s += ":"
        a = not a
    return s[:-1]

def bytesToMac(mac):
    s = ""
    for b in mac:
        s += "%.2X:" % b
    return s[:-1]

class Connection(object):
    def __init__(self, caption, type_, ipv4=None, ipv6=None, port=None,
            ipv4_dns=None, ipv4_search=None, ipv6_dns=None, ipv6_search=None,
            ipv4_routes=None, ipv6_routes=None):

        # Mark our created connections with XXX for easier deletion
        self.caption = "XXX " + caption
        self.type = type_
        self.ipv4 = ipv4
        self.ipv6 = ipv6
        self.ipv4_dns = ipv4_dns
        self.ipv4_search = ipv4_search
        self.ipv6_dns = ipv6_dns
        self.ipv6_search = ipv6_search
        self.ipv4_routes = ipv4_routes
        self.ipv6_routes = ipv6_routes
        self.port = port

    def modifyInstance(self, instance):
        if instance.classname == "LMI_ExtendedStaticIPAssignmentSettingData":
            if instance["ProtocolIFType"] == 4096:
                ipaddresses = []
                subnetmasks = []
                gateways = []
                for addr in self.ipv4:
                    ipaddresses.append(addr[0])
                    subnetmasks.append(IPy.IP("0.0.0.0/%d" % addr[1]).netmask().strFullsize())
                    if len(addr) > 2:
                        gateways.append(addr[2])
                    else:
                        gateways.append("")
                instance['IPAddresses'] = ipaddresses
                instance['SubnetMasks'] = subnetmasks
                instance['GatewayAddresses'] = gateways
            elif instance["ProtocolIFType"] == 4097:
                ipaddresses = []
                prefixes = []
                gateways = []
                for addr in self.ipv6:
                    ipaddresses.append(addr[0])
                    prefixes.append(pywbem.Uint16(addr[1]))
                    if len(addr) > 2:
                        gateways.append(addr[2])
                    else:
                        gateways.append("")
                instance['IPAddresses'] = ipaddresses
                instance['IPv6SubnetPrefixLengths'] = prefixes
                instance['GatewayAddresses'] = gateways
        elif instance.classname == "LMI_DNSSettingData":
            if instance["ProtocolIFType"] == 4096:
                if self.ipv4_dns is not None:
                    instance["DNSServerAddresses"] = pywbem.CIMProperty(
                            "DNSServerAddresses",
                            self.ipv4_dns,
                            type='string',
                            array_size=len(self.ipv4_dns),
                            is_array=True)

                if self.ipv4_search is not None:
                    instance["DNSSearchDomains"] = pywbem.CIMProperty(
                            "DNSSearchDomains",
                            self.ipv4_search,
                            type='string',
                            array_size=len(self.ipv4_search),
                            is_array=True)
            elif instance["ProtocolIFType"] == 4097:
                if self.ipv6_dns is not None:
                    instance["DNSServerAddresses"] = pywbem.CIMProperty(
                            "DNSServerAddresses",
                            self.ipv6_dns,
                            type='string',
                            array_size=len(self.ipv6_dns),
                            is_array=True)
                if self.ipv6_search is not None:
                    instance["DNSSearchDomains"] = pywbem.CIMProperty(
                            "DNSSearchDomains",
                            self.ipv6_search,
                            type='string',
                            array_size=len(self.ipv6_search),
                            is_array=True)
        return instance

class TestCreatingConnections(TestBase):
    def runTest(self):
        self.test_CreateStaticConnection()
        self.test_CreateDHCPConnection()

    @classmethod
    def setUpClass(cls):
        TestBase.setUpClass()
        gobject.threads_init()
        mainloop = DBusGMainLoop()
        if cls.fake_test:
            cls.bus = dbus.SessionBus(mainloop=mainloop)
        else:
            cls.bus = dbus.SystemBus(mainloop=mainloop)

        cls.settings = cls.bus.get_object(cls.bus_name, OBJECT_PATH)

        cls.connections = []
        cls.ports = cls.wbemconnection.ExecQuery("WQL", 'select * from LMI_IPNetworkConnection where Name = "%s"' % cls.port)
        if len(cls.ports) > 0:
            cls.portref = cls.ports[0]

        # Register NewConnection signal
        cls.settings.connect_to_signal("NewConnection", cls.newConnection, dbus_interface=IFACE_SETTINGS)

        cls.confService = cls.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        cls.macs = {}
        for lan in cls.wbemconnection.EnumerateInstances("LMI_LANEndpoint"):
            cls.macs[lan["Name"]] = lan["MACAddress"]

    @classmethod
    def tearDownClass(cls):
        TestBase.tearDownClass()
        # Give it some time to delete the connections
        time.sleep(3)
        connections = []
        for connection in cls.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if connection["Caption"] is not None and connection["Caption"].startswith("XXX "):
                connections.append(connection["Caption"])
        if len(connections) > 0:
            raise AssertionError("Connections was not deleted: %s" % ", ".join(connections))

    def tearDown(self):
        TestBase.tearDown(self)
        try:
            self.wbemconnection.DeleteInstance(self.con.ref)
        except AttributeError:
            pass

    def setUp(self):
        TestBase.setUp(self)
        self.con = None
        self.assertGreater(len(self.ports), 0, "No network port found")
        # Subscribe to indication
        self.filter_name = "test_apply_%d" % (time.time() * 1000)
        self.subscribe(self.filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_NetworkJob")

    def findDBusConnection(self, caption):
        for path in self.settings.ListConnections():
            settings = self.bus.call_blocking(self.bus_name, path, IFACE_CONNECTION, "GetSettings", "", [])
            c = settings.get("connection", {}).get("id", None)
            if c is None:
                continue
            if c == caption:
                return settings

    def newConnection(self, path):
        print "newConnection", path

    def connectionCreate(self, connection):
        caps = self.wbemconnection.Associators(connection.port.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertEqual(len(caps), 1)
        cap = caps[0]

        if connection.type == STATIC:
            ipv4type = 3 # static
            ipv6type = 3 # static
        else:
            ipv4type = 4 # DHCP
            ipv6type = 9 # Stateless

        if not connection.ipv4:
            ipv4type = 0 # Disabled
        if not connection.ipv6:
            ipv6type = 0 # Disabled

        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", cap.path,
                Caption=connection.caption,
                Type=pywbem.Uint16(1), # Ethernet
                IPv4Type=pywbem.Uint16(ipv4type),
                IPv6Type=pywbem.Uint16(ipv6type))

        self.assertEqual(rc[0], 0, "Creating connection has invalid return code: %d" % rc[0])
        try:
            connection.ref = rc[1]["SettingData"]
        except KeyError:
            self.assertTrue(False, "Method LMI_CreateIPSetting didn't return proper SettingData reference: %s" % rc[1])

        self.assertEqual(connection.ref.classname, "LMI_IPAssignmentSettingData")
        self.assertEqual(self.wbemconnection.GetInstance(connection.ref)["AddressOrigin"], 11) # Cumulative setting

        for assoc in self.wbemconnection.Associators(connection.ref, AssocClass="LMI_OrderedIPAssignmentComponent"):
            if assoc.classname == "LMI_DNSSettingData":
                continue
            if assoc["ProtocolIFType"] == 4096:
                self.assertEqual(assoc["AddressOrigin"], ipv4type)
                if ipv4type == 3:
                    self.assertEqual(assoc.classname, "LMI_ExtendedStaticIPAssignmentSettingData")
                elif ipv4type == 4:
                    self.assertEqual(assoc.classname, "LMI_DHCPSettingData")
            elif assoc["ProtocolIFType"] == 4097:
                self.assertEqual(assoc["AddressOrigin"], ipv6type)
                if ipv4type == 3:
                    self.assertEqual(assoc.classname, "LMI_ExtendedStaticIPAssignmentSettingData")
                elif ipv4type == 6:
                    self.assertEqual(assoc.classname, "LMI_DHCPSettingData")
                elif ipv4type == 9:
                    self.assertEqual(assoc.classname, "LMI_IPAssignmentSettingData")
            else:
                self.assertTrue(True, "Unknown ProtocolIFType: %s" % assoc["ProtocolIFType"])

        # Modify Static connections:
        for assoc in self.wbemconnection.Associators(connection.ref, AssocClass="LMI_OrderedIPAssignmentComponent"):
            if assoc.classname == "LMI_ExtendedStaticIPAssignmentSettingData":
                connection.modifyInstance(assoc)
                self.wbemconnection.ModifyInstance(assoc)
            if assoc.classname == "LMI_DNSSettingData":
                connection.modifyInstance(assoc)
                self.wbemconnection.ModifyInstance(assoc)

        routes = []
        if connection.ipv4_routes is not None:
            for route in connection.ipv4_routes:
                routes.append((pywbem.Uint16(1), # IPv4
                      route[0],
                      IPy.IP("0.0.0.0/%d" % route[1]).netmask().strFullsize(),
                      route[2],
                      pywbem.Uint16(route[3])))
        if connection.ipv6_routes is not None:
            for route in connection.ipv6_routes:
                routes.append((pywbem.Uint16(2), # IPv6
                      route[0],
                      pywbem.Uint8(route[1]),
                      route[2],
                      pywbem.Uint16(route[3])))

        for route in routes:
            args = {
                "AddressType": route[0],
                "DestinationAddress": route[1],
            }
            if route[0] == pywbem.Uint16(1):
                args["DestinationMask"] = route[2]
            else:
                args["PrefixLength"] = route[2]
            rc = self.wbemconnection.InvokeMethod("LMI_AddStaticIPRoute", connection.ref, **args)
            self.assertEqual(rc[0], 0)
            self.assertIn("Route", rc[1])
            routeSD = self.wbemconnection.GetInstance(rc[1]["Route"])
            routeSD["NextHop"] = route[3]
            routeSD["RouteMetric"] = route[4]
            rc = self.wbemconnection.ModifyInstance(routeSD)

    def connectionTest(self, connection):
        dbusConnection = self.findDBusConnection(connection.caption)
        if connection.type == STATIC:
            if connection.ipv4 is not None:
                self.assertIn("ipv4", dbusConnection)
                self.assertIn("method", dbusConnection["ipv4"])
                self.assertEqual(dbusConnection["ipv4"]["method"], "manual")
                self.assertIn("addresses", dbusConnection["ipv4"])
                addresses = dbusConnection["ipv4"]["addresses"]
                self.assertEqual(len(addresses), len(connection.ipv4))
                for i in range(len(addresses)):
                    self.assertEqual(connection.ipv4[i][0], intToIPv4(addresses[i][0]))
                    self.assertEqual(connection.ipv4[i][1], addresses[i][1])
                    if len(connection.ipv4[i]) > 2:
                        self.assertEqual(connection.ipv4[i][2], intToIPv4(addresses[i][2]))
                if connection.ipv4_dns is not None:
                    self.assertIn("dns", dbusConnection["ipv4"])
                    ip4stringlist = [intToIPv4(ip) for ip in dbusConnection["ipv4"]["dns"]]
                    self.assertListEqual(connection.ipv4_dns, ip4stringlist)

            if connection.ipv6 is not None:
                self.assertIn("ipv6", dbusConnection)
                self.assertIn("method", dbusConnection["ipv6"])
                self.assertEqual(dbusConnection["ipv6"]["method"], "manual")
                self.assertIn("addresses", dbusConnection["ipv6"])
                addresses = dbusConnection["ipv6"]["addresses"]
                self.assertEqual(len(addresses), len(connection.ipv6))
                for i in range(len(addresses)):
                    self.assertEqual(connection.ipv6[i][0], bytesToIPv6(addresses[i][0]))
                    self.assertEqual(connection.ipv6[i][1], addresses[i][1])
                    if len(connection.ipv6[i]) > 2:
                        self.assertEqual(connection.ipv6[i][2], bytesToIPv6(addresses[i][2]))
                if connection.ipv6_dns is not None:
                    self.assertIn("dns", dbusConnection["ipv6"])
                    ip6stringlist = [bytesToIPv6(ip) for ip in dbusConnection["ipv6"]["dns"]]
                    self.assertListEqual(connection.ipv6_dns, ip6stringlist)

            # dns-search property is IP-version agnostic
            dnssearch = set()
            if "ipv4" in dbusConnection and "dns-search" in dbusConnection["ipv4"]:
                for s in dbusConnection["ipv4"]["dns-search"]:
                    dnssearch.add(s)
            if "ipv6" in dbusConnection and "dns-search" in dbusConnection["ipv6"]:
                for s in dbusConnection["ipv6"]["dns-search"]:
                    dnssearch.add(s)
            if connection.ipv4_search is not None:
                for search in connection.ipv4_search:
                    self.assertIn(search, dnssearch)
            if connection.ipv6_search is not None:
                for search in connection.ipv6_search:
                    self.assertIn(search, dnssearch)

        else:
            if connection.ipv4:
                self.assertIn("ipv4", dbusConnection)
                self.assertIn("method", dbusConnection["ipv4"])
                self.assertEqual(dbusConnection["ipv4"]["method"], "auto")
            else:
                if "ipv4" in dbusConnection and "method" in dbusConnection["ipv4"]:
                    self.assertEqual(dbusConnection["ipv4"]["method"], "disabled")

            if connection.ipv6:
                self.assertIn("ipv6", dbusConnection)
                self.assertIn("method", dbusConnection["ipv6"])
                self.assertEqual(dbusConnection["ipv6"]["method"], "auto")
            else:
                if "ipv6" in dbusConnection and "method" in dbusConnection["ipv6"]:
                    self.assertEqual(dbusConnection["ipv6"]["method"], "ignore")

        if connection.ipv4_routes is not None:
            self.assertIn("ipv4", dbusConnection)
            self.assertIn("routes", dbusConnection["ipv4"])
            dbusRoutes = []
            for route in dbusConnection["ipv4"]["routes"]:
                dbusRoutes.append((intToIPv4(route[0]), route[1], intToIPv4(route[2]), route[3]))

            for route in connection.ipv4_routes:
                self.assertIn(route, dbusRoutes)

        if connection.ipv6_routes is not None:
            self.assertIn("ipv6", dbusConnection)
            self.assertIn("routes", dbusConnection["ipv6"])
            dbusRoutes = []
            for route in dbusConnection["ipv6"]["routes"]:
                dbusRoutes.append((bytesToIPv6(route[0]), route[1], bytesToIPv6(route[2]), route[3]))

            for route in connection.ipv6_routes:
                self.assertIn(route, dbusRoutes)

        if connection.port is not None:
            self.assertIn("802-3-ethernet", dbusConnection)
            self.assertIn("mac-address", dbusConnection["802-3-ethernet"])
            self.assertEqual(self.macs[connection.port["Name"]], bytesToMac(dbusConnection["802-3-ethernet"]["mac-address"]))

    def connectionDelete(self, connection):
        for i in self.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if i["Caption"] == connection.caption:
                self.wbemconnection.DeleteInstance(i.path)
        # wait for connection deletion
        time.sleep(2)

    def connectionApply(self, connection, mode=32768):
        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection", self.confService.path, SettingData=connection.ref, IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(mode))
        self.assertIn(rc[0], [0, 4096])
        if rc[0] == 4096:
            # Job is started, wait for finish
            indication = self.get_indication(10)
            self.assertEqual(indication["SourceInstance"].classname, "LMI_NetworkJob")
            # Save the error message if the method fails
            errors = []
            if indication["SourceInstance"]["JobState"] != 7:
                for err in self.wbemconnection.InvokeMethod("GetErrors", rc[1]["Job"])[1]['Errors']:
                    errors.append(err["Message"])
            self.assertEqual(indication["SourceInstance"]["JobState"], 7, # Completed
                             "Unable to activate connection: %s" % ("; ".join(errors)))
    ## Static

    def test_static_both(self):
        """ Test creating and activating static connection with IPv4 and IPv6 """
        self.con = Connection("Test Static Both", STATIC,
                ipv4=[("192.168.122.201", 24, "192.168.147.100")],
                ipv6=[("2001:0db8:0000:2219:0210:18ff:fe97:2222", 24, "2001:0db8:0000:2219:0210:18ff:fe97:0001")],
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_static_both_dns(self):
        """ Test creating and activating static connection with IPv4 and IPv6 and dns records """
        # Set some DNS servers
        self.con = Connection("Test Static Both DNS", STATIC,
                ipv4=[("192.168.122.201", 24, "192.168.147.100")],
                ipv4_dns=["192.168.147.100", "192.168.147.101"],
                ipv4_search=["example.com", "test.example.com"],
                ipv6=[("2001:0db8:0000:2219:0210:18ff:fe97:2222", 24, "2001:0db8:0000:2219:0210:18ff:fe97:0001")],
                ipv6_dns=["2001:0db8:0000:2219:0210:18ff:fe97:0001", "2001:0db8:0000:2219:0210:18ff:fe97:0002"],
                ipv6_search=["example2.com", "test.example2.com"],
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_static_both_without_gateway(self):
        """ Test creating and activating static connection with IPv4 and IPv6 without default gateway """
        # Don't set gateway
        self.con = Connection("Test Static Both 2", STATIC,
                ipv4=[("192.168.147.1", 20)],
                ipv6=[("2001:0db8:0000:2219:0210:18ff:fe97:2222", 20)],
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_static_ipv4(self):
        """ Test creating and activating static connection with IPv4 only """
        self.con = Connection("Test Static IPv4", STATIC,
                ipv4=[("192.168.147.1", 24, "192.168.147.100")],
                ipv6=None,
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_static_ipv6(self):
        """ Test creating and activating static connection with IPv6 only """
        self.con = Connection("Test Static IPv6", STATIC,
                ipv4=None,
                ipv6=[("2001:0db8:0000:2219:0210:18ff:fe97:2222", 24, "2001:0db8:0000:2219:0210:18ff:fe97:0001")],
                port=self.portref)
        self.connectionCreate(self.con)
        self.connections.append(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_static_fail(self):
        """ Test creating invalid static connection """
        self.con = Connection("Test Static None", STATIC,
                ipv4=None,
                ipv6=None,
                port=self.portref)
        self.assertRaises(pywbem.CIMError, self.connectionCreate, self.con)

    ## DHCP

    def test_dhcp_both(self):
        """ Test creating and activating DHCP connection with IPv4 and IPv6 """
        self.con = Connection("Test DHCP Both", DHCP,
                ipv4=True,
                ipv6=True,
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_dhcp_ipv4(self):
        """ Test creating and activating DHCP connection with IPv4 only """
        self.con = Connection("Test DHCP IPv4", DHCP,
                ipv4=True,
                ipv6=False,
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)

    def test_dhcp_ipv6(self):
        """ Test creating and activating DHCP connection with IPv6 only """
        self.con = Connection("Test DHCP IPv6", DHCP,
                ipv4=False,
                ipv6=True,
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.skipTest("This test requires DHCPv6 server, skipped")
        # self.connectionApply(self.con)

    def test_dhcp_fail(self):
        """ Test creating invalid DHCP connection """
        self.con = Connection("Test DHCP None", DHCP,
                ipv4=False,
                ipv6=False,
                port=self.portref)
        self.assertRaises(pywbem.CIMError, self.connectionCreate, self.con)

    def test_static_route(self):
        """ Test creating and activating static connection with routes"""
        self.con = Connection("Test Static Route", STATIC,
                ipv4=[("192.168.122.201", 24, "192.168.122.100")],
                ipv6=[("2001:0db8:0000:2219:0210:18ff:fe97:2222", 64, "2001:0db8:0000:2219:0210:18ff:fe97:0001")],
                ipv4_routes=[("192.168.123.0", 24, "192.168.122.101", 10),
                             ("192.168.124.0", 24, "192.168.122.102", 20)],
                ipv6_routes=[("2001:0db8:0000:2220:0000:0000:0000:0000", 64, "2001:0db8:0000:2219:0210:18ff:fe97:0002", 10),
                             ("2001:0db8:0000:2221:0000:0000:0000:0000", 64, "2001:0db8:0000:2219:0210:18ff:fe97:0003", 20)],
                port=self.portref)
        self.connectionCreate(self.con)
        self.connectionTest(self.con)
        self.connectionApply(self.con)
