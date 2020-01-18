/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#include "ipnetworkconnection.h"
#include "port.h"

/* CIM says:
 *
 * " Ethernet/802.3 MAC addresses formatted as twelve hexadecimal digits "
 * "(for example, ``010203040506``), with each pair representing one "
 * "of the six octets of the MAC address in ``canonical`` bit order. "
 * "(Therefore, the Group address bit is found in the low order bit "
 * "of the first character of the string.)",
 *
 * this function will create that format out of colon separed hex pairs
 */
static const char *format_mac(const char *mac)
{
    if (mac == NULL) {
        return NULL;
    }
    static char m[13];
    if (strlen(mac) != 17) {
        return mac;
    }
    snprintf(m, 13, "%c%c%c%c%c%c%c%c%c%c%c%c", mac[0], mac[1], mac[3], mac[4],
             mac[6], mac[7], mac[9], mac[10],
             mac[12], mac[13], mac[15], mac[16]);
    return m;
}

int convert_operating_status(PortOperatingStatus status, IPNetworkConnectionTypes type)
{
    switch (status) {
        case STATUS_NA:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Not_Available :
                LMI_LANEndpoint_OperatingStatus_Not_Available;
        case STATUS_IN_SERVICE:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_In_Service :
                LMI_LANEndpoint_OperatingStatus_In_Service;
        case STATUS_STARTING:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Starting :
                LMI_LANEndpoint_OperatingStatus_Starting;
        case STATUS_STOPPING:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Stopping :
                LMI_LANEndpoint_OperatingStatus_Stopping;
        case STATUS_STOPPED:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Stopped :
                LMI_LANEndpoint_OperatingStatus_Stopped;
            break;
        case STATUS_OFFLINE:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Dormant :
                LMI_LANEndpoint_OperatingStatus_Dormant;
        case STATUS_ABORTED:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Aborted :
                LMI_LANEndpoint_OperatingStatus_Aborted;
        default:
            return (type == LMI_IPNetworkConnection_Type) ?
                LMI_IPNetworkConnection_OperatingStatus_Unknown :
                LMI_LANEndpoint_OperatingStatus_Unknown;
    }
}

CMPIStatus port_to_IPNetworkConnection(
    const Port *port,
    LMI_IPNetworkConnection *w,
    const CMPIContext *cc)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_IPNetworkConnection_Set_CreationClassName(w, LMI_IPNetworkConnection_ClassName);
    LMI_IPNetworkConnection_Set_Name(w, port_get_id(port));
    LMI_IPNetworkConnection_Set_SystemCreationClassName(w, get_system_creation_class_name());
    LMI_IPNetworkConnection_Set_SystemName(w, lmi_get_system_name_safe(cc));

    LMI_IPNetworkConnection_Set_OperatingStatus(w,
            convert_operating_status(port_get_operating_status(port),
                                     LMI_IPNetworkConnection_Type));

    LMI_IPNetworkConnection_Set_ElementName(w, port_get_id(port));
    return res;
}

CMPIStatus port_to_LANEndpoint(
    const Port *port,
    LMI_LANEndpoint *w,
    const CMPIContext *cc)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_LANEndpoint_Set_CreationClassName(w, LMI_LANEndpoint_ClassName);
    LMI_LANEndpoint_Set_Name(w, port_get_id(port));
    LMI_LANEndpoint_Set_SystemCreationClassName(w, get_system_creation_class_name());
    LMI_LANEndpoint_Set_SystemName(w, lmi_get_system_name_safe(cc));

    LMI_LANEndpoint_Set_MACAddress(w, port_get_mac(port));
    LMI_LANEndpoint_Null_NameFormat(w);
    LMI_LANEndpoint_Set_ElementName(w, port_get_id(port));
    LMI_LANEndpoint_Set_ProtocolIFType(w, LMI_LANEndpoint_ProtocolIFType_Ethernet_CSMA_CD);

    LMI_LANEndpoint_Set_OperatingStatus(w,
            convert_operating_status(port_get_operating_status(port),
                                     LMI_LANEndpoint_Type));

    // AvailableRequestedStates indicates the possible values for the
    // RequestedState parameter of the method RequestStateChange, used to
    // initiate a state change.
    LMI_LANEndpoint_Init_AvailableRequestedStates(w, 2);
    LMI_LANEndpoint_Set_AvailableRequestedStates(w, 0, LMI_LANEndpoint_AvailableRequestedStates_Enabled);
    LMI_LANEndpoint_Set_AvailableRequestedStates(w, 1, LMI_LANEndpoint_AvailableRequestedStates_Disabled);

    // RequestedState is an integer enumeration that indicates the last
    // requested or desired state for the element, irrespective of
    // the mechanism through which it was requested.
    switch (port_get_requested_state(port)) {
        case STATE_DISABLED:
            LMI_LANEndpoint_Set_RequestedState(w, LMI_LANEndpoint_RequestedState_Disabled);
            break;
        case STATE_ENABLED:
            LMI_LANEndpoint_Set_RequestedState(w, LMI_LANEndpoint_RequestedState_Enabled);
            break;
        case STATE_NO_CHANGE:
            LMI_LANEndpoint_Set_RequestedState(w, LMI_LANEndpoint_RequestedState_No_Change);
            break;
        default:
            LMI_LANEndpoint_Set_RequestedState(w, LMI_LANEndpoint_RequestedState_Unknown);
            break;
    }

    switch (port_get_enabled_state(port)) {
        case STATE_ENABLED:
            LMI_LANEndpoint_Set_EnabledState(w, LMI_LANEndpoint_EnabledState_Enabled);
            break;
        case STATE_DISABLED:
            LMI_LANEndpoint_Set_EnabledState(w, LMI_LANEndpoint_EnabledState_Disabled);
            break;
        case STATE_ENABLED_BUT_OFFLINE:
            LMI_LANEndpoint_Set_EnabledState(w, LMI_LANEndpoint_EnabledState_Enabled_but_Offline);
            break;
        default:
            LMI_LANEndpoint_Set_EnabledState(w, LMI_LANEndpoint_EnabledState_Unknown);
    }
    return res;
}

CMPIStatus port_to_EthernetPort(
    const Port *port,
    LMI_EthernetPort *w,
    const CMPIContext *cc)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_EthernetPort_Set_SystemName(w, lmi_get_system_name_safe(cc));
    LMI_EthernetPort_Set_CreationClassName(w, LMI_EthernetPort_ClassName);
    LMI_EthernetPort_Set_SystemCreationClassName(w, get_system_creation_class_name());
    LMI_EthernetPort_Set_DeviceID(w, port_get_id(port));

    LMI_EthernetPort_Set_ElementName(w, port_get_id(port));
    LMI_EthernetPort_Set_Name(w, port_get_id(port));

    // MAC and Permanent MAC addresses
    const char *permmac = port_get_permanent_mac(port);
    const char *mac = port_get_mac(port);
    if (permmac != NULL) {
        LMI_EthernetPort_Set_PermanentAddress(w, permmac);
        if (mac != NULL && strcmp(mac, permmac) != 0) {
            // We have both MAC and Permanent MAC address and they're different
            LMI_EthernetPort_Init_NetworkAddresses(w, 2);
            LMI_EthernetPort_Set_NetworkAddresses(w, 0, format_mac(permmac));
            LMI_EthernetPort_Set_NetworkAddresses(w, 1, format_mac(mac));
        } else {
            // Only Permanent MAC exists, or they're same
            LMI_EthernetPort_Init_NetworkAddresses(w, 1);
            LMI_EthernetPort_Set_NetworkAddresses(w, 0, format_mac(permmac));
        }
    } else {
        // Only MAC exists
        LMI_EthernetPort_Init_NetworkAddresses(w, 1);
        LMI_EthernetPort_Set_NetworkAddresses(w, 0, format_mac(mac));
    }
    switch (port_get_type(port)) {
        case TYPE_ETHERNET:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Ethernet);
            break;
        case TYPE_WIFI:
        case TYPE_WIMAX:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Wireless_LAN);
            break;
        case TYPE_BT:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_BlueTooth);
            break;
        case TYPE_OLPC_MESH:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Other);
            LMI_EthernetPort_Set_OtherLinkTechnology(w, "OLPC Mesh");
            break;
        case TYPE_MODEM:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Other);
            LMI_EthernetPort_Set_OtherLinkTechnology(w, "Modem");
            break;
        case TYPE_INFINIBAND:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_IB);
            break;
        case TYPE_BOND:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Other);
            LMI_EthernetPort_Set_OtherLinkTechnology(w, "Bonding");
            break;
        case TYPE_VLAN:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Other);
            LMI_EthernetPort_Set_OtherLinkTechnology(w, "VLAN");
            break;
        case TYPE_ADSL:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Other);
            LMI_EthernetPort_Set_OtherLinkTechnology(w, "ADSL");
            break;
        default:
            LMI_EthernetPort_Set_LinkTechnology(w, LMI_EthernetPort_LinkTechnology_Unknown);
    }
    LMI_EthernetPort_Set_RequestedState(w, LMI_EthernetPort_RequestedState_Not_Applicable);
    uint64_t speed = port_get_speed(port);
    if (speed > 0) {
        // Convert from Mbps to bps
        LMI_EthernetPort_Set_MaxSpeed(w, speed * 1024 * 1024);
    }
    return res;
}

CMPIStatus IPNetworkConnectionEnumInstances(
    IPNetworkConnectionTypes type,
    Network *network,
    const CMPIResult *cr,
    const CMPIBroker *cb,
    const CMPIContext *cc,
    const char *ns)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Port *port;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);

        if (type == LMI_IPNetworkConnection_Type) {
            LMI_IPNetworkConnection w;
            LMI_IPNetworkConnection_Init(&w, cb, ns);

            res = port_to_IPNetworkConnection(port, &w, cc);
            if (!KOkay(res)) {
                error("Unable to convert connection to "
                      LMI_IPNetworkConnection_ClassName
                      ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class "
                      LMI_IPNetworkConnection_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        } else if (type == LMI_LANEndpoint_Type) {
            LMI_LANEndpoint w;
            LMI_LANEndpoint_Init(&w, cb, ns);
            res = port_to_LANEndpoint(port, &w, cc);
            if (!KOkay(res)) {
                error("Unable to convert connection to "
                      LMI_LANEndpoint_ClassName
                      ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class "
                      LMI_LANEndpoint_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        } else if (type == LMI_EthernetPort_Type) {
            LMI_EthernetPort w;
            LMI_EthernetPort_Init(&w, cb, ns);

            res = port_to_EthernetPort(port, &w, cc);
            if (!KOkay(res)) {
                error("Unable to convert connection to "
                      LMI_EthernetPort_ClassName
                      ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class "
                      LMI_EthernetPort_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}
