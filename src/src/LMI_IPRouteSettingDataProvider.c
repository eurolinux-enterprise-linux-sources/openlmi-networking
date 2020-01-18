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
#include "LMI_IPRouteSettingData.h"
#include "network.h"
#include "ipassignmentsettingdata.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IPRouteSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPRouteSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPRouteSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPRouteSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_IPRouteSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_IPRouteSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPRouteSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPRouteSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_IPRouteSettingDataRef ref;
    if (!KOkay(LMI_IPRouteSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to LMI_IPRouteSettingData");
        KReturn(ERR_INVALID_PARAMETER);
    }
    LMI_IPRouteSettingData w;
    LMI_IPRouteSettingData_InitFromInstance(&w, _cb, ci);

    if (!w.AddressType.exists || w.AddressType.null) {
        KReturn2(_cb, ERR_INVALID_PARAMETER, "AddressType must be present");
    }
    if (!w.DestinationAddress.exists || w.DestinationAddress.null) {
        KReturn2(_cb, ERR_INVALID_PARAMETER, "DestinationAddress must be present");
    }
    int prefix;
    ProtocolType type;
    switch (w.AddressType.value) {
        case LMI_IPRouteSettingData_AddressType_IPv4:
            if (!w.DestinationMask.exists || w.DestinationMask.null) {
                KReturn2(_cb, ERR_INVALID_PARAMETER, "DestinationMask must be present");
            }
            prefix = netmaskToPrefix4(w.DestinationMask.chars);
            type = IPv4;
            break;
        case LMI_IPRouteSettingData_AddressType_IPv6:
            if (!w.PrefixLength.exists || w.PrefixLength.null) {
                KReturn2(_cb, ERR_INVALID_PARAMETER, "PrefixLength must be present");
            }
            prefix = w.PrefixLength.value;
            type = IPv6;
            break;
        default:
            KReturn2(_cb, ERR_INVALID_PARAMETER, "AddressType has invalid value");
    }

    Network *network = mi->hdl;
    size_t setting_index, route_index;
    char *id = id_from_instanceid_with_index2(w.InstanceID.chars, "LMI_IPRouteSettingData", &setting_index, &route_index);
    if (id == NULL) {
        KReturn2(_cb, ERR_INVALID_PARAMETER, "No such instance of LMI_IPRouteSettingData");
    }

    network_lock(network);
    const Connections *connections = network_get_connections(network);
    Connection *old_connection = connections_find_by_id(connections, id);
    free(id);
    if (old_connection == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "No such connection");
    }

    Connection *connection = connection_clone(old_connection);
    if (connection == NULL) {
        error("Unable to clone connection");
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify the connection");
    }
    Setting *setting = settings_index(connection_get_settings(connection), setting_index);
    if (setting == NULL) {
        error("Unable to get setting with index %zu", setting_index);
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify the connection");
    }
    Route *route = setting_get_route(setting, route_index);
    if (route == NULL) {
        error("Unable to get route with index %zu", route_index);
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify the connection");
    }

    route->type = type;
    if ((route->route = strdup(w.DestinationAddress.chars)) == NULL) {
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
    }
    route->prefix = prefix;
    if (w.RouteMetric.exists && !w.RouteMetric.null) {
        route->metric = w.RouteMetric.value;
    }
    if (w.NextHop.exists && !w.NextHop.null) {
        if ((route->next_hop = strdup(w.NextHop.chars)) == NULL) {
            connection_free(connection);
            network_unlock(network);
            KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
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

static CMPIStatus LMI_IPRouteSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    LMI_IPRouteSettingDataRef ref;
    if (!KOkay(LMI_IPRouteSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to LMI_IPRouteSettingData");
        KReturn(ERR_INVALID_PARAMETER);
    }
    size_t setting_index, route_index;
    char *id = id_from_instanceid_with_index2(ref.InstanceID.chars, "LMI_IPRouteSettingData", &setting_index, &route_index);
    if (id == NULL) {
        KReturn2(_cb, ERR_INVALID_PARAMETER, "No such instance of LMI_IPRouteSettingData");
    }

    Network *network = mi->hdl;
    network_lock(network);
    const Connections *connections = network_get_connections(network);
    Connection *old_connection = connections_find_by_id(connections, id);
    free(id);
    if (old_connection == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "No such connection");
    }

    Connection *connection = connection_clone(old_connection);
    if (connection == NULL) {
        error("Unable to clone connection");
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify the connection");
    }
    Setting *setting = settings_index(connection_get_settings(connection), setting_index);
    if (setting == NULL) {
        error("Unable to get setting with index %zu", setting_index);
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to modify the connection");
    }
    if (setting_delete_route(setting, route_index) != LMI_SUCCESS) {
        error("Unable to delete route: no such route");
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Unable to delete route");
    }

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPRouteSettingDataExecQuery(
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
    LMI_IPRouteSettingData,
    LMI_IPRouteSettingData,
    _cb,
    LMI_IPRouteSettingDataInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPRouteSettingData",
    "LMI_IPRouteSettingData",
    "instance method")
