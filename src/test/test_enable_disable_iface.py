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

import os
import sys
import unittest
import pywbem
import time

from test_base import TestBase

ENABLED = 2
DISABLED = 3

NM_UNAVAILABLE = 20
NM_DISCONNECTED = 30
NM_CONNECTED = 100

def waitForValues(values, function, *p):
    value = None
    for i in range(10):
        value = function(*p)
        if value in values:
            return value
        time.sleep(1)
    return value

class TestEnableDisableIface(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        ports = self.wbemconnection.ExecQuery("WQL", 'select * from LMI_LANEndpoint where Name="%s"' % self.port)
        self.assertEqual(len(ports), 1, "Port with name %s doesn't exist, or is not managed by NetworkManager" % self.port)
        self.ethport = ports[0]
        # Create default connection, so we're able to activate the port
        self.defaultConnectionCaption = "Test Default %s" % self.port
        self.confService = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        ipnetcons = self.wbemconnection.Associators(self.ethport.path, ResultClass="LMI_IPNetworkConnection")
        self.assertEqual(len(ipnetcons), 1)
        ipnetcon = ipnetcons[0]

        capas = self.wbemconnection.Associators(ipnetcon.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertEqual(len(capas), 1)
        capa = capas[0]
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", capa.path,
                Caption=self.defaultConnectionCaption, IPv4Type=pywbem.Uint16(4)) # DHCP

    def tearDown(self):
        for instance in self.wbemconnection.ExecQuery("WQL", 'select * from LMI_IPAssignmentSettingData where Caption="%s"' % self.defaultConnectionCaption):
            self.wbemconnection.DeleteInstance(instance.path)

    def getState(self, port):
        port = self.wbemconnection.GetInstance(port.path)
        if port['EnabledState'] == 2:
            return NM_CONNECTED
        else:
            return NM_UNAVAILABLE

    def test_enableDisableIface(self):
        new_state = self.getState(self.ethport)
        for i in range(2):
            state = new_state
            if state in [NM_CONNECTED, NM_DISCONNECTED]:
                self.wbemconnection.InvokeMethod("RequestStateChange", self.ethport.path, RequestedState=pywbem.Uint16(DISABLED))
                new_state = waitForValues([NM_UNAVAILABLE], self.getState, self.ethport)
                self.assertEqual(new_state, NM_UNAVAILABLE, "Cannot disable device %s" % self.ethport["Name"])
            elif state == NM_UNAVAILABLE:
                self.wbemconnection.InvokeMethod("RequestStateChange", self.ethport.path, RequestedState=pywbem.Uint16(ENABLED))
                new_state = waitForValues([NM_CONNECTED, NM_DISCONNECTED], self.getState, self.ethport)
                self.assertIn(new_state, [NM_CONNECTED, NM_DISCONNECTED], "Cannot enable device %s" % self.ethport["Name"])
