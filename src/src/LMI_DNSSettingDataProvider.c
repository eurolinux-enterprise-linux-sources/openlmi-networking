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
#include "LMI_DNSSettingData.h"
#include "network.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_DNSSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_DNSSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DNSSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_DNSSettingDataEnumInstances(
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
    size_t j, k, length;
    Connection *connection;
    Setting *setting;
    network_lock(network);
    const Connections *connections = network_get_connections(network);
    for (size_t i = 0; i < connections_length(connections); ++i) {
        if (!KOkay(res)) {
            break;
        }
        connection = connections_index(connections, i);
        for (j = 0; j < settings_length(connection_get_settings(connection)); ++j) {
            setting = settings_index(connection_get_settings(connection), j);
            if ((setting_get_type(setting) != SETTING_TYPE_IPv4) &&
                    (setting_get_type(setting) != SETTING_TYPE_IPv6)) {

                continue;
            }
            if (setting_get_method(setting) == SETTING_METHOD_DISABLED) {
                continue;
            }

            LMI_DNSSettingData w;
            LMI_DNSSettingData_Init(&w, _cb, ns);
            instanceid = id_to_instanceid(setting_get_id(setting), LMI_DNSSettingData_ClassName);
            if (instanceid == NULL) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            LMI_DNSSettingData_Set_InstanceID(&w, instanceid);
            free(instanceid);
            LMI_DNSSettingData_Set_ElementName(&w, setting_get_caption(setting));

            if (setting_get_type(setting) == SETTING_TYPE_IPv4) {
                LMI_DNSSettingData_Set_ProtocolIFType(&w, LMI_DNSSettingData_ProtocolIFType_IPv4);
            } else if (setting_get_type(setting) == SETTING_TYPE_IPv6) {
                LMI_DNSSettingData_Set_ProtocolIFType(&w, LMI_DNSSettingData_ProtocolIFType_IPv6);
            } else {

                continue;
            }

            length = setting_get_dns_servers_length(setting);
            LMI_DNSSettingData_Init_DNSServerAddresses(&w, length);
            for (k = 0; k < length; ++k) {
                LMI_DNSSettingData_Set_DNSServerAddresses(&w, k, setting_get_dns_server(setting, k));
            }

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_DNSSettingData_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_DNSSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_DNSSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DNSSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_DNSSettingDataRef ref;
    if (!KOkay(LMI_DNSSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        KReturn2(_cb, ERR_FAILED, "Invalid ObjectPath for class " LMI_DNSSettingData_ClassName);
    }
    size_t index;
    char *connection_id = id_from_instanceid_with_index(ref.InstanceID.chars, LMI_DNSSettingData_ClassName, &index);
    if (connection_id == NULL) {
        KReturn2(_cb, ERR_FAILED, "Invalid ObjectPath for class LMI_DNSSettingData");
    }

    LMI_DNSSettingData w;
    if (!KOkay(LMI_DNSSettingData_InitFromInstance(&w, _cb, ci))) {
        free(connection_id);
        KReturn2(_cb, ERR_FAILED, "Invalid instace of class " LMI_DNSSettingData_ClassName);
    }

    if (!w.ProtocolIFType.exists || w.ProtocolIFType.null) {
        free(connection_id);
        KReturn2(_cb, ERR_FAILED, "ProtocolIFType must be specified");
    }

    Network *network = mi->hdl;
    CMPIStatus rc = { CMPI_RC_OK, NULL };
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
    if (connection == NULL) {
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify connection");
    }

    if (w.ProtocolIFType.value == LMI_DNSSettingData_ProtocolIFType_IPv4) {
        setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_IPv4);
    } else if (w.ProtocolIFType.value == LMI_DNSSettingData_ProtocolIFType_IPv6) {
        setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_IPv6);
    }
    if (setting == NULL) {
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Wrong ProtocolIFType");
    }

    setting_clear_dns_servers(setting);
    for (CMPICount i = 0; i < w.DNSServerAddresses.count; ++i) {
        if (setting_add_dns_server(setting, KStringA_Get(&w.DNSServerAddresses, i)) != 0) {
            network_unlock(network);
            connection_free(connection);
            KReturn2(_cb, ERR_FAILED, "Unable to modify connection");
        }
    }

    setting_clear_search_domains(setting);
    for (CMPICount i = 0; i < w.DNSSearchDomains.count; ++i) {
        if (setting_add_search_domain(setting, KStringA_Get(&w.DNSSearchDomains, i)) != 0) {
            network_unlock(network);
            connection_free(connection);
            KReturn2(_cb, ERR_FAILED, "Unable to modify connection");
        }
    }

    rc.rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (!KOkay(rc)) {
        KReturn(ERR_FAILED);
    }
    KReturn(OK);
}

static CMPIStatus LMI_DNSSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_DNSSettingDataExecQuery(
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
    LMI_DNSSettingData,
    LMI_DNSSettingData,
    _cb,
    LMI_DNSSettingDataInitialize(&mi, ctx))

static void LMI_DNSSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_DNSSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_DNSSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_DNSSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_DNSSettingData,
    LMI_DNSSettingData,
    _cb,
    LMI_DNSSettingDataMethodInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_DNSSettingData",
    "LMI_DNSSettingData",
    "instance method")
