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

#include <konkret/konkret.h>
#include "LMI_IPNetworkConnectionCapabilities.h"
#include "LMI_BondingMasterSettingData.h"
#include "LMI_BondingSlaveSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "LMI_BridgingSlaveSettingData.h"
#include "LMI_IPAssignmentSettingData.h"
#include "globals.h"
#include "network.h"
#include "port.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IPNetworkConnectionCapabilitiesInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);
    char *instanceid;

    Port *port;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);

        LMI_IPNetworkConnectionCapabilities w;
        LMI_IPNetworkConnectionCapabilities_Init(&w, _cb, ns);

        instanceid = id_to_instanceid(port_get_id(port),
                LMI_IPNetworkConnectionCapabilities_ClassName);
        LMI_IPNetworkConnectionCapabilities_Set_InstanceID(&w, instanceid);
        free(instanceid);

        LMI_IPNetworkConnectionCapabilities_Set_ElementName(&w, port_get_id(port));
        LMI_IPNetworkConnectionCapabilities_Set_ElementNameEditSupported(&w, false);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class "
                    LMI_IPNetworkConnectionCapabilities_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_IPNetworkConnectionCapabilities,
    LMI_IPNetworkConnectionCapabilities,
    _cb,
    LMI_IPNetworkConnectionCapabilitiesInitialize(&mi, ctx))

static void LMI_IPNetworkConnectionCapabilitiesMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPNetworkConnectionCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_IPNetworkConnectionCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_IPNetworkConnectionCapabilities,
    LMI_IPNetworkConnectionCapabilities,
    _cb,
    LMI_IPNetworkConnectionCapabilitiesMethodInitialize(&mi, ctx))

KUint16 LMI_IPNetworkConnectionCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPNetworkConnectionCapabilitiesRef* self,
    const KInstanceA* TemplateGoalSettings,
    KInstanceA* SupportedGoalSettings,
    CMPIStatus* status)
{
    KUint16 result = KUINT16_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

static int get_device_id(Network *network, const char *prefix)
{
    const Ports *ports = network_get_ports(network);
    const char *name;
    int max = -1, id, len = strlen(prefix);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        name = port_get_id(ports_index(ports, i));
        if (strncmp(name, prefix, len) == 0) {
            id = atoi(name + 4);
            if (id > max) {
                max = id;
            }
        }
    }
    return max;
}

static int get_bond_id(Network *network)
{
    return get_device_id(network, "bond");
}

static int get_bridge_id(Network *network)
{
    return get_device_id(network, "bridge");
}

static LMIResult enslave(Network *network, Connection *master_connection, Port *port, char **slave_id)
{
    LMIResult res;
    // find number of slaves of master_connection
    const Connections *connections = network_get_connections(network);
    const char *master_id = connection_get_id(master_connection);
    size_t count = 0;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        if (strcmp(connection_get_id(connections_index(connections, i)), master_id) ==0) {
            count++;
        }
    }
    char *name;
    if (asprintf(&name, "%s Slave %ld", connection_get_name(master_connection), count + 1) < 0) {
        res = LMI_ERROR_MEMORY;
        return res;
    }
    Connection *connection = connection_new(network, NULL, name);
    free(name);
    if (connection == NULL) {
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    if ((res = connection_set_type(connection, CONNECTION_TYPE_ETHERNET)) != LMI_SUCCESS) {
        goto err;
    }
    if ((res = connection_set_port(connection, port)) != LMI_SUCCESS) {
        goto err;
    }
    switch (connection_get_type(master_connection)) {
        case CONNECTION_TYPE_BOND:
            if ((res = connection_set_master_connection(connection, master_connection, SETTING_TYPE_BOND)) != LMI_SUCCESS) {
                goto err;
            }
            break;
        case CONNECTION_TYPE_BRIDGE:
            if ((res = connection_set_master_connection(connection, master_connection, SETTING_TYPE_BRIDGE)) != LMI_SUCCESS) {
                goto err;
            }
            break;
        default:
            error("Connection with type %d can't be master connection", connection_get_type(master_connection));
            res = LMI_WRONG_PARAMETER;
            goto err;
            break;
    }
    res = network_create_connection(network, connection);
    if (slave_id != NULL) {
        if ((*slave_id = strdup(connection_get_id(connection))) == NULL) {
            res = LMI_ERROR_MEMORY;
        }
    }
err:
    connection_free(connection);
    return res;
}

KUint16 LMI_IPNetworkConnectionCapabilities_LMI_CreateIPSetting(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPNetworkConnectionCapabilitiesRef* self,
    const KString* Caption,
    const KUint16* Type,
    const KUint16* IPv4Type,
    const KUint16* IPv6Type,
    KRef* SettingData,
    CMPIStatus* status)
{
    debug("CreateIPSetting(Caption=\"%s\", Type=%d, IPv4Type=%d, IPv6Type=%d)",
          Caption->chars, Type->value, IPv4Type->value, IPv6Type->value);
    Network *network = mi->hdl;

    KUint16 result = KUINT16_INIT;

    int type = CONNECTION_TYPE_ETHERNET;
    if (Type->exists && !Type->null) {
        if (Type->value != CONNECTION_TYPE_ETHERNET &&
            Type->value != CONNECTION_TYPE_BOND &&
            Type->value != CONNECTION_TYPE_BRIDGE) {

            error("Type parameter has invalid value %d", Type->value);
            KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Type parameter has invalid value");
            return result;
        } else {
            type = Type->value;
        }
    }

    int ipv4type = SETTING_METHOD_DISABLED;
    if (IPv4Type->exists && !IPv4Type->null) {
        if (IPv4Type->value != SETTING_METHOD_DISABLED &&
            IPv4Type->value != SETTING_METHOD_STATIC &&
            IPv4Type->value != SETTING_METHOD_DHCP) {

            error("IPv4Type parameter has invalid value %d", IPv4Type->value);
            KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "IPv4Type parameter has invalid value");
            return result;
        } else {
            ipv4type = IPv4Type->value;
        }
    }

    int ipv6type = SETTING_METHOD_DISABLED;
    if (IPv6Type->exists && !IPv6Type->null) {
        if (IPv6Type->value != SETTING_METHOD_DISABLED &&
            IPv6Type->value != SETTING_METHOD_STATIC &&
            IPv6Type->value != SETTING_METHOD_DHCPv6 &&
            IPv6Type->value != SETTING_METHOD_STATELESS) {

            error("IPv6Type parameter has invalid value %d", IPv6Type->value);
            KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "IPv6Type parameter has invalid value");
            return result;
        } else {
            ipv6type = IPv6Type->value;
        }
    }

    if (ipv4type == SETTING_METHOD_DISABLED && ipv6type == SETTING_METHOD_DISABLED) {
        error("Can't create SettingData with both IPv4 and IPv6 disabled");
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Can't create SettingData with both IPv4 and IPv6 disabled");
        return result;
    }

    char *id = id_from_instanceid(self->InstanceID.chars, LMI_IPNetworkConnectionCapabilities_ClassName);
    if (id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, LMI_IPNetworkConnectionCapabilities_ClassName " has wrong InstanceID");
        return result;
    }

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port = NULL;
    const char *port_id;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port_id = port_get_id(ports_index(ports, i));
        if (port_id == NULL) {
            continue;
        }
        if (strcmp(port_id, id) == 0) {
            port = ports_index(ports, i);
            break;
        }
    }
    free(id);
    if (port == NULL) {
        network_unlock(network);
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Associated port doesn't exist");
        return result;
    }

    const char *caption = NULL;
    if (Caption->exists && !Caption->null) {
        caption = Caption->chars;
    }

    LMIResult res;
    Connection *connection = connection_new(network, NULL, caption);
    if (connection == NULL) {
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    if ((res = connection_set_port(connection, port)) != LMI_SUCCESS) {
        goto err;
    }

    if ((res = connection_set_type(connection, type)) != LMI_SUCCESS) {
        goto err;
    }
    Setting *setting;
    char *name;
    switch (type) {
        case CONNECTION_TYPE_BOND:
            if ((setting = setting_new(SETTING_TYPE_BOND)) == NULL) {
                res = LMI_ERROR_MEMORY;
                goto err;
            }
            if (asprintf(&name, "bond%d", get_bond_id(network) + 1) < 0) {
                res = LMI_ERROR_MEMORY;
                setting_free(setting);
                goto err;
            }
            setting_get_bond_setting(setting)->interface_name = name;
            if ((res = connection_add_setting(connection, setting)) != LMI_SUCCESS) {
                setting_free(setting);
                goto err;
            }
            break;

        case CONNECTION_TYPE_BRIDGE:
            setting = setting_new(SETTING_TYPE_BRIDGE);
            if (setting == NULL) {
                res = LMI_ERROR_MEMORY;
                goto err;
            }
            if (asprintf(&name, "bridge%d", get_bridge_id(network) + 1) < 0) {
                res = LMI_ERROR_MEMORY;
                setting_free(setting);
                goto err;
            }
            setting_get_bridge_setting(setting)->interface_name = name;
            if ((res = connection_add_setting(connection, setting)) != LMI_SUCCESS) {
                setting_free(setting);
                goto err;
            }
            break;
        default:
            break;
    }

    // IPv4
    setting = setting_new(SETTING_TYPE_IPv4);
    if (setting == NULL) {
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    if ((res = setting_set_method(setting, ipv4type)) != LMI_SUCCESS) {
        goto err;
    }
    if (ipv4type == SETTING_METHOD_STATIC) {
        // Static connection needs to have at least one address
        if ((res = setting_add_ip_address(setting,
            SETTING_METHOD_STATIC, "0.0.0.1", 24, NULL)) != LMI_SUCCESS) {

            setting_free(setting);
            goto err;
        }
    }
    if ((res = connection_add_setting(connection, setting)) != LMI_SUCCESS) {
        setting_free(setting);
        goto err;
    }

    // IPv6
    setting = setting_new(SETTING_TYPE_IPv6);
    if (setting == NULL) {
        res = LMI_ERROR_MEMORY;
        goto err;
    }

    if ((res = setting_set_method(setting, ipv6type)) != LMI_SUCCESS) {
        setting_free(setting);
        goto err;
    }
    if (ipv6type == SETTING_METHOD_STATIC) {
        // Static connection needs to have at least one address
        if ((res = setting_add_ip_address(setting,
            SETTING_METHOD_STATIC, "1::1", 24, NULL)) != LMI_SUCCESS) {

            setting_free(setting);
            goto err;
        }
    }
    if ((res = connection_add_setting(connection, setting)) != LMI_SUCCESS) {
        setting_free(setting);
        goto err;
    }

    if ((res = network_create_connection(network, connection)) != LMI_SUCCESS) {
        goto err;
    }

    if (type == CONNECTION_TYPE_BOND || type == CONNECTION_TYPE_BRIDGE) {
        if ((res = enslave(network, connection, port, NULL)) != LMI_SUCCESS) {
            goto err;
        }
    }

    // Return reference to LMI_IPAssignmentSettingData
    const char *ns = KNameSpace(LMI_IPNetworkConnectionCapabilitiesRef_ToObjectPath(self, NULL));
    LMI_IPAssignmentSettingDataRef ref;
    LMI_IPAssignmentSettingDataRef_Init(&ref, _cb, ns);
    char *instanceid;
    const char *classname;
    switch (connection_get_type(connection)) {
        case CONNECTION_TYPE_BOND:
            classname = LMI_BondingMasterSettingData_ClassName;
            break;
        case CONNECTION_TYPE_BRIDGE:
            classname = LMI_BridgingMasterSettingData_ClassName;
            break;
        default:
            classname = LMI_IPAssignmentSettingData_ClassName;
            break;
    }
    instanceid = id_to_instanceid(connection_get_id(connection), classname);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    LMI_IPAssignmentSettingDataRef_Set_InstanceID(&ref, instanceid);
    free(instanceid);

    CMPIObjectPath *cop = LMI_IPAssignmentSettingDataRef_ToObjectPath(&ref, NULL);
    CMSetClassName(cop, classname);
    KRef_SetObjectPath(SettingData, cop);

err:
    connection_free(connection);
    network_unlock(network);
    KUint16_Set(&result, (int) res); // TODO: convert error to string and return it
    return result;
}

KUint16 LMI_IPNetworkConnectionCapabilities_LMI_CreateSlaveSetting(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPNetworkConnectionCapabilitiesRef* self,
    const KRef* MasterSettingData,
    KRef* SettingData,
    CMPIStatus* status)
{
    debug("LMI_CreateSlaveSetting");
    Network *network = mi->hdl;

    KUint16 result = KUINT16_INIT;

    // Check MasterSetttingData parameter
    Require(MasterSettingData, "SettingData parameter must be present", result, CMPI_RC_ERR_INVALID_PARAMETER);
    LMI_IPAssignmentSettingDataRef masterSettingDataRef;
    if (!KOkay(LMI_IPAssignmentSettingDataRef_InitFromObjectPath(&masterSettingDataRef, _cb, MasterSettingData->value))) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Unable to use parameter MasterSettingData");
        return result;
    }
    char *connection_id;
    connection_id = id_from_instanceid(masterSettingDataRef.InstanceID.chars, LMI_BondingMasterSettingData_ClassName);
    if (connection_id == NULL) {
        connection_id = id_from_instanceid(masterSettingDataRef.InstanceID.chars, LMI_BridgingMasterSettingData_ClassName);
    }
    if (connection_id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "MasterSettingData has wrong InstanceID");
        return result;
    }

    // Check associated port
    char *id = id_from_instanceid(self->InstanceID.chars, LMI_IPNetworkConnectionCapabilities_ClassName);
    if (id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, LMI_IPNetworkConnectionCapabilities_ClassName " has wrong InstanceID");
        free(connection_id);
        return result;
    }
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port = NULL;
    const char *port_id;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port_id = port_get_id(ports_index(ports, i));
        if (port_id == NULL) {
            continue;
        }
        if (strcmp(port_id, id) == 0) {
            port = ports_index(ports, i);
            break;
        }
    }
    free(id);
    if (port == NULL) {
        network_unlock(network);
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Associated port doesn't exist");
        free(connection_id);
        return result;
    }

    Connection *master_connection = connections_find_by_id(network_get_connections(network), connection_id);
    free(connection_id);
    if (master_connection == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "MasterSettingData doesn't exist");
        network_unlock(network);
        return result;
    }

    char *slave_id = NULL;
    LMIResult res = enslave(network, master_connection, port, &slave_id);
    if (res != LMI_SUCCESS) {
        free(slave_id);
        network_unlock(network);
        KUint16_Set(&result, (int) res); // TODO: convert error to string and return it
        return result;
    }

    // Return reference to LMI_IPAssignmentSettingData
    const char *ns = KNameSpace(LMI_IPNetworkConnectionCapabilitiesRef_ToObjectPath(self, NULL));

    const char *classname;
    switch (connection_get_type(master_connection)) {
        case CONNECTION_TYPE_BRIDGE:
            classname = LMI_BridgingSlaveSettingData_ClassName;
            break;
        case CONNECTION_TYPE_BOND:
            classname = LMI_BondingSlaveSettingData_ClassName;
            break;
        default:
            classname = LMI_IPAssignmentSettingData_ClassName;
            break;
    }

    LMI_IPAssignmentSettingDataRef ref;
    LMI_IPAssignmentSettingDataRef_Init(&ref, _cb, ns);
    char *instanceid = id_to_instanceid(slave_id, classname);
    free(slave_id);
    if (instanceid == NULL) {
        error("Memory allocation failed");
        res = LMI_ERROR_MEMORY;
    } else {
        LMI_IPAssignmentSettingDataRef_Set_InstanceID(&ref, instanceid);
        free(instanceid);

        CMPIObjectPath *cop = LMI_IPAssignmentSettingDataRef_ToObjectPath(&ref, NULL);
        CMSetClassName(cop, classname);
        KRef_SetObjectPath(SettingData, cop);
    }

    network_unlock(network);
    KUint16_Set(&result, (int) res); // TODO: convert error to string and return it
    return result;

}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPNetworkConnectionCapabilities",
    "LMI_IPNetworkConnectionCapabilities",
    "instance method")
