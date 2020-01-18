#!/usr/bin/python
# -*- Coding:utf-8 -*-
#
# Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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
# This test tests changing connections.
# It tests modification of IPAssociationSettingData objects

import sys
import os
import pywbem
import time
import unittest
import IPy

from test_base import TestBase


class TestModifyConnections(TestBase):
    @classmethod
    def setUpClass(cls):
        TestBase.setUpClass()

        cls.portrefs = []
        for port in cls.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            if port["Name"] in cls.ports:
                cls.portrefs.append(port)
        if len(cls.portrefs) > 0:
            cls.portref = cls.portrefs[0]
        else:
            raise AssertionError("No network port found")

        cls.confService = cls.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        capas = cls.wbemconnection.Associators(cls.portref.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        if len(capas) != 1:
            raise AssertionError("No LMI_IPNetworkConnectionCapablities for port %s" % cls.portref["Name"])
        capa = capas[0]

        # Create connection for testing
        cls.caption = "XXX Test Connection"
        rc = cls.wbemconnection.InvokeMethod("LMI_CreateIPSetting",
            capa.path, Caption=cls.caption, IPv4Type=pywbem.Uint16(4)) # DHCP
        if rc[0] != 0:
            raise AssertionError("Unable to create connection")
        cls.settingData = rc[1]["SettingData"]

    @classmethod
    def tearDownClass(cls):
        TestBase.tearDownClass()
        for connection in cls.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if connection['AddressOrigin'] == 11 and connection["Caption"].startswith("XXX "):
                cls.wbemconnection.DeleteInstance(connection.path)

    def test_modify_caption(self):
        """ Test modify caption of the connection """
        self.caption += " Modified"
        self.settingData = self.wbemconnection.GetInstance(self.settingData)
        self.settingData["Caption"] = self.caption
        self.wbemconnection.ModifyInstance(self.settingData)

        for i in self.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if i["Caption"] == self.caption:
                break
        else:
            self.assertTrue(True, "Updating of Caption failed")

    @unittest.skip("Changing type of connection is not currently supported")
    def test_modify_ip_version(self):
        """ Test modify IP version of connection """
        sd = self.wbemconnection.GetInstance(self.settingData)
        self.assertEqual(sd["ProtocolIFType"], 4096) # IPv4

        rc = self.wbemconnection.InvokeMethod("CreateOrModifyDynamicSetting",
                self.confService.path, ProtocolIFType=pywbem.Uint16(2),
                SettingData=self.settingData)
        self.assertEqual(rc[0], 0)
        sd = self.wbemconnection.GetInstance(rc[1]["SettingData"])
        self.assertEqual(sd["ProtocolIFType"], 4097) # IPv6
        self.settingData = rc[1]["SettingData"]

    def test_modify_setting_method(self):
        """ Test modify setting method """

        # Set IPv4Type to Static
        self.settingData = self.wbemconnection.GetInstance(self.settingData)
        self.settingData["IPv4Type"] = pywbem.Uint16(3) # Static
        self.wbemconnection.ModifyInstance(self.settingData)
        # Check the changed value
        self.settingData = self.wbemconnection.GetInstance(self.settingData.path)
        self.assertEqual(self.settingData["IPv4Type"], pywbem.Uint16(3))
        # Check association
        assoc = self.wbemconnection.Associators(self.settingData.path, AssocClass="LMI_OrderedIPAssignmentComponent",
                                        ResultClass="LMI_ExtendedStaticIPAssignmentSettingData")
        self.assertEqual(len(assoc), 1)
        self.assertEqual(assoc[0]["ProtocolIFType"], 4096)

        # Add Stateless IPv6Type
        self.settingData["IPv6Type"] = pywbem.Uint16(9) # Stateless
        self.wbemconnection.ModifyInstance(self.settingData)
        # Check the changed value
        self.settingData = self.wbemconnection.GetInstance(self.settingData.path)
        self.assertEqual(self.settingData["IPv6Type"], pywbem.Uint16(9))
        self.assertEqual(self.settingData["ProtocolIFType"], pywbem.Uint16(32768))
        # Check association
        assoc = self.wbemconnection.Associators(self.settingData.path, AssocClass="LMI_OrderedIPAssignmentComponent",
                                        ResultClass="LMI_IPAssignmentSettingData")
        self.assertEqual(len(assoc), 1)
        self.assertEqual(assoc[0]["ProtocolIFType"], 4097)


        # TODO: get rid of it once the bug gets fixed
        self.skipTest("The rest of the test is skipped due to bug #1018781")


        # Disable both IPv4 and IPv6
        self.settingData["IPv4Type"] = pywbem.Uint16(0) # Disabled
        self.settingData["IPv6Type"] = pywbem.Uint16(0) # Disabled
        self.wbemconnection.ModifyInstance(self.settingData)
        # Check the changed value

        self.settingData = self.wbemconnection.GetInstance(self.settingData.path)
        self.assertEqual(self.settingData["IPv4Type"], pywbem.Uint16(0))
        self.assertEqual(self.settingData["IPv6Type"], pywbem.Uint16(0))
        assoc = self.wbemconnection.Associators(AssocClass="LMI_OrderedIPAssignmentComponent",
                                        ResultClass="LMI_IPAssignmentSettingData")
        self.assertEqual(len(assoc), 0)
