
import sys
from test_base import TestBase
import pywbem
import datetime
import time

class TestIndications(TestBase):
    def create_setting(self):
        self.ipNetworkConnection = None
        for networkConnection in self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            if networkConnection["Name"] == self.port:
                self.ipNetworkConnection = networkConnection
        self.assertTrue(self.ipNetworkConnection is not None)
        capas = self.wbemconnection.Associators(self.ipNetworkConnection.path, ResultClass="LMI_IPNetworkConnectionCapabilities")
        self.assertGreater(len(capas), 0)
        capa = capas[0]

        self.caption = "XXX Test Jobs"
        rc = self.wbemconnection.InvokeMethod("LMI_CreateIPSetting", capa.path,
                Caption=self.caption, Type=pywbem.Uint16(1),
                IPv4Type=pywbem.Uint16(4))
        self.assertEqual(len(rc), 2)
        self.assertEqual(rc[0], 0)
        self.addCleanup(self.wbemconnection.DeleteInstance, rc[1]["SettingData"])
        return self.wbemconnection.GetInstance(rc[1]["SettingData"])

    def setUp(self):
        TestBase.setUp(self)
        filter_name = "test_job_%d" % (time.time() * 1000)
        sub = self.subscribe(filter_name, "select * from LMI_NetworkInstModification where SourceInstance isa LMI_NetworkJob")

    def test_job(self):
        """ Test job creation and its properties """
        settingData = self.create_setting()

        conf_services = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")
        self.assertEqual(len(conf_services), 1)
        conf_service = conf_services[0]

        rc = self.wbemconnection.InvokeMethod("ApplySettingToIPNetworkConnection",
                conf_service.path, SettingData=settingData.path,
                IPNetworkConnection=self.ipNetworkConnection.path,
                Mode=pywbem.Uint16(32768))
        self.assertGreater(len(rc), 1)
        self.assertEqual(rc[0], 4096) # Job started
        self.assertIn("Job", rc[1].keys())
        job = self.wbemconnection.GetInstance(rc[1]["Job"])

        self.assertEqual(job["DeleteOnCompletion"], True)
        self.assertTrue(job["TimeBeforeRemoval"].is_interval)
        self.assertEqual(job["TimeBeforeRemoval"].timedelta.seconds, 300)

        # Check LMI_OwningNetworkJobElement association
        assoc = self.wbemconnection.Associators(job.path, AssocClass="LMI_OwningNetworkJobElement")
        self.assertEqual(len(assoc), 1)
        self.assertEqual(assoc[0].classname, "LMI_IPConfigurationService")

        # Check LMI_AffectedNetworkJobElement association
        assoc = self.wbemconnection.Associators(job.path, AssocClass="LMI_AffectedNetworkJobElement")
        self.assertEqual(len(assoc), 2)
        for a in assoc:
            if a.classname == "LMI_IPAssignmentSettingData":
                self.assertEqual(a["Caption"], self.caption)
            elif a.classname == "LMI_IPNetworkConnection":
                self.assertEqual(a["Name"], self.port)
            else:
                self.assertTrue(False, "Invalid association to %s" % a.classname)

        # Wait for job completion
        self.wait_for_job(job)

        # Modify the job
        job["TimeBeforeRemoval"] = pywbem.CIMDateTime(datetime.timedelta(seconds=3))
        self.wbemconnection.ModifyInstance(job)

        # Check if the job is modified
        job = self.wbemconnection.GetInstance(job.path)
        self.assertTrue(job["TimeBeforeRemoval"].is_interval)
        self.assertEqual(job["TimeBeforeRemoval"].timedelta.seconds, 3)

        # Wait for job deletion
        time.sleep(5)

        # Check if the job was really deleted
        self.assertRaises(pywbem.CIMError, self.wbemconnection.GetInstance, job.path)
