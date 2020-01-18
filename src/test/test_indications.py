
import sys
from test_base import TestBase, unittest
import pywbem
import time

class TestIndications(TestBase):
    #def setUp(self):
    #    TestBase.setUp(self)

    def test_connection_lifetime_indications(self):
        ipNetworkConnection = None
        for networkConnection in self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            if networkConnection["Name"] == self.port:
                ipNetworkConnection = networkConnection
        self.assertTrue(ipNetworkConnection is not None)
        capas = self.wbemconnection.Associators(ipNetworkConnection.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertGreater(len(capas), 0)
        capa = capas[0]

        # Connection creation indication
        filter_name = "test_create_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_NetworkInstCreation where SourceInstance isa LMI_IPAssignmentSettingData")

        caption = "XXX Test Indication"
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", capa.path,
                Caption=caption, Type=pywbem.Uint16(1),
                IPv4Type=pywbem.Uint16(4))
        self.assertEqual(len(rc), 2)
        self.assertEqual(rc[0], 0)
        settingData = self.wbemconnection.GetInstance(rc[1]["SettingData"])

        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_NetworkInstCreation")
        self.assertIn("SourceInstance", indication.keys())
        self.assertTrue(indication["SourceInstance"] is not None)
        self.assertEqual(indication["SourceInstance"]["Caption"], caption)

        # Connection modification indication
        filter_name = "test_modify_%d" % (time.time() * 1000)
        sub2 = self.subscribe(filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_IPAssignmentSettingData")

        new_caption = caption + " Modified"
        settingData["Caption"] = new_caption
        self.wbemconnection.ModifyInstance(settingData)

        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_NetworkInstModification")
        self.assertIn("SourceInstance", indication.keys())
        source = indication["SourceInstance"]
        self.assertEqual(source["Caption"], new_caption)

        self.assertIn("PreviousInstance", indication.keys())
        previous = indication["PreviousInstance"]
        self.assertEqual(previous["Caption"], caption)

        # Connection deletion indication
        filter_name = "test_delete_%d" % (time.time() * 1000)
        sub3 = self.subscribe(filter_name, "select * from LMI_NetworkInstDeletion where SourceInstance isa LMI_IPAssignmentSettingData")

        self.wbemconnection.DeleteInstance(settingData.path)

        indication = self.get_indication(10)
        self.assertEqual(indication.classname, "LMI_NetworkInstDeletion")
        self.assertIn("SourceInstance", indication.keys())
        source = indication["SourceInstance"]
        self.assertEqual(source["Caption"], new_caption)

    def test_port_lifetime_indications(self):
        # Create SettingData for testing
        ipNetworkConnection = None
        for networkConnection in self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            if networkConnection["Name"] == self.port:
                ipNetworkConnection = networkConnection
        self.assertTrue(ipNetworkConnection is not None)
        capas = self.wbemconnection.Associators(ipNetworkConnection.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertGreater(len(capas), 0)
        capa = capas[0]
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", capa.path,
                Caption="XXX Test Indication", Type=pywbem.Uint16(1),
                IPv4Type=pywbem.Uint16(4))
        self.assertEqual(len(rc), 2)
        self.assertEqual(rc[0], 0)
        settingData = rc[1]["SettingData"]

        services = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")
        self.assertGreater(len(services), 0)
        ipConfService = services[0]

        # Subscribe
        filter_name = "test_port_modify_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_IPNetworkConnection")

        # Emit event
        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection", ipConfService.path,
                SettingData=settingData, IPNetworkConnection=ipNetworkConnection.path, Mode=pywbem.Uint32(32768))
        self.assertGreater(len(rc), 0)
        self.assertIn(rc[0], [0, 4096])

        while 1:
            indication = self.get_indication(10)
            if indication.classname == "LMI_NetworkInstModification":
                self.assertIn("SourceInstance", indication.keys())
                source = indication["SourceInstance"]
                break

    def tearDown(self):
        for settingData in self.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if settingData["Caption"] == "XXX Test Indication":
                self.wbemconnection.DeleteInstance(settingData.path)

if __name__ == '__main__':
    import os
    os.environ["LMI_CIMOM_URL"] = "local://pegasus"
    unittest.main()
