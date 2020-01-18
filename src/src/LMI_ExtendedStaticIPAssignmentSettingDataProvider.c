/*
 * Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
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
#include "LMI_ExtendedStaticIPAssignmentSettingData.h"
#include "network.h"
#include "ipconfig.h"
#include "connection.h"
#include "setting.h"
#include "ipassignmentsettingdata.h"

static const CMPIBroker* _cb = NULL;

static void LMI_ExtendedStaticIPAssignmentSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_ExtendedStaticIPAssignmentSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus fill_setting_from_instance(
    Setting *setting,
    LMI_ExtendedStaticIPAssignmentSettingData *w,
    const CMPIBroker *cb)
{
    uint16_t prefix;
    setting_set_method(setting, SETTING_METHOD_STATIC);
    if (!w->IPAddresses.exists || w->IPAddresses.null || w->IPAddresses.count == 0) {
        KReturn2(cb, ERR_FAILED, "Property IPAddresses can't be empty");
    }

    if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
        if (!w->SubnetMasks.exists || w->SubnetMasks.null || w->SubnetMasks.count == 0) {
            KReturn2(cb, ERR_FAILED, "Property SubnetMasks can't be empty");
        }
    } else {
        if (!w->IPv6SubnetPrefixLengths.exists || w->IPv6SubnetPrefixLengths.null || w->IPAddresses.count == 0) {
            KReturn2(cb, ERR_FAILED, "Property IPv6SubnetPrefixLengths can't be empty");
        }
    }

    if (w->IPAddresses.count != w->GatewayAddresses.count) {
        KReturn2(_cb, ERR_FAILED, "Number of addresses and gateways must be the same");
    }
    if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
        if (w->IPAddresses.count != w->SubnetMasks.count) {
            KReturn2(_cb, ERR_FAILED, "Number of addresses and subnet masks must be the same");
        }
    } else {
        if (w->IPAddresses.count != w->IPv6SubnetPrefixLengths.count) {
            KReturn2(_cb, ERR_FAILED, "Number of addresses and subnet prefixes must be the same");
        }
    }

    for (CMPIUint32 i = 0; i < w->IPAddresses.count; ++i) {
        if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
            prefix = netmaskToPrefix4(KStringA_Get(&w->SubnetMasks, i));
        } else {
            prefix = KUint16A_Get(&w->IPv6SubnetPrefixLengths, i).value;
        }
        if (setting_add_ip_address(setting,
                SETTING_METHOD_STATIC,
                KStringA_Get(&w->IPAddresses, i),
                prefix,
                KStringA_Get(&w->GatewayAddresses, i)) != LMI_SUCCESS) {

            KReturn2(_cb, ERR_FAILED, "Unable to change IP addresses");
        }
    }
    KReturn(OK);
}


static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_ExtendedStaticIPAssignmentSettingDataRef ref;
    LMI_ExtendedStaticIPAssignmentSettingDataRef_InitFromObjectPath(&ref, _cb, cop);
    size_t index;
    char *connection_id = id_from_instanceid_with_index(ref.InstanceID.chars, LMI_ExtendedStaticIPAssignmentSettingData_ClassName, &index);
    if (connection_id == NULL) {
        KReturn2(_cb, ERR_INVALID_PARAMETER, LMI_ExtendedStaticIPAssignmentSettingData_ClassName " has wrong InstanceID");
    }

    LMI_ExtendedStaticIPAssignmentSettingData w;
    LMI_ExtendedStaticIPAssignmentSettingData_InitFromInstance(&w, _cb, ci);

    if (!w.ProtocolIFType.exists || w.ProtocolIFType.null) {
        free(connection_id);
        KReturn2(_cb, ERR_FAILED, "ProtocolIFType must be specified");
    }

    Network *network = mi->hdl;
    Setting *setting = NULL;

    network_lock(network);
    const Connections *connections = network_get_connections(network);

    Connection *old_connection = connections_find_by_id(connections, connection_id);

    if (old_connection == NULL) {
        error("ModifyInstance on nonexisting connection: %s", connection_id);
        network_unlock(network);
        free(connection_id);
        KReturn2(_cb, ERR_FAILED, "Connection doesn't exist");
    }
    free(connection_id);

    Connection *connection = connection_clone(old_connection);

    if (w.ProtocolIFType.value == LMI_ExtendedStaticIPAssignmentSettingData_ProtocolIFType_IPv4) {
        setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_IPv4);
    } else if (w.ProtocolIFType.value == LMI_ExtendedStaticIPAssignmentSettingData_ProtocolIFType_IPv6) {
        setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_IPv6);
    }
    if (setting == NULL) {
        error("Changing protocol version is not supported");
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_NOT_SUPPORTED, "Changing protocol version is not supported");
    }

    setting_clear_addresses(setting);
    CMPIStatus rc = fill_setting_from_instance(setting, &w, _cb);
    if (KOkay(rc)) {
        rc.rc = connection_update(old_connection, connection);
    }
    connection_free(connection);
    network_unlock(network);
    return rc;
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataExecQuery(
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
    LMI_ExtendedStaticIPAssignmentSettingData,
    LMI_ExtendedStaticIPAssignmentSettingData,
    _cb,
    LMI_ExtendedStaticIPAssignmentSettingDataInitialize(&mi, ctx))

static void LMI_ExtendedStaticIPAssignmentSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_ExtendedStaticIPAssignmentSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_ExtendedStaticIPAssignmentSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_ExtendedStaticIPAssignmentSettingData,
    LMI_ExtendedStaticIPAssignmentSettingData,
    _cb,
    LMI_ExtendedStaticIPAssignmentSettingDataMethodInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_ExtendedStaticIPAssignmentSettingData",
    "LMI_ExtendedStaticIPAssignmentSettingData",
    "instance method")
