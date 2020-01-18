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

#include "ipassignmentsettingdata.h"
#include "connection.h"
#include "setting.h"
#include <string.h>

#include "LMI_IPAssignmentSettingData.h"
#include "LMI_IPRouteSettingData.h"
#include "LMI_ExtendedStaticIPAssignmentSettingData.h"
#include "LMI_DHCPSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "LMI_BondingMasterSettingData.h"
#include "LMI_BridgingSlaveSettingData.h"
#include "LMI_BondingSlaveSettingData.h"

CMPIStatus connection_to_IPAssignmentSettingData(
    const Connection *connection,
    LMI_IPAssignmentSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_IPAssignmentSettingData_Set_Caption(w, connection_get_name(connection));
    char *instanceid = id_to_instanceid(connection_get_id(connection), LMI_IPAssignmentSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_IPAssignmentSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);

    // Get IPv4 and IPv6 method, default is disabled
    LMI_IPAssignmentSettingData_Set_IPv4Type(w, LMI_IPAssignmentSettingData_IPv4Type_Disabled);
    LMI_IPAssignmentSettingData_Set_IPv6Type(w, LMI_IPAssignmentSettingData_IPv6Type_Disabled);
    const Settings *settings = connection_get_settings(connection);
    Setting *setting;
    for (size_t i = 0; i < settings_length(settings); ++i) {
        setting = settings_index(settings, i);
        if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_IPAssignmentSettingData_Set_IPv4Type(w,
                            LMI_IPAssignmentSettingData_IPv4Type_Static);
                    break;
                case SETTING_METHOD_DHCP:
                    LMI_IPAssignmentSettingData_Set_IPv4Type(w,
                            LMI_IPAssignmentSettingData_IPv4Type_DHCP);
                    break;
                default:
                    break;
            }
        } else if (setting_get_type(setting) == SETTING_TYPE_IPv6) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_IPAssignmentSettingData_Set_IPv6Type(w,
                            LMI_IPAssignmentSettingData_IPv6Type_Static);
                    break;
                case SETTING_METHOD_STATELESS:
                    LMI_IPAssignmentSettingData_Set_IPv6Type(w,
                            LMI_IPAssignmentSettingData_IPv6Type_Stateless);
                    break;
                case SETTING_METHOD_DHCPv6:
                    LMI_IPAssignmentSettingData_Set_IPv6Type(w,
                            LMI_IPAssignmentSettingData_IPv6Type_DHCPv6);
                    break;
                default:
                    break;
            }
        }
    }

    LMI_IPAssignmentSettingData_Set_AddressOrigin(w,
            LMI_IPAssignmentSettingData_AddressOrigin_cumulative_configuration);
    LMI_IPAssignmentSettingData_Set_ProtocolIFType(w,
            LMI_IPAssignmentSettingData_ProtocolIFType_Both_IPv4_and_IPv6);
    return res;
}

CMPIStatus connection_to_BridgingMasterSettingData(
    const Connection *connection,
    LMI_BridgingMasterSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_BridgingMasterSettingData_Set_Caption(w, connection_get_name(connection));
    char *instanceid = id_to_instanceid(connection_get_id(connection), LMI_BridgingMasterSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_BridgingMasterSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    LMI_BridgingMasterSettingData_Set_AddressOrigin(w,
            LMI_IPAssignmentSettingData_AddressOrigin_cumulative_configuration);
    LMI_BridgingMasterSettingData_Set_ProtocolIFType(w,
            LMI_IPAssignmentSettingData_ProtocolIFType_Both_IPv4_and_IPv6);

    // Get IPv4 and IPv6 method, default is disabled
    LMI_BridgingMasterSettingData_Set_IPv4Type(w, LMI_BridgingMasterSettingData_IPv4Type_Disabled);
    LMI_BridgingMasterSettingData_Set_IPv6Type(w, LMI_BridgingMasterSettingData_IPv6Type_Disabled);
    const Settings *settings = connection_get_settings(connection);
    Setting *setting, *setting_bridge = NULL;
    for (size_t i = 0; i < settings_length(settings); ++i) {
        setting = settings_index(settings, i);
        if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_BridgingMasterSettingData_Set_IPv4Type(w,
                            LMI_BridgingMasterSettingData_IPv4Type_Static);
                    break;
                case SETTING_METHOD_DHCP:
                    LMI_BridgingMasterSettingData_Set_IPv4Type(w,
                            LMI_BridgingMasterSettingData_IPv4Type_DHCP);
                    break;
                default:
                    break;
            }
        } else if (setting_get_type(setting) == SETTING_TYPE_IPv6) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_BridgingMasterSettingData_Set_IPv6Type(w,
                            LMI_BridgingMasterSettingData_IPv6Type_Static);
                    break;
                case SETTING_METHOD_STATELESS:
                    LMI_BridgingMasterSettingData_Set_IPv6Type(w,
                            LMI_BridgingMasterSettingData_IPv6Type_Stateless);
                    break;
                case SETTING_METHOD_DHCP:
                    LMI_BridgingMasterSettingData_Set_IPv6Type(w,
                            LMI_BridgingMasterSettingData_IPv6Type_DHCPv6);
                    break;
                default:
                    break;
            }
        } else if (setting_get_type(setting) == SETTING_TYPE_BRIDGE) {
            setting_bridge = setting;
        }
    }
    if (setting_bridge == NULL) {
        error("Bridge connection has no bridge setting");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    BridgeSetting *bridge_setting = setting_get_bridge_setting(setting_bridge);
    LMI_BridgingMasterSettingData_Set_InterfaceName(w,
            bridge_setting->interface_name);
    LMI_BridgingMasterSettingData_Set_STP(w,
            bridge_setting->stp);
    LMI_BridgingMasterSettingData_Set_Priority(w,
            bridge_setting->priority);
    LMI_BridgingMasterSettingData_Set_ForwardDelay(w,
            bridge_setting->forward_delay);
    LMI_BridgingMasterSettingData_Set_HelloTime(w,
            bridge_setting->hello_time);
    LMI_BridgingMasterSettingData_Set_MaxAge(w,
            bridge_setting->max_age);
    LMI_BridgingMasterSettingData_Set_AgeingTime(w,
            bridge_setting->ageing_time);
    return res;
}

CMPIStatus connection_to_BridgingSlaveSettingData(
    const Connection *connection,
    LMI_BridgingSlaveSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_BridgingSlaveSettingData_Set_Caption(w, connection_get_name(connection));
    char *instanceid = id_to_instanceid(connection_get_id(connection), LMI_BridgingSlaveSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_BridgingSlaveSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    LMI_BridgingSlaveSettingData_Set_AddressOrigin(w,
            LMI_IPAssignmentSettingData_AddressOrigin_cumulative_configuration);
    LMI_BridgingSlaveSettingData_Set_ProtocolIFType(w,
            LMI_IPAssignmentSettingData_ProtocolIFType_Both_IPv4_and_IPv6);

    Setting *setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_BRIDGE_SLAVE);
    if (setting != NULL) {
        BridgeSlaveSetting *bridge_setting = setting_get_bridge_slave_setting(setting);
        LMI_BridgingSlaveSettingData_Set_Priority(w,
                bridge_setting->priority);
        LMI_BridgingSlaveSettingData_Set_PathCost(w,
                bridge_setting->path_cost);
        LMI_BridgingSlaveSettingData_Set_HairpinMode(w,
                bridge_setting->hairpin_mode);
    }
    return res;
}

CMPIStatus connection_to_BondingMasterSettingData(
    const Connection *connection,
    LMI_BondingMasterSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_BondingMasterSettingData_Set_Caption(w, connection_get_name(connection));
    char *instanceid = id_to_instanceid(connection_get_id(connection), LMI_BondingMasterSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_BondingMasterSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    LMI_BondingMasterSettingData_Set_AddressOrigin(w,
            LMI_IPAssignmentSettingData_AddressOrigin_cumulative_configuration);
    LMI_BondingMasterSettingData_Set_ProtocolIFType(w,
            LMI_IPAssignmentSettingData_ProtocolIFType_Both_IPv4_and_IPv6);

    // Get IPv4 and IPv6 method, default is disabled
    LMI_BondingMasterSettingData_Set_IPv4Type(w, LMI_BondingMasterSettingData_IPv4Type_Disabled);
    LMI_BondingMasterSettingData_Set_IPv6Type(w, LMI_BondingMasterSettingData_IPv6Type_Disabled);
    const Settings *settings = connection_get_settings(connection);
    Setting *setting, *setting_bond = NULL;
    for (size_t i = 0; i < settings_length(settings); ++i) {
        setting = settings_index(settings, i);
        if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_BondingMasterSettingData_Set_IPv4Type(w,
                            LMI_BondingMasterSettingData_IPv4Type_Static);
                    break;
                case SETTING_METHOD_DHCP:
                    LMI_BondingMasterSettingData_Set_IPv4Type(w,
                            LMI_BondingMasterSettingData_IPv4Type_DHCP);
                    break;
                default:
                    break;
            }
        } else if (setting_get_type(setting) == SETTING_TYPE_IPv6) {
            switch (setting_get_method(setting)) {
                case SETTING_METHOD_STATIC:
                case SETTING_METHOD_LINK_LOCAL:
                    LMI_BondingMasterSettingData_Set_IPv6Type(w,
                            LMI_BondingMasterSettingData_IPv6Type_Static);
                    break;
                case SETTING_METHOD_STATELESS:
                    LMI_BondingMasterSettingData_Set_IPv6Type(w,
                            LMI_BondingMasterSettingData_IPv6Type_Stateless);
                    break;
                case SETTING_METHOD_DHCP:
                    LMI_BondingMasterSettingData_Set_IPv6Type(w,
                            LMI_BondingMasterSettingData_IPv6Type_DHCPv6);
                    break;
                default:
                    break;
            }
        } else if (setting_get_type(setting) == SETTING_TYPE_BOND) {
            setting_bond = setting;
        }
    }

    if (setting_bond == NULL) {
        error("Connection of type Bond doesn't have bond setting");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_BondingMasterSettingData_Set_InterfaceName(w, setting_get_bond_setting(setting_bond)->interface_name);

    BondSetting *bond = setting_get_bond_setting(setting_bond);
    LMI_BondingMasterSettingData_Set_Mode(w, bond->mode);
    LMI_BondingMasterSettingData_Set_MIIMon(w, bond->miimon);
    LMI_BondingMasterSettingData_Set_DownDelay(w, bond->downdelay);
    LMI_BondingMasterSettingData_Set_UpDelay(w, bond->updelay);
    LMI_BondingMasterSettingData_Set_ARPInterval(w, bond->arp_interval);
    LMI_BondingMasterSettingData_Init_ARPIPTarget(w, ip_addresses_length(bond->arp_ip_target));
    for (size_t i = 0; i < ip_addresses_length(bond->arp_ip_target); ++i) {
        LMI_BondingMasterSettingData_Set_ARPIPTarget(w, i, ip_addresses_index(bond->arp_ip_target, i));
    }
    return res;
}

CMPIStatus connection_to_BondingSlaveSettingData(
    const Connection *connection,
    LMI_BondingSlaveSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_BondingSlaveSettingData_Set_Caption(w, connection_get_name(connection));
    char *instanceid = id_to_instanceid(connection_get_id(connection), LMI_BondingSlaveSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_BondingSlaveSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    LMI_BondingSlaveSettingData_Set_AddressOrigin(w,
            LMI_IPAssignmentSettingData_AddressOrigin_cumulative_configuration);
    LMI_BondingSlaveSettingData_Set_ProtocolIFType(w,
            LMI_IPAssignmentSettingData_ProtocolIFType_Both_IPv4_and_IPv6);
    return res;
}

CMPIStatus setting_to_ExtendedStaticIPAssignmentSettingData(
    const Setting *setting,
    LMI_ExtendedStaticIPAssignmentSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_ExtendedStaticIPAssignmentSettingData_Set_Caption(w, setting_get_caption(setting));
    char *instanceid = id_to_instanceid(setting_get_id(setting), LMI_ExtendedStaticIPAssignmentSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_ExtendedStaticIPAssignmentSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    LMI_ExtendedStaticIPAssignmentSettingData_Set_ElementName(w, setting_get_caption(setting));

    LMI_ExtendedStaticIPAssignmentSettingData_Set_ProtocolIFType(w,
            setting_get_type(setting) == SETTING_TYPE_IPv4 ?
            LMI_ExtendedStaticIPAssignmentSettingData_ProtocolIFType_IPv4 :
            LMI_ExtendedStaticIPAssignmentSettingData_ProtocolIFType_IPv6);
    Addresses *addresses = setting_get_addresses(setting);

    size_t count = addresses_length(addresses);
    LMI_ExtendedStaticIPAssignmentSettingData_Init_IPAddresses(w, count);
    if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
        LMI_ExtendedStaticIPAssignmentSettingData_Init_SubnetMasks(w, count);
    } else {
        LMI_ExtendedStaticIPAssignmentSettingData_Init_IPv6SubnetPrefixLengths(w, count);
    }
    LMI_ExtendedStaticIPAssignmentSettingData_Init_GatewayAddresses(w, count);

    Address *address;
    for (size_t k = 0; k < count; ++k) {
        address = addresses_index(addresses, k);
        LMI_ExtendedStaticIPAssignmentSettingData_Set_IPAddresses(w, k, address->addr);
        if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
            LMI_ExtendedStaticIPAssignmentSettingData_Set_SubnetMasks(w, k, prefixToMask4(address->prefix));
        } else {
            LMI_ExtendedStaticIPAssignmentSettingData_Set_IPv6SubnetPrefixLengths(w, k, address->prefix);
        }
        if (address->default_gateway != NULL) {
            LMI_ExtendedStaticIPAssignmentSettingData_Set_GatewayAddresses(w, k, address->default_gateway);
        } else {
            LMI_ExtendedStaticIPAssignmentSettingData_Null_GatewayAddresses(w, k);
        }
    }
    return res;
}

CMPIStatus setting_to_DHCPSettingData(
    const Setting *setting,
    LMI_DHCPSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    // AddressOrigin has the same number as SettingMethod
    LMI_DHCPSettingData_Set_AddressOrigin(w, setting_get_method(setting));
    LMI_DHCPSettingData_Set_Caption(w, setting_get_caption(setting));
    LMI_DHCPSettingData_Set_ProtocolIFType(w,
            setting_get_method(setting) == SETTING_METHOD_DHCP ?
            LMI_DHCPSettingData_ProtocolIFType_IPv4 :
            LMI_DHCPSettingData_ProtocolIFType_IPv6);

    char *instanceid = id_to_instanceid(setting_get_id(setting), LMI_DHCPSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_DHCPSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    return res;
}

CMPIStatus setting_to_IPAssignmentSettingData(
    const Setting *setting,
    LMI_IPAssignmentSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    // AddressOrigin has the same number as SettingMethod
    LMI_IPAssignmentSettingData_Set_AddressOrigin(w, setting_get_method(setting));
    LMI_IPAssignmentSettingData_Set_Caption(w, setting_get_caption(setting));
    LMI_IPAssignmentSettingData_Set_ProtocolIFType(w,
            setting_get_method(setting) == SETTING_METHOD_DHCPv6 ||
            setting_get_method(setting) == SETTING_METHOD_STATELESS ?
            LMI_IPAssignmentSettingData_ProtocolIFType_IPv6 :
            LMI_IPAssignmentSettingData_ProtocolIFType_IPv4);

    char *instanceid = id_to_instanceid(setting_get_id(setting), LMI_IPAssignmentSettingData_ClassName);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_IPAssignmentSettingData_Set_InstanceID(w, instanceid);
    free(instanceid);
    return res;
}

CMPIStatus route_to_IPRouteSettingData(
    const Route *route,
    const char *setting_id,
    size_t route_nr,
    LMI_IPRouteSettingData *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    char *name, *id;
    if (asprintf(&name, "%s_%zu", setting_id, route_nr) < 0) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    id = id_to_instanceid(name, "LMI_IPRouteSettingData");
    if (id == NULL) {
        error("Unable to get ID from InstanceID: %s", name);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_IPRouteSettingData_Set_InstanceID(w, id);
    free(name);
    free(id);

    LMI_IPRouteSettingData_Null_AddressOrigin(w);

    LMI_IPRouteSettingData_Set_DestinationAddress(w, route->route);
    char *mask;
    if (route->type == IPv4) {
        if ((mask = prefixToMask4(route->prefix)) == NULL) {
            error("Memory allocation failed");
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            return res;
        }
        LMI_IPRouteSettingData_Set_DestinationMask(w, mask);
        free(mask);
    } else {
        LMI_IPRouteSettingData_Set_PrefixLength(w, route->prefix);
    }
    LMI_IPRouteSettingData_Set_RouteMetric(w, route->metric);
    LMI_IPRouteSettingData_Set_AddressType(w,
    route->type == IPv4 ?
            LMI_IPRouteSettingData_AddressType_IPv4 :
            LMI_IPRouteSettingData_AddressType_IPv6);
    LMI_IPRouteSettingData_Set_NextHop(w, route->next_hop);
    return res;
}

CMPIStatus IPAssignmentSettingDataEnumInstances(
    IPAssignmentSettingDataTypes type,
    Network *network,
    const CMPIResult* cr,
    const CMPIBroker* cb,
    const char *ns)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    size_t j, k;
    Connection *connection;
    Setting *setting;
    Route *route;

    network_lock(network);

    const Connections *connections = network_get_connections(network);
    for (size_t i = 0; i < connections_length(connections); ++i) {
        connection = connections_index(connections, i);
        Connection *master = connection_get_master_connection(connection);

        if (connection_get_type(connection) == CONNECTION_TYPE_BOND &&
                type == LMI_BondingMasterSettingData_Type) {

            LMI_BondingMasterSettingData w;
            LMI_BondingMasterSettingData_Init(&w, cb, ns);

            res = connection_to_BondingMasterSettingData(connection, &w);
            if (!KOkay(res)) {
                error("Unable to convert connection to "
                      LMI_BondingMasterSettingData_ClassName
                      ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class "
                      LMI_BondingMasterSettingData_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

        } else if (connection_get_type(connection) == CONNECTION_TYPE_BRIDGE &&
                type == LMI_BridgingMasterSettingData_Type) {

            LMI_BridgingMasterSettingData w;
            LMI_BridgingMasterSettingData_Init(&w, cb, ns);

            res = connection_to_BridgingMasterSettingData(connection, &w);
            if (!KOkay(res)) {
                error("Unable to convert connection to "
                      LMI_BridgingMasterSettingData_ClassName
                      ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class "
                      LMI_BridgingMasterSettingData_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        } else if (master != NULL) {
            // This is slave connection
            if (type == LMI_BridgingSlaveSettingData_Type &&
                connection_get_type(master) == CONNECTION_TYPE_BRIDGE) {

                LMI_BridgingSlaveSettingData w;
                LMI_BridgingSlaveSettingData_Init(&w, cb, ns);

                res = connection_to_BridgingSlaveSettingData(connection, &w);
                if (!KOkay(res)) {
                    error("Unable to convert connection to "
                        LMI_BridgingMasterSettingData_ClassName
                        ": %d (%s)", res.rc, KChars(res.msg));
                    break;
                }
                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class "
                        LMI_BridgingSlaveSettingData_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            } else if (type == LMI_BondingSlaveSettingData_Type &&
                connection_get_type(master) == CONNECTION_TYPE_BOND) {

                LMI_BondingSlaveSettingData w;
                LMI_BondingSlaveSettingData_Init(&w, cb, ns);

                res = connection_to_BondingSlaveSettingData(connection, &w);
                if (!KOkay(res)) {
                    error("Unable to convert connection to "
                        LMI_BondingMasterSettingData_ClassName
                        ": %d (%s)", res.rc, KChars(res.msg));
                    break;
                }
                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class "
                        LMI_BondingSlaveSettingData_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
        } else if (type == LMI_IPAssignmentSettingData_Type &&
            connection_get_type(connection) != CONNECTION_TYPE_BRIDGE &&
            connection_get_type(connection) != CONNECTION_TYPE_BOND) {

            // Cumulative IPAssignmentSettingData
            LMI_IPAssignmentSettingData w;
            LMI_IPAssignmentSettingData_Init(&w, cb, ns);

            res = connection_to_IPAssignmentSettingData(connection, &w);
            if (!KOkay(res)) {
                error("Unable to convert connection to " LMI_IPAssignmentSettingData_ClassName ": %d (%s)", res.rc, KChars(res.msg));
                break;
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_ExtendedStaticIPAssignmentSettingData_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }

        for (j = 0; j < settings_length(connection_get_settings(connection)); ++j) {
            setting = settings_index(connection_get_settings(connection), j);

            if (setting_get_type(setting) != SETTING_TYPE_IPv4 &&
                setting_get_type(setting) != SETTING_TYPE_IPv6) {

                continue;
            }

            if (setting_get_method(setting) == SETTING_METHOD_STATIC &&
                type == LMI_ExtendedStaticIPAssignmentSettingData_Type) {

                LMI_ExtendedStaticIPAssignmentSettingData w;
                LMI_ExtendedStaticIPAssignmentSettingData_Init(&w, cb, ns);
                res = setting_to_ExtendedStaticIPAssignmentSettingData(setting, &w);
                if (!KOkay(res)) {
                    error("Unable to convert setting to "
                          LMI_ExtendedStaticIPAssignmentSettingData_ClassName
                          ": %d (%s)", res.rc, KChars(res.msg));
                    break;
                }

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class "
                          LMI_ExtendedStaticIPAssignmentSettingData_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }

            if ((setting_get_method(setting) == SETTING_METHOD_DHCP ||
                 setting_get_method(setting) == SETTING_METHOD_DHCPv6) &&
                type == LMI_DHCPSettingData_Type) {

                LMI_DHCPSettingData w;
                LMI_DHCPSettingData_Init(&w, cb, ns);

                res = setting_to_DHCPSettingData(setting, &w);
                if (!KOkay(res)) {
                    error("Unable to convert setting to "
                          LMI_DHCPSettingData_ClassName
                          ": %d (%s)", res.rc, KChars(res.msg));
                    break;
                }

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class "
                          LMI_DHCPSettingData_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }

            if (setting_get_method(setting) == SETTING_METHOD_STATELESS &&
                type == LMI_IPAssignmentSettingData_Type) {

                LMI_IPAssignmentSettingData w;
                LMI_IPAssignmentSettingData_Init(&w, cb, ns);

                res = setting_to_IPAssignmentSettingData(setting, &w);
                if (!KOkay(res)) {
                    error("Unable to convert setting to "
                          LMI_IPAssignmentSettingData_ClassName
                          ": %d (%s)", res.rc, KChars(res.msg));
                    break;
                }


                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_IPAssignmentSettingData_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }

            if (type == LMI_IPRouteSettingData_Type) {
                LMI_IPRouteSettingData w;
                LMI_IPRouteSettingData_Init(&w, cb, ns);

                for (k = 0; k < setting_get_routes_length(setting); ++k) {
                    route = setting_get_route(setting, k);

                    res = route_to_IPRouteSettingData(route, setting_get_id(setting), k, &w);
                    if (!KOkay(res)) {
                        error("Unable to convert route to "
                            LMI_IPRouteSettingData_ClassName
                            ": %d (%s)", res.rc, KChars(res.msg));
                        break;
                    }

                    if (!ReturnInstance(cr, w)) {
                        error("Unable to return instance of class " LMI_IPRouteSettingData_ClassName);
                        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                        break;
                    }
                }
            }
        }
    }

    network_unlock(network);
    return res;
}

CMPIStatus IPAssignmentSettingDataDeleteInstance(
    Network *network,
    const char *instanceid)
{
    LMIResult res = LMI_SUCCESS;

    char *uuid = strrchr(instanceid, ':');
    if (uuid == NULL) {
        error("Invalid InstanceID: %s", instanceid);
        CMReturn(CMPI_RC_ERR_INVALID_PARAMETER);
    }
    // Skip the semicolon
    uuid++;

    network_lock(network);
    const Connections *connections = network_get_connections(network);
    Connection *connection = connections_find_by_id(connections, uuid), *c;

    if (connection == NULL) {
        network_unlock(network);
        error("No such connection: %s", uuid);
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    // Delete all slave connections first
    Connection *master;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        c = connections_index(connections, i);
        master = connection_get_master_connection(c);
        if (master != NULL && connection_get_id(master) != NULL &&
                strcmp(connection_get_id(master), uuid) == 0) {

            if ((res = network_delete_connection(network, c)) != LMI_SUCCESS) {
                break;
            }
        }
    }
    // Delete (master) connection
    if (res == LMI_SUCCESS) {
        res = network_delete_connection(network, connection);
    }
    network_unlock(network);
    if (res != LMI_SUCCESS) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}
