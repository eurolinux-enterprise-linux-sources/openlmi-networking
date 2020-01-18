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
# This test tests applying connections in different modes.


import sys
import os
import pywbem
import time
import unittest
import IPy

from test_base import TestBase

class TestApplyConnection(TestBase):
    @classmethod
    def setUpClass(cls):
        TestBase.setUpClass()
        # Get reference on port
        cls.ports = cls.wbemconnection.ExecQuery("WQL", 'select * from LMI_IPNetworkConnection where Name = "%s"' % cls.port)
        if len(cls.ports) < 1:
            raise AssertionError("No LMI_IPNetworkConnection with Name %s" % cls.port)
        cls.portref = cls.ports[0]

        cls.confService = cls.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        capas = cls.wbemconnection.Associators(cls.portref.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        if len(capas) != 1:
            raise AssertionError("No LMI_IPNetworkConnectionCapabilities for port %s" % cls.portref["Name"])
        capa = capas[0]

        # Create a couple of connections
        rc = cls.wbemconnection.InvokeMethod("LMI_CreateIPSetting",
                capa.path, Caption="XXX Connection 1",
                IPv4Type=pywbem.Uint16(4))
        if rc[0] != 0:
            cls.skipTest("Unable to create connection - ApplyConnection test will be skipped")
            return
        cls.con1 = cls.wbemconnection.GetInstance(rc[1]["SettingData"])

        rc = cls.wbemconnection.InvokeMethod("LMI_CreateIPSetting",
                capa.path, Caption="XXX Connection 2",
                IPv4Type=pywbem.Uint16(4))
        if rc[0] != 0:
            cls.skipTest("Unable to create connection - ApplyConnection test will be skipped")
            return
        cls.con2 = cls.wbemconnection.GetInstance(rc[1]["SettingData"])

        rc = cls.wbemconnection.InvokeMethod("LMI_CreateIPSetting",
                capa.path, Caption="XXX Connection 3",
                IPv4Type=pywbem.Uint16(4))
        if rc[0] != 0:
            cls.skipTest("Unable to create connection - ApplyConnection test will be skipped")
            return
        cls.con3 = cls.wbemconnection.GetInstance(rc[1]["SettingData"])

    @classmethod
    def tearDownClass(cls):
        TestBase.tearDownClass()
        cls.wbemconnection.DeleteInstance(cls.con1.path)
        cls.wbemconnection.DeleteInstance(cls.con2.path)
        cls.wbemconnection.DeleteInstance(cls.con3.path)

    def setUp(self):
        TestBase.setUp(self)
        # Subscribe to indication
        self.filter_name = "test_apply_%d" % (time.time() * 1000)
        self.subscribe(self.filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_NetworkJob")

    def assertIPElementSettingData(self, esd, isCurrent, isNext):
        portName = esd["ManagedElement"]["Name"]
        connectionName = self.wbemconnection.GetInstance(esd["SettingData"])["Caption"]
        if isCurrent is not None:
            self.assertEqual(esd["IsCurrent"], isCurrent,
                    "Value of IsCurrent should be %d (but it's %d) for port %s "
                    "and configuration %s" % (isCurrent, esd["IsCurrent"],
                    portName, connectionName))
        if isNext is not None:
            self.assertEqual(esd["IsNext"], isNext,
                    "Value of IsNext should be %d (but it's %d) for port %s "
                    "and configuration %s" % (isNext, esd["IsNext"],
                    portName, connectionName))

    def test_mode_1(self):
        """ Test activating connection with Mode == 1

        Mode 1: IsNext = 1 (Is Next), IsCurrent = 1 (Is Current)
        """
        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(1))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed

        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 1, 1)
        for esd in self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, 2)

    def test_mode_2(self):
        """ Test activating connection with Mode == 2

        Mode 2: IsNext = 1 (Is Next) and IsCurrent not affected
        """
        esd = self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData")[0]
        old_isCurrent = esd["IsCurrent"]

        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(2))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed

        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, old_isCurrent, 1)
        for esd in self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, None, 2)

    def test_mode_4(self):
        """ Test activating connection with Mode == 4

        Mode 4 - IsNext = 2 (Is Not Next) and IsCurrent = 2 (Is Not Current)
        """
        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(4))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed

        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, 2)

    def test_mode_5(self):
        """ Test activating connection with Mode == 5

        Mode 5 - IsNext = 2 (Is Not Next) and IsCurrent not affected
        """
        esd = self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData")[0]
        old_isCurrent = esd["IsCurrent"]

        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(5))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed


        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, old_isCurrent, 2)
        for esd in self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, 2)

    def test_mode_32768(self):
        """ Test activating connection with Mode == 32768

        Mode 32768 - IsCurrent = 1 (Is Current) and IsNext not affected
        """
        esd = self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData")[0]
        old_isNext = esd["IsNext"]

        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(32768))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed


        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 1, old_isNext)
        for esd in self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, 2)

    def test_mode_32769(self):
        """ Test activating connection with Mode == 32769

        Mode 32769 - ``IsCurrent = 2 (Is Not Current) and IsNext not affected
        """
        esd = self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData")[0]
        old_isNext = esd["IsNext"]

        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                self.confService.path, SettingData=self.con1.path,
                IPNetworkConnection=self.portref.path, Mode=pywbem.Uint16(32769))
        self.assertIn(rc[0], [0, 4096], "Applying setting failed")
        if rc[0] == 4096: # Job started
            job = self.wait_for_job(rc[1]["Job"])
            self.assertEqual(job["JobState"], 7) # Completed

        # Test values of IsNext and IsCurrent for all created connections
        for esd in self.wbemconnection.References(self.con1.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, old_isNext)
        for esd in self.wbemconnection.References(self.con2.path, ResultClass="LMI_IPElementSettingData") + \
                   self.wbemconnection.References(self.con3.path, ResultClass="LMI_IPElementSettingData"):
            self.assertIPElementSettingData(esd, 2, 2)

if __name__ == '__main__':
    import os
    os.environ["LMI_CIMOM_URL"] = "local://pegasus"
    unittest.main()
