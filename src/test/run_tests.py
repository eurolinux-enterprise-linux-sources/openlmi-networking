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
# Usage: ./test.py https://localhost:5989 root <password>
#
# Runs all defined tests

import sys
import os
import signal
import time

from optparse import OptionParser

import pexpect
import subprocess

from fake_cimom import CIMOM, IOThread

class LocalDBus(subprocess.Popen):
    def __init__(self):
        print "Starting dbus daemon"
        # Start private DBus daemon
        out = subprocess.check_output(['dbus-launch'], universal_newlines=True)
        variables = {}
        for line in out.splitlines():
            (k, v) = line.split('=', 1)
            variables[k] = v
        self.buspid = int(variables['DBUS_SESSION_BUS_PID'])
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = variables['DBUS_SESSION_BUS_ADDRESS']

        testdir = os.path.dirname(__file__)

        subprocess.Popen.__init__(self, ["/usr/bin/python", os.path.join(testdir, "fake_nm.py")])

    def terminate(self):
        subprocess.Popen.terminate(self)
        try:
            os.environ.pop("DBUS_SESSION_BUS_ADDRESS")
        except KeyError:
            pass
        os.kill(self.buspid, signal.SIGTERM)

if __name__ == '__main__':
    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("-f", "--fake", dest="fake", action="store_true", default=False,
                      help="Use fake NetworkManager API to aviod spoiling targeted system")
    parser.add_option("-p", "--ports", dest="ports", default=None, help="Network ports to use for tests (comma-separated), some tests require to use at least 2 ports")
    parser.add_option("-t", "--test", dest="tests", action="append")
    parser.add_option("-l", "--local", dest="path", help="Use local (not registered) build, with given path to provider library")
    parser.add_option("-d", "--debug", dest="debug", action="store_true", default=False, help="Use gdb to debug build")

    parser.add_option("--hostname", action="store", dest="hostname",
            default="https://localhost:5989",
            help="remote machine hostname")
    parser.add_option("--username", action="store", dest="username",
            default="pegasus",
            help="remote machine username")
    parser.add_option("--password", action="store", dest="password",
            default="pegasus",
            help="remote machine password")

    (options, args) = parser.parse_args()

    if options.fake and options.path is None:
        parser.error("Test can't be run in fake mode without local build")

    if options.path is not None:
        os.environ["LMI_CIMOM_URL"] = "http://localhost:54320"
        os.environ["LMI_CIMOM_USERNAME"] = ""
        os.environ["LMI_CIMOM_PASSWORD"] = ""
    else:
        os.environ["LMI_CIMOM_URL"] = options.hostname
        os.environ["LMI_CIMOM_USERNAME"] = options.username
        os.environ["LMI_CIMOM_PASSWORD"] = options.password

    if options.ports is None:
        if options.fake:
            # always use eth1 for local tests
            options.ports = "eth1,eth2"
        else:
            options.ports = ",".join(subprocess.check_output(["nmcli", "-f", "DEVICE", "-t", "dev"]).splitlines())
            if len(options.ports) < 1:
                sys.exit("No usable network port found (check result of command 'nmcli dev')")

    os.environ["LMI_NETWORKING_PORTS"] = options.ports
    if options.fake:
        os.environ["LMI_NETWORKING_BUS_NAME"] = "org.freedesktop.FakeNetworkManager"
        os.environ["LMI_NETWORKING_FAKE_NM"] = "1"
    else:
        os.environ["LMI_NETWORKING_FAKE_NM"] = "0"

    fake_nm = None
    fake_cimom = None

    if options.fake:
        fake_nm = LocalDBus()

    if options.path is not None:
        fake_cimom = CIMOM(
                os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "mof", "LMI_Networking.mof"),
                os.path.join(options.path, "LMI_Networking.reg"), options.path,
                debug=options.debug)

    if options.tests is None:
        options.tests = []

    print "------ nosetests"
    nose = IOThread("/usr/bin/nosetests", ["-v", "--process-timeout=20"] + options.tests, env=os.environ, cwd=os.path.dirname(__file__), timeout=60)
    nose.start()
    if fake_cimom is not None:
        fake_cimom.run_debugger()

    nose.end_event.wait()
    if nose.is_timeout:
        print "------ nosetests TIMEOUT"
    else:
        print "------ nosetests done"

    if fake_nm is not None:
        fake_nm.terminate()
        fake_nm.wait()
    if fake_cimom is not None:
        fake_cimom.terminate()

    sys.exit(nose.p.exitstatus)
