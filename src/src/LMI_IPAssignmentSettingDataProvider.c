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
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "LMI_BondingMasterSettingData.h"

#include "network.h"
#include "connection.h"
#include "setting.h"
#include "ipassignmentsettingdata.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IPAssignmentSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPAssignmentSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPAssignmentSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPAssignmentSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_IPAssignmentSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_IPAssignmentSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPAssignmentSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPAssignmentSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_IPAssignmentSettingDataRef ref;
    if (!KOkay(LMI_IPAssignmentSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_IPAssignmentSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }
    LMI_IPAssignmentSettingData w;
    LMI_IPAssignmentSettingData_InitFromInstance(&w, _cb, ci);

    Network *network = mi->hdl;
    char *id = id_from_instanceid(w.InstanceID.chars, LMI_IPAssignmentSettingData_ClassName);

    network_lock(network);
    const Connections *connections = network_get_connections(network);
    Connection *old_connection = connections_find_by_id(connections, id);
    free(id);
    if (old_connection == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "No such connection");
    }

    Connection *connection = connection_clone(old_connection);

    if (w.Caption.exists && !w.Caption.null) {
        connection_set_name(connection, w.Caption.chars);
    }

    // Change IPv4 and IPv6 method
    const Settings *settings = connection_get_settings(connection);
    Setting *setting;
    for (size_t i = 0; i < settings_length(settings); ++i) {
        setting = settings_index(settings, i);
        if (setting_get_type(setting) == SETTING_TYPE_IPv4 && w.IPv4Type.exists && !w.IPv4Type.null) {
            if (w.IPv4Type.value != setting_get_method(setting)) {
                setting_set_method(setting, w.IPv4Type.value);
                if (setting_get_method(setting) == SETTING_METHOD_STATIC) {
                    // We need to have at least one IP address, NM won't create
                    // the connection otherwise
                    setting_add_ip_address(setting,
                            SETTING_METHOD_STATIC, "0.0.0.1", 24, NULL);
                }
            }
        }
        if (setting_get_type(setting) == SETTING_TYPE_IPv6 && w.IPv6Type.exists && !w.IPv6Type.null) {
            if (w.IPv6Type.value != setting_get_method(setting)) {
                setting_set_method(setting, w.IPv6Type.value);
                if (setting_get_method(setting) == SETTING_METHOD_STATIC) {
                    // We need to have at least one IP address, NM won't create
                    // the connection otherwise
                    setting_add_ip_address(setting,
                            SETTING_METHOD_STATIC, "1::1", 24, NULL);
                }
            }
        }
    }

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPAssignmentSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    Network *network = mi->hdl;

    LMI_IPAssignmentSettingData w;
    if (LMI_IPAssignmentSettingData_InitFromObjectPath(&w, _cb, cop).rc != 0) {
        warn("Unable to convert object path to " LMI_IPAssignmentSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    return IPAssignmentSettingDataDeleteInstance(network, w.InstanceID.chars);
}

static CMPIStatus LMI_IPAssignmentSettingDataExecQuery(
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
    LMI_IPAssignmentSettingData,
    LMI_IPAssignmentSettingData,
    _cb,
    LMI_IPAssignmentSettingDataInitialize(&mi, ctx))

static void LMI_IPAssignmentSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}


static CMPIStatus LMI_IPAssignmentSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPAssignmentSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_IPAssignmentSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

KUint32 LMI_IPAssignmentSettingData_LMI_AddStaticIPRoute(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPAssignmentSettingDataRef* self,
    const KUint16* AddressType,
    const KString* DestinationAddress,
    const KString* DestinationMask,
    const KUint8* PrefixLength,
    KRef* Route,
    CMPIStatus* status)
{
    LMIResult res = LMI_SUCCESS;
    Network *network = mi->hdl;
    KUint32 result = KUINT32_INIT;
    const char *ns = KNameSpace(LMI_IPAssignmentSettingDataRef_ToObjectPath(self, NULL));
    Require(AddressType, "AddressType parameter must be present", result, CMPI_RC_ERR_INVALID_PARAMETER);
    Require(DestinationAddress, "DestinationAddress parameter must be present", result, CMPI_RC_ERR_INVALID_PARAMETER);
    int prefix = 0;
    if (AddressType->value == IPv4) {
        // IPv4
        Require(DestinationMask, "DestinationMask parameter must be present", result, CMPI_RC_ERR_INVALID_PARAMETER);
        prefix = netmaskToPrefix4(DestinationMask->chars);
    } else if (AddressType->value == IPv6) {
        // IPv6
        Require(PrefixLength, "PrefixLength parameter must be present", result, CMPI_RC_ERR_INVALID_PARAMETER);
        prefix = PrefixLength->value;
    }

    Connection *connection = NULL;
    network_lock(network);
    char *connection_id = id_from_instanceid(self->InstanceID.chars, "LMI_IPAssignmentSettingData");
    if (connection_id == NULL) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Invalid connection");
        res = LMI_ERROR_UNKNOWN;
        goto err;
    }
    const Connections *connections = network_get_connections(network);
    Connection *old_connection;
    if ((old_connection = connections_find_by_id(connections, connection_id)) == NULL) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Invalid connection");
        free(connection_id);
        res = LMI_ERROR_UNKNOWN;
        goto err;
    }
    free(connection_id);
    if ((connection = connection_clone(old_connection)) == NULL) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Memory allocation failed");
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    const Settings *settings = connection_get_settings(connection);
    Setting *setting = NULL, *s;
    for (size_t i = 0; i < settings_length(settings); ++i) {
        s = settings_index(settings, i);
        switch (setting_get_type(s)) {
            case SETTING_TYPE_IPv4:
                if ((AddressType->value == IPv4) &&
                    (setting_get_method(s) != SETTING_METHOD_DISABLED)) {

                    setting = s;
                }
                break;
            case SETTING_TYPE_IPv6:
                if ((AddressType->value == IPv6) &&
                    (setting_get_method(s) != SETTING_METHOD_DISABLED)) {

                    setting = s;
                }
                break;
            default:
                break;
        }
    }
    if (setting == NULL) {
        if (AddressType->value == IPv4) {
            CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Unable to add IPv4 route to IPv6 only connection");
        } else {
            CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Unable to add IPv6 route to IPv4 only connection");
        }
        res = LMI_WRONG_PARAMETER;
        goto err;
    }
    if ((res = setting_add_route(setting, DestinationAddress->chars, prefix, NULL, 0)) != LMI_SUCCESS) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Memory allocation failed");
        goto err;
    }

    if ((res = connection_update(old_connection, connection)) != LMI_SUCCESS) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Unable to change the connection");
        goto err;
    }

    LMI_IPRouteSettingDataRef w;
    LMI_IPRouteSettingDataRef_Init(&w, cb, ns);
    char *id, *settingid;
    if ((settingid = id_to_instanceid(setting_get_id(setting), "LMI_IPRouteSettingData")) == NULL) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Memory allocation failed");
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    if (asprintf(&id, "%s_%ld", settingid, setting_get_routes_length(setting) - 1) < 0) {
        CMSetStatusWithChars(_cb, status, CMPI_RC_ERR_FAILED, "Memory allocation failed");
        res = LMI_ERROR_MEMORY;
        goto err;
    }
    LMI_IPRouteSettingDataRef_Set_InstanceID(&w, id);
    free(id);
    free(settingid);
    KRef_SetObjectPath(Route, LMI_IPRouteSettingDataRef_ToObjectPath(&w, NULL));
    CMSetStatus(status, CMPI_RC_OK);
err:
    connection_free(connection);
    network_unlock(network);
    KUint32_Set(&result, (int) res);
    return result;
}

CMMethodMIStub(
    LMI_IPAssignmentSettingData,
    LMI_IPAssignmentSettingData,
    _cb,
    LMI_IPAssignmentSettingDataMethodInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPAssignmentSettingData",
    "LMI_IPAssignmentSettingData",
    "instance method")
