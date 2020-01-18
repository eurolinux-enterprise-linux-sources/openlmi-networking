
from test_base import TestBase
import pywbem
import time

class TestBonding(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        if not self.fake_test and len(self.ports) < 2:
            self.skipTest("Specify at least two ports to test bonding")

        self.confService = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        self.networkConnections = []
        les = self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection")
        for le in les:
            if le["Name"] in self.ports and le["Name"] != "lo":
                self.networkConnections.append(le)
        print "Running test on: ", [le["Name"] for le in self.networkConnections]
        self.assertGreater(len(self.networkConnections), 1, "At least two ports required for bonding test")

        # Subscribe to indication
        self.filter_name = "test_apply_%d" % (time.time() * 1000)
        self.subscribe(self.filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_NetworkJob")

    def test_createBond(self):
        # Create Bond connection
        caps = self.wbemconnection.Associators(self.networkConnections[0].path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertEqual(len(caps), 1)
        cap = caps[0]

        caption = "XXX Test DHCP Bond"
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", cap.path,
                Caption=caption,
                Type=pywbem.Uint16(4), # Bonding
                IPv4Type=pywbem.Uint16(4), # DHCP
                IPv6Type=pywbem.Uint16(9)) # Stateless

        self.assertEqual(len(rc), 2)
        self.assertEqual(rc[0], 0)
        self.assertTrue(rc[1] is not None)
        settingData = self.wbemconnection.GetInstance(rc[1]["SettingData"])

        slaves = self.wbemconnection.AssociatorNames(settingData.path,
                ResultClass="LMI_BondingSlaveSettingData",
                AssocClass="LMI_OrderedIPAssignmentComponent")
        self.assertEqual(len(slaves), 1)

        # Save the slave SettingData
        slaveSettingDataList = {}
        slaveSettingDataList[self.networkConnections[0]] = slaves[0]

        # Delete the connection on cleanup
        # Deleting master connection will also delete the slaves
        self.addCleanup(self.wbemconnection.DeleteInstance, settingData.path)

        # Check object presence
        for sd in self.wbemconnection.EnumerateInstances("LMI_BondingMasterSettingData"):
            if sd["Caption"] == caption:
                break
        else:
            self.assertTrue(False, "No instance of LMI_BondingMasterSettingData with Caption \"%s\"" % caption)

        # Check if properties are present
        self.assertTrue(settingData["InterfaceName"] is not None)
        self.assertEqual(settingData["Mode"], 0)
        self.assertEqual(settingData["MIIMon"], 100)
        self.assertEqual(settingData["DownDelay"], 0)
        self.assertEqual(settingData["UpDelay"], 0)
        self.assertEqual(settingData["ARPInterval"], 0)
        self.assertEqual(len(settingData["ARPIPTarget"]), 0)

        # Modify some options
        settingData["Mode"] = pywbem.Uint16(1)
        settingData["MIIMon"] = pywbem.Uint64(0)
        settingData["ARPInterval"] = pywbem.Uint64(1)
        settingData["ARPIPTarget"] = ["192.168.1.1", "192.168.1.2"]
        self.wbemconnection.ModifyInstance(settingData)
        settingData = self.wbemconnection.GetInstance(settingData.path)

        # Check if the options are modified
        self.assertEqual(settingData["Mode"], 1)
        self.assertEqual(settingData["ARPInterval"], 1)
        self.assertEqual(len(settingData["ARPIPTarget"]), 2)
        self.assertListEqual(settingData["ARPIPTarget"], ["192.168.1.1", "192.168.1.2"])

        # Modify some other options
        settingData["Mode"] = pywbem.Uint16(2)
        settingData["MIIMon"] = pywbem.Uint64(200)
        settingData["DownDelay"] = pywbem.Uint64(200)
        settingData["UpDelay"] = pywbem.Uint64(200)
        settingData["ARPInterval"] = pywbem.Uint64(0)
        settingData["ARPIPTarget"] = pywbem.CIMProperty("ARPIPTarget", [], type='string', is_array=True)
        self.wbemconnection.ModifyInstance(settingData)
        settingData = self.wbemconnection.GetInstance(settingData.path)

        # Check if the options are modified
        self.assertEqual(settingData["Mode"], 2)
        self.assertEqual(settingData["MIIMon"], 200)
        self.assertEqual(settingData["DownDelay"], 200)
        self.assertEqual(settingData["UpDelay"], 200)
        self.assertEqual(settingData["ARPInterval"], 0)
        self.assertEqual(len(settingData["ARPIPTarget"]), 0)

        # Add slaves, first one is added automatically when creating IP setting
        for networkConnection in self.networkConnections[1:]:
            caps = self.wbemconnection.Associators(networkConnection.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
            self.assertEqual(len(caps), 1)
            cap = caps[0]

            rc = self.wbemconnection.InvokeMethod("LMI_CreateSlaveSetting", cap.path, MasterSettingData=settingData.path)
            self.assertEqual(len(rc), 2)
            self.assertEqual(rc[0], 0)
            self.assertEqual(rc[1]["SettingData"].classname, "LMI_BondingSlaveSettingData")
            slaveSettingDataList[networkConnection] = rc[1]["SettingData"]

        # Check if the slave SettingDatas are associated to master
        assoc = self.wbemconnection.Associators(settingData.path, ResultClass="LMI_BondingSlaveSettingData")
        self.assertEqual(len(assoc), len(self.networkConnections))

        # Test modification of slave setting data
        slaveSettingData = assoc[0]
        caption = "%s Modified" % slaveSettingData["Caption"]
        slaveSettingData["Caption"] = caption
        self.wbemconnection.ModifyInstance(slaveSettingData)
        slaveSettingData = self.wbemconnection.GetInstance(slaveSettingData.path)

        # Check if the properties are modified
        self.assertEqual(slaveSettingData["Caption"], caption)

        # Activate connection using SlaveSettingData
        for networkConnection in self.networkConnections:
            rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection", self.confService.path,
                    SettingData=slaveSettingDataList[networkConnection], IPNetworkConnection=networkConnection.path, Mode=pywbem.Uint16(32768))
            self.assertIn(rc[0], [0, 4096])
            if rc[0] == 4096: # Job started
                # Wait for job completed indication
                self.wait_for_job_completion(rc[1]["Job"])

        # Check presence of master port
        aggregators = self.wbemconnection.Associators(sd.path, ResultClass="LMI_LinkAggregator8023ad",
                AssocClass="LMI_IPElementSettingData")
        self.assertGreater(len(aggregators), 0)
        aggregator = aggregators[0]

        lagPorts = self.wbemconnection.EnumerateInstances("LMI_LAGPort8023ad")

        # Check presence of LMI_LAGPort8023ad for each slave
        lagPortNames = [x["Name"] for x in lagPorts]
        for networkConnection in self.networkConnections:
            self.assertIn(networkConnection["Name"], lagPortNames)

        # Check if the slaves are associated with master
        aggregatedPorts = self.wbemconnection.Associators(aggregator.path, ResultClass="LMI_LAGPort8023ad",
                AssocClass="LMI_LinkAggregationBindsTo")
        for aggregatedPort in aggregatedPorts:
            self.assertIn(aggregatedPort["Name"], lagPortNames)

        # Check if slaves are aggregated to LANEndpoints
        for lagPort in lagPorts:
            lanendpoints = self.wbemconnection.Associators(lagPort.path, ResultClass="LMI_LANEndpoint",
                    AssocClass="LMI_LinkAggregationConcreteIdentity")
            self.assertEqual(len(lanendpoints), 1)
            self.assertEqual(lanendpoints[0]["Name"], lagPort["Name"])

    @classmethod
    def setUpClass(cls):
        TestBase.setUpClass()

    @classmethod
    def tearDownClass(cls):
        TestBase.tearDownClass()
        # Give it some time to actually delete the connections
        time.sleep(1)
        connections = []
        for connection in cls.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData"):
            if connection["Caption"].startswith("XXX "):
                connections.append(connection["Caption"])
        if len(connections) > 0:
            raise AssertionError("Connections was not deleted: %s" % ", ".join(connections))
