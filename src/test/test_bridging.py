
from test_base import TestBase
import pywbem
import time
import random

class TestBriding(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        if not self.fake_test and len(self.ports) < 2:
            self.skipTest("Specify at least two ports to test bridging")

        self.confService = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")[0]

        self.networkConnections = []
        les = self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection")
        for le in les:
            if le["Name"] in self.ports and le["Name"] != "lo":
                self.networkConnections.append(le)
        print "Running test on: ", [le["Name"] for le in self.networkConnections]
        self.assertGreater(len(self.networkConnections), 1, "At least two ports required for bridging test")

        # Subscribe to indication
        self.filter_name = "test_apply_%d" % (time.time() * 1000)
        self.subscribe(self.filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_NetworkJob")

    def test_createBridge(self):
        # Create Bridge connection
        caps = self.wbemconnection.Associators(self.networkConnections[0].path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertEqual(len(caps), 1)
        cap = caps[0]

        caption = "XXX Test DHCP Bridge"
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", cap.path,
                Caption=caption,
                Type=pywbem.Uint16(5), # Bridging
                IPv4Type=pywbem.Uint16(4), # DHCP
                IPv6Type=pywbem.Uint16(9)) # Stateless

        self.assertEqual(len(rc), 2)
        self.assertEqual(rc[0], 0)
        self.assertTrue(rc[1] is not None)
        settingData = self.wbemconnection.GetInstance(rc[1]["SettingData"])

        slaves = self.wbemconnection.AssociatorNames(settingData.path,
                ResultClass="LMI_BridgingSlaveSettingData",
                AssocClass="LMI_OrderedIPAssignmentComponent")
        self.assertEqual(len(slaves), 1)

        # Save the slave SettingData
        slaveSettingDataList = {}
        slaveSettingDataList[self.networkConnections[0]] = slaves[0]

        # Delete the connection on cleanup
        # Deleting master connection will also delete the slaves
        self.addCleanup(self.wbemconnection.DeleteInstance, settingData.path)

        # Check object presence
        for sd in self.wbemconnection.EnumerateInstances("LMI_BridgingMasterSettingData"):
            if sd["Caption"] == caption:
                break
        else:
            self.assertTrue(False, "No instance of LMI_IPAssignmentSettingData with Caption \"%s\"" % caption)

        # Check if properties are present
        self.assertTrue(settingData["AgeingTime"] is not None)
        self.assertTrue(settingData["ForwardDelay"] is not None)
        self.assertTrue(settingData["HelloTime"] is not None)
        self.assertTrue(settingData["InterfaceName"] is not None)
        self.assertTrue(settingData["MaxAge"] is not None)
        self.assertTrue(settingData["Priority"] is not None)
        self.assertTrue(settingData["STP"] is not None)

        # Modify some properties
        settingData["AgeingTime"] = pywbem.Uint32(301)
        settingData["ForwardDelay"] =  pywbem.Uint32(16)
        settingData["HelloTime"] = pywbem.Uint32(3)
        # Use random name to minimize probability to reuse existing port name
        bridgeName = "bridge%d" % random.randint(0, 100)
        settingData["InterfaceName"] = bridgeName
        settingData["MaxAge"] = pywbem.Uint32(21)
        settingData["Priority"] = pywbem.Uint32(128)
        self.wbemconnection.ModifyInstance(settingData)
        settingData = self.wbemconnection.GetInstance(settingData.path)

        # Check if the properties are modified
        self.assertEqual(settingData["AgeingTime"], 301)
        self.assertEqual(settingData["ForwardDelay"], 16)
        self.assertEqual(settingData["HelloTime"], 3)
        self.assertEqual(settingData["InterfaceName"], bridgeName)
        self.assertEqual(settingData["MaxAge"], 21)
        self.assertEqual(settingData["Priority"], 128)
        self.assertEqual(settingData["STP"], True)

        # Add slaves, first one is added automatically when creating IP setting
        for networkConnection in self.networkConnections[1:]:
            caps = self.wbemconnection.Associators(networkConnection.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
            self.assertEqual(len(caps), 1)
            cap = caps[0]

            rc = self.wbemconnection.InvokeMethod("LMI_CreateSlaveSetting", cap.path, MasterSettingData=settingData.path)
            self.assertEqual(len(rc), 2)
            self.assertEqual(rc[0], 0)
            self.assertEqual(rc[1]["SettingData"].classname, "LMI_BridgingSlaveSettingData")
            slaveSettingDataList[networkConnection] = rc[1]["SettingData"]

        # Check if the slave SettingDatas are associated to master
        assoc = self.wbemconnection.Associators(settingData.path, ResultClass="LMI_BridgingSlaveSettingData")
        self.assertEqual(len(assoc), len(self.networkConnections))

        # Test modification of slave setting data
        slaveSettingData = assoc[0]
        slaveSettingData["Priority"] = pywbem.Uint32(33)
        slaveSettingData["PathCost"] = pywbem.Uint32(101)
        slaveSettingData["HairpinMode"] = True
        self.wbemconnection.ModifyInstance(slaveSettingData)
        slaveSettingData = self.wbemconnection.GetInstance(slaveSettingData.path)

        # Check if the properties are modified
        self.assertEqual(slaveSettingData["Priority"], 33)
        self.assertEqual(slaveSettingData["PathCost"], 101)
        self.assertEqual(slaveSettingData["HairpinMode"], True)

        # Activate connection using SlaveSettingData
        for networkConnection in self.networkConnections:
            rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection", self.confService.path,
                    SettingData=slaveSettingDataList[networkConnection], IPNetworkConnection=networkConnection.path, Mode=pywbem.Uint16(32768))
            self.assertIn(rc[0], [0, 4096])
            if rc[0] == 4096: # Job started
                # Wait for job completed indication
                self.wait_for_job_completion(rc[1]["Job"])

        # Check presence of SwitchService
        switchServices = self.wbemconnection.Associators(settingData.path, ResultClass="LMI_SwitchService",
                AssocClass="LMI_IPElementSettingData")
        self.assertGreater(len(switchServices), 0)
        switchService = switchServices[0]

        switchPorts = self.wbemconnection.EnumerateInstances("LMI_SwitchPort")
        self.assertEqual(len(switchPorts), 2)

        # Check presence of LMI_SwitchPort for each slave
        switchPortNames = [x["Name"] for x in switchPorts]
        for networkConnection in self.networkConnections:
            self.assertIn(networkConnection["Name"], switchPortNames)

        # Check if the slaves are associated with master
        aggregatedPorts = self.wbemconnection.Associators(switchService.path, ResultClass="LMI_SwitchPort",
                AssocClass="LMI_SwitchesAmong")
        self.assertEqual(len(aggregatedPorts), 2)
        for aggregatedPort in aggregatedPorts:
            self.assertIn(aggregatedPort["Name"], switchPortNames)

        # Check if slaves are identical to LANEndpoints
        for switchPort in switchPorts:
            lanendpoints = self.wbemconnection.Associators(switchPort.path, ResultClass="LMI_LANEndpoint",
                    AssocClass="LMI_EndpointIdentity")
            self.assertEqual(len(lanendpoints), 1)
            self.assertEqual(lanendpoints[0]["Name"], switchPort["Name"])

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
