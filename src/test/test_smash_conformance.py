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
# This test checks if the networking provider is compatible with parts of SMASH
# standard
#
from test_base import TestBase

import socket

ComputerSystemClass = "PG_ComputerSystem"

def IPv4_valid(ip):
    try:
        socket.inet_pton(socket.AF_INET, ip)
        return True
    except socket.error:
        return False

def IPv6_valid(ip):
    try:
        socket.inet_pton(socket.AF_INET6, ip)
        return True
    except socket.error:
        return False

class TestHostLANNetworkPortProfile(TestBase):
    """
    This test checks conformity against DSP1035 (version 1.0.2 - 2011-06-30).
    """
    def setUp(self):
        TestBase.setUp(self)

    def test_network_port_state_management(self):
        """ Section 7.1 of DSP 1035 """

        ports = self.wbemconnection.EnumerateInstances("LMI_EthernetPort")

        for port in ports:
            # 7.1.1 - Enabled but Offline
            # Not implemented - don't know how to emulate unplugged cable

            # 7.1.2 - Network Port State Management Is Supported - not our case

            # 7.1.3 - Network Port State Management Is Not Supported
            # 7.1.3.1 CIM_EnabledLogicalElementCapabilities
            capabilities = self.wbemconnection.Associators(port.path,
                    ResultClass="LMI_NetworkEnabledLogicalElementCapabilities",
                    AssocClass="LMI_NetworkElementCapabilities")
            self.assertEqual(len(capabilities), 1)
            # 7.1.3.1.1 - CIM_EnabledLogicalElementCapabilities.RequestedStatesSupported
            self.assertEqual(len(capabilities[0]["RequestedStatesSupported"]), 0,
                    "RequestedStatesSupported property of LMI_NetworkEnabledLogicalElementCapabilities should be empty list, is %s" % str(capabilities[0]["RequestedStatesSupported"]))
            # 7.1.3.2 - CIM_NetworkPort.RequestedState
            self.assertEqual(port["RequestedState"], 12) # Not Applicable
            # 7.1.3.3 - CIM_NetworkPort.EnabledState
            self.assertIn(port["EnabledState"], [2, 3, 5, 6])

            # 7.1.4 - Modifying ElementName Is Supported - Conditional - not our case

            # 7.1.5 - Modifying ElementName Is Not Supported
            # 7.1.5.1 - CIM_EnabledLogicalElementCapabilities
            # 7.1.5.1.1 - CIM_EnabledLogicalElementCapabilities.ElementNameEditSupported
            self.assertTrue(not capabilities[0]["ElementNameEditSupported"])

            # 7.1.6 - Representing the Physical Packaging - not implemented

    def test_representing_communication_endpoint(self):
        """ Section 7.2 of DSP 1035 """

        ports = self.wbemconnection.EnumerateInstances("LMI_EthernetPort")

        for port in ports:
            # 7.2.1 - Endpoint Identified by Hardware MAC
            # There shall be exactly one instance of CIM_LANEndpoint in which
            # the MACAddress property has the same value as the PermanentAddress
            # property of the associated CIM_NetworkPort instance.
            # 7.2.2 is almost the same
            lanEndpoints = self.wbemconnection.Associators(port.path,
                    ResultClass="LMI_LANEndpoint")
            self.assertGreater(len(lanEndpoints), 0)
            if port["PermanentAddress"] is not None and port["PermanentAddress"] != "":
                for lanEndpoint in lanEndpoints:
                    if lanEndpoint["MACAddress"] == port["PermanentAddress"]:
                        break
                else:
                    self.fail("No LANEndpoint with MACAddress same as NetworkPort's PermanentAddress (%s)" % port["DeviceID"])

        for lan in self.wbemconnection.EnumerateInstances("LMI_LANEndpoint"):
            # 7.2.3 - Relationship between the Interface and Port
            ports = self.wbemconnection.Associators(lan.path,
                    ResultClass="LMI_EthernetPort",
                    AssocClass="LMI_NetworkDeviceSAPImplementation")
            self.assertEqual(len(ports), 1)

            # 7.2.4 - Endpoint State Management Is Supported - Conditional
            # 7.2.4.1 - CIM_EnabledLogicalElementCapabilities
            capabilities = self.wbemconnection.Associators(lan.path,
                    ResultClass="LMI_NetworkEnabledLogicalElementCapabilities",
                    AssocClass="LMI_NetworkElementCapabilities")
            self.assertEqual(len(capabilities), 1)
            # 7.2.4.1.1 - CIM_EnabledLogicalElementCapabilities.RequestedStatesSupported
            self.assertListEqual(capabilities[0]["RequestedStatesSupported"], [2, 3])

            # 7.2.4.2 - CIM_LANEndpoint.RequestedState
            self.assertIn(lan["RequestedState"], capabilities[0]["RequestedStatesSupported"] + [5])

            # 7.2.4.3 - CIM_LANEndpoint.EnabledState
            self.assertIn(lan["EnabledState"], [2, 3])

            # 7.2.5 - Endpoint State Management Is Not Supported  - not our case
            # 7.2.6 - Modifying ElementName Is Supported - Conditional - not our case
            # 7.2.7 - Modifying ElementName Is Not Supported
            # 7.2.7.1 - CIM_EnabledLogicalElementCapabilities
            # 7.2.7.1.1 - CIM_EnabledLogicalElementCapabilities.ElementNameEditSupported
            self.assertTrue(not capabilities[0]["ElementNameEditSupported"])

    def test_managing_network_endpoints(self):
        """ Section 7.3 of DSP 1035 """
        # Creating network endpoints is not supported
        self.assertTrue("LMI_NetworkPortConfigurationService" not in self.wbemconnection.EnumerateClassNames(DeepInheritance=True))

    def test_representing_multiple_ports_controlled_from_a_single_controller(self):
        """ Section 7.4 of DSP 1035 """
        # Multiple ports controlled from single controller is not supported
        self.assertTrue("LMI_PortController" not in self.wbemconnection.EnumerateClassNames(DeepInheritance=True))

class TestIPConfigurationProfile(TestBase):
    """
    This test checks conformity against DSP1116 (version 1.0.0).
    """
    def assertAssociatedWithComputerSystem(self, cls, AssocClass):
        assocs = self.wbemconnection.Associators(cls.path, AssocClass=AssocClass, ResultClass=ComputerSystemClass)
        self.assertEqual(len(assocs), 1, "Instance of class %s should be associated with %s via %s" % (cls.classname, ComputerSystemClass, AssocClass))


    def assertAssociationCount(self, cls, count=1, **kw):
        assoc = self.wbemconnection.Associators(cls.path, **kw)

        self.assertEqual(len(assoc), count,
            "Instance of %s should be associated to %d instance(s) of %s via %s. %d associations found" %
            (cls.classname, count, kw.get("ResultClass", "<any>"), kw.get("AssocClass", "<any>"), len(assoc)))
        return assoc

    def test_representing_network_connection(self):
        """
        Representing the network connection.

        Section 7.1 of DSP 1116
        """

        # 7.1.1 - CIM_IPNetworkConnection
        ports = self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection")
        for port in ports:
            self.assertAssociatedWithComputerSystem(port,
                    AssocClass="LMI_NetworkHostedAccessPoint")

        # 7.1.2 Managing the CIM_IPNetworkConnection state
        for port in ports:
            self.assertIn(port["EnabledState"], [2, 3, 5, 6]) # Enabled, Disabled, Not Applicable, Enabled but Offline
            self.assertEqual(port["RequestedState"], 12) # Not Applicable

    def test_representing_ip_version(self):
        """
        Representing the IP version

        Section 7.2 of DSP 1116
        """

        ports = self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection")
        # 7.2.1 - CIM_IPVersionSettingData
        settingDatas = self.wbemconnection.EnumerateInstances("LMI_IPVersionSettingData")
        self.assertGreater(len(settingDatas), 0)
        for settingData in settingDatas:
            self.assertAssociatedWithComputerSystem(settingData,
                    AssocClass="LMI_IPVersionElementSettingData")

            self.assertAssociationCount(settingData, ResultClass="LMI_IPNetworkConnection", AssocClass="LMI_IPVersionElementSettingData", count=len(ports))

            # 7.2.1.1 - CIM_IPVersionSettingData.ProtocolIFType
            self.assertIn(settingData["ProtocolIFType"], [4096, 4097]) # IPv4 or IPv6

    def test_ip_setting(self):
        """
        IP setting

        Section 7.3 of DSP 1116
        """

        settingDatas = self.wbemconnection.EnumerateInstances("LMI_IPAssignmentSettingData")
        for settingData in settingDatas:
            # Static, DHCP, BOOTP, IPv4 Link Local, DHCPv6, IPv6AutoConfig, Stateless, Link Local, cumulative configuration
            self.assertIn(settingData["AddressOrigin"], [3, 4, 5, 6, 7, 8, 9, 10, 11])

            # 7.3.1 - CIM_IPAssignmentSettingData requirements for accumulation of
            #         settings, stateless IP assignment settings

            # 7.3.1.2 - CIM_IPAssignmentSettingData.ProtocolIFType
            if settingData["AddressOrigin"] == 9: # Stateless
                self.assertEqual(settingData["ProtocolIFType"], 4097) # IPv6


            # 7.3.1.3 - CIM_IPAssignmentSettingData.ConfigurationName
            if settingData["AddressOrigin"] == 11: # cumulative configuration
                self.assertTrue(settingData["ConfigurationName"] != "")

            # 7.3.2 - CIM_ExtendedStaticIPAssignmentSettingData requirements
            #         for static IP assignment settings
            if settingData.classname == "LMI_ExtendedStaticIPAssignmentSettingData":
                # 7.3.2.1 - CIM_ExtendedStaticIPAssignmentSettingData.AddressOrigin
                self.assertEqual(settingData["AddressOrigin"], 3) # Static
                # 7.3.2.2 - CIM_ExtendedStaticIPAssignmentSettingData.ProtocolIFType
                self.assertIn(settingData["ProtocolIFType"], [4096, 4097]) # IPv4 or IPv6
                # 7.3.2.3 - CIM_ExtendedStaticIPAssignmentSettingData.IPAddresses
                for ip in settingData["IPAddresses"]:
                    if settingData["ProtocolIFType"] == 4096:
                        self.assertTrue(IPv4_valid(ip), "IPv4 address is not valid")
                    else:
                        self.assertTrue(IPv6_valid(ip), "IPv6 address is not valid")
                # 7.3.2.4 - CIM_ExtendedStaticIPAssignmentSettingData.IPv6SubnetPrefixLengths
                if settingData["ProtocolIFType"] == 4097:
                    self.assertEqual(len(settingData["IPAddresses"]), len(settingData["IPv6SubnetPrefixLengths"]))
                else:
                    self.assertIsNone(settingData["IPv6SubnetPrefixLengths"])
                # 7.3.2.5 - CIM_ExtendedStaticIPAssignmentSettingData.SubnetMasks
                if settingData["ProtocolIFType"] == 4096:
                    self.assertEqual(len(settingData["IPAddresses"]), len(settingData["SubnetMasks"]))
                else:
                    self.assertIsNone(settingData["SubnetMasks"])
                # 7.3.2.6 - CIM_ExtendedStaticIPAssignmentSettingData.GatewayAddresses
                self.assertEqual(len(settingData["IPAddresses"]), len(settingData["GatewayAddresses"]))
                for ip in settingData["GatewayAddresses"]:
                    if settingData["ProtocolIFType"] == 4096:
                        self.assertTrue(IPv4_valid(ip), "IPv4 gateway is not valid")
                    else:
                        self.assertTrue(IPv6_valid(ip), "IPv6 gateway is not valid")

            # 7.3.3 - CIM_DHCPSettingData requirements for dynamic IP assignment settings
            if settingData.classname == "LMI_DHCPSettingData":
                # 7.3.3.1 - CIM_DHCPSettingData.AddressOrigin
                self.assertIn(settingData["AddressOrigin"], [4, 7]) # DHCP or DHCPv6
                # 7.3.3.2 - CIM_DHCPSettingData.ProtocolIFType
                if settingData["AddressOrigin"] == 4: # IPv4
                    self.assertEqual(settingData["ProtocolIFType"], 4096)
                else:
                    self.assertEqual(settingData["ProtocolIFType"], 4097)

    def test_representation_of_current_and_pending_settings(self):
        """
        Representation of current and pending settings.

        Section 7.4 of DSP1116
        """
        # 7.4.1 - CIM_ElementSettingData
        elementSettingDatas = self.wbemconnection.EnumerateInstances("LMI_IPElementSettingData")
        for elementSettingData in elementSettingDatas:
            # 7.4.1.1 - CIM_ElementSettingData.IsCurrent
            self.assertIn(elementSettingData["IsCurrent"], [1, 2]) # Is Current or Is Not Current
            # 7.4.1.2 - CIM_ElementSettingData.IsNext
            self.assertIn(elementSettingData["IsNext"], [1, 3, 2]) # Is Next or Is Next for Single user or Is Not Next
        # 7.4.2 - Modification of CIM_SettingData - not testable

    def test_representation_settings_of_network_connection(self):
        """
        Representation settings of a network connection

        Section 7.5 of DSP1116
        """
        # Concurrent settings and accumulation of settings are not testable here,
        # because we don't know which setting is concurrent (accumulation).
        pass

    def test_representing_ip_interface(self):
        """
        Representing the IP interface

        Section 7.6 of DSP1116
        """
        # 7.6.1 - CIM_IPProtocolEndpoint
        endpoints = self.wbemconnection.EnumerateInstances("LMI_IPProtocolEndpoint")
        for endpoint in endpoints:
            self.assertAssociatedWithComputerSystem(endpoint,
                    AssocClass="LMI_NetworkHostedAccessPoint")
            self.assertAssociationCount(endpoint, ResultClass="LMI_IPNetworkConnection", AssocClass="LMI_NetworkSAPSAPDependency")
            # 7.6.1.1 - CIM_IPProtocolEndpoint.AddressOrigin
            # TODO: get rid of 0, shouldn't be there
            self.assertIn(endpoint["AddressOrigin"], [0, 3, 4, 7, 9, 10]) # Static, DHCPv4, DHCPv6, Stateless, Link Local
            # 7.6.1.2 - CIM_IPProtocolEndpoint.ProtocolIFType
            self.assertIn(endpoint["ProtocolIFType"], [4096, 4097]) # IPv4 or IPv6
            if endpoint["ProtocolIFType"] == 4096:
                self.assertTrue(endpoint["IPv4Address"] is not None)
                self.assertTrue(endpoint["SubnetMask"] is not None)
            else:
                self.assertTrue(endpoint["IPv6Address"] is not None)
                self.assertTrue(endpoint["IPv6SubnetPrefixLength"] is not None)

            # 7.6.1.3 - CIM_IPProtocolEndpoint.IPv4Address
            if endpoint["ProtocolIFType"] == 4096:
                self.assertTrue(IPv4_valid(endpoint["IPv4Address"]), "IPv4 address is not valid")
            else:
                self.assertIsNone(endpoint["IPv4Address"])
            # 7.6.1.4 - CIM_IPProtocolEndpoint.SubnetMask
            if endpoint["ProtocolIFType"] == 4096:
                self.assertTrue(IPv4_valid(endpoint["SubnetMask"]), "IPv4 subnet mask is not valid")
            else:
                self.assertIsNone(endpoint["SubnetMask"])
            # 7.6.1.5 - CIM_IPProtocolEndpoint.IPv6Address
            if endpoint["ProtocolIFType"] == 4097:
                self.assertTrue(IPv6_valid(endpoint["IPv6Address"]), "IPv6 address is not valid")
            else:
                self.assertIsNone(endpoint["IPv6Address"])
            # 7.6.1.6 - CIM_IPProtocolEndpoint.IPv6SubnetPrefixLength
            if endpoint["ProtocolIFType"] == 4097:
                self.assertTrue(endpoint["IPv6SubnetPrefixLength"] is not None, "IPv6 prefix length is not valid")
            else:
                self.assertIsNone(endpoint["IPv6SubnetPrefixLength"])

    def test_ip_configuration_management(self):
        """
        IP configuration management

        Section 7.7 of DSP1116
        """
        # 7.7.1 - Configuration management is supported (optional)
        confServices = self.wbemconnection.EnumerateInstances("LMI_IPConfigurationService")
        self.assertEqual(len(confServices), 1)
        confService = confServices[0]

        self.assertAssociatedWithComputerSystem(confService,
                AssocClass="LMI_HostedIPConfigurationService")

        for networkConnection in self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            self.assertAssociationCount(networkConnection, ResultClass="LMI_IPConfigurationService", AssocClass="LMI_IPConfigurationServiceAffectsElement")

    def test_dhcp_client(self):
        """
        DHCP client

        Section 7.8 of DSP1116
        """
        # not yet implemented

    def test_dns_client_and_configuration(self):
        """
        DNS client and configuration

        Section 7.9 of DSP1116
        """
        # not yet implemented

    def test_relationship_with_network_interface(self):
        """
        Relationship with a network interface

        Section 7.10 of DSP1116
        """
        for networkConnection in self.wbemconnection.EnumerateInstances("LMI_IPNetworkConnection"):
            self.assertAssociationCount(networkConnection, ResultClass="LMI_LANEndpoint", AssocClass="LMI_EndpointForIPNetworkConnection")
        for protocolEndpoint in self.wbemconnection.EnumerateInstances("LMI_IPProtocolEndpoint"):
            self.assertAssociationCount(protocolEndpoint, ResultClass="LMI_LANEndpoint", AssocClass="LMI_BindsToLANEndpoint")

    def test_remote_services(self):
        """
        Remote services

        Section 7.11 of DSP1116
        """
        for rsap in self.wbemconnection.EnumerateInstances("LMI_NetworkRemoteServiceAccessPoint"):
            # 7.11.1 - Default gateway
            # 7.11.1.1 - CIM_RemoteServiceAccessPoint
            # 7.11.1.1.1 - CIM_RemoteServiceAccessPoint.AccessContext
            if rsap["AccessContext"] == 2: # Default Gateway
                # 7.11.1.1.2 - CIM_RemoteServiceAccessPoint.AccessInfo
                self.assertTrue(IPv4_valid(rsap["AccessInfo"]) or IPv6_valid(rsap["AccessInfo"]), "Invalid format of default gateway")
                # 7.11.1.2 - CIM_RemoteAccessAvailableToElement
                self.assertAssociationCount(rsap, ResultClass="LMI_IPNetworkConnection", AssocClass="LMI_NetworkRemoteAccessAvailableToElement")
                self.assertAssociatedWithComputerSystem(rsap,
                        AssocClass="LMI_NetworkRemoteAccessAvailableToElement")

            # 7.11.2 DHCP servers - not yet implemented
            # 7.11.3 DNS servers - not yet implemented
            """
            # 7.11.3.1 - CIM_RemoteServiceAccessPoint
            # 7.11.3.1.1 - CIM_RemoteServiceAccessPoint.AccessContext
            elif rsap["AccessContext"] == 3: # DNS Server
                # 7.11.3.1.2 - CIM_RemoteServiceAccessPoint.AccessInfo
                self.assertTrue(IPv4_valid(rsap["AccessInfo"]) or IPv6_valid(rsap["AccessInfo"]), "Invalid format of dns server")
                # 7.11.3.2 - CIM_RemoteAccessAvailableToElement
                assoc = self.wbemconnection.Associators(rsap.path,
                        ResultClass="LMI_IPNetworkConnection",
                        AssocClass="LMI_RemoteAccessAvailableToElement")
                self.assertEqual(len(assoc), 1)
                assoc = self.wbemconnection.Associators(rsap.path,
                        ResultClass=ComputerSystemClass,
                        AssocClass="LMI_RemoteAccessAvailableToElement")
                self.assertEqual(len(assoc), 1)
            """
