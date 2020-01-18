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

#include "globals.h"
#include <konkret/konkret.h>
#include "LMI_IPElementSettingData.h"
#include "LMI_BondingMasterSettingData.h"
#include "LMI_BondingSlaveSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "LMI_BridgingSlaveSettingData.h"
#include <LMI_IPAssignmentSettingData.h>
#include <LMI_IPNetworkConnection.h>
#include <LMI_LinkAggregator8023ad.h>
#include <LMI_SwitchService.h>
#include "network.h"
#include "port.h"
#include "connection.h"
#include "activeconnection.h"
#include "ref_factory.h"
#include "setting.h"

static const CMPIBroker* _cb;

static void LMI_IPElementSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPElementSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPElementSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPElementSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    network_lock(network);
    const Connections *connections = network_get_connections(network);
    if (!connections) {
        network_unlock(network);
        KReturn(OK);
    }
    Connection *connection, *master;
    const Ports *ports = network_get_ports(network);
    Port *port;
    Ports *available_for_ports;
    size_t j;
    char *instanceid;
    bool isActive = false;

    const ActiveConnections *activeConnections = network_get_active_connections(network);

    CMPIObjectPath *settingDataOP, *managedElementOP;
    LMI_IPElementSettingData w;
    LMI_IPElementSettingData_Init(&w, _cb, ns);

    for (size_t i = 0; i < connections_length(connections); ++i) {
        if (!KOkay(res)) {
            break;
        }
        connection = connections_index(connections, i);

        instanceid = NULL;
        switch (connection_get_type(connection)) {
            case CONNECTION_TYPE_BOND:
                instanceid = id_to_instanceid(connection_get_id(connection), LMI_BondingMasterSettingData_ClassName);
                settingDataOP = CIM_IPAssignmentSettingDataRefOP(instanceid, LMI_BondingMasterSettingData_ClassName, _cb, cc, ns);
                break;
            case CONNECTION_TYPE_BRIDGE:
                instanceid = id_to_instanceid(connection_get_id(connection), LMI_BridgingMasterSettingData_ClassName);
                settingDataOP = CIM_IPAssignmentSettingDataRefOP(instanceid, LMI_BridgingMasterSettingData_ClassName, _cb, cc, ns);
                break;
            default:
                master = connection_get_master_connection(connection);
                if (master != NULL) {
                    switch (connection_get_type(master)) {
                        case CONNECTION_TYPE_BOND:
                            instanceid = id_to_instanceid(connection_get_id(connection), LMI_BondingSlaveSettingData_ClassName);
                            settingDataOP = CIM_IPAssignmentSettingDataRefOP(instanceid, LMI_BondingSlaveSettingData_ClassName, _cb, cc, ns);
                            break;
                        case CONNECTION_TYPE_BRIDGE:
                            instanceid = id_to_instanceid(connection_get_id(connection), LMI_BridgingSlaveSettingData_ClassName);
                            settingDataOP = CIM_IPAssignmentSettingDataRefOP(instanceid, LMI_BridgingSlaveSettingData_ClassName, _cb, cc, ns);
                            break;
                        default:
                            error("Wrong master setting data type: %d", connection_get_type(master));
                            break;
                    }
                } else {
                    instanceid = id_to_instanceid(connection_get_id(connection), LMI_IPAssignmentSettingData_ClassName);
                    settingDataOP = CIM_IPAssignmentSettingDataRefOP(instanceid, LMI_IPAssignmentSettingData_ClassName, _cb, cc, ns);
                }
                break;
        }
        if (instanceid == NULL) {
            continue;
        }
        free(instanceid);
        LMI_IPElementSettingData_SetObjectPath_SettingData(&w, settingDataOP);

        if (connection_get_port(connection)) {
            // Connection is available for one port only
            available_for_ports = ports_new(1);
            ports_add(available_for_ports, (Port *) connection_get_port(connection));
        } else {
            // Connection is available for all ports
            available_for_ports = (Ports *) ports;
        }

        for (j = 0; j < ports_length(available_for_ports); ++j) {
            port = ports_index(available_for_ports, j);

            if (connection_get_type(connection) == CONNECTION_TYPE_BOND) {
                isActive = active_connections_is_connection_active_on_port(activeConnections, connection, NULL);

                if (port_get_type(port) == TYPE_BOND) {
                    // Association between IPAssignmentSettingData and LinkAggregator8023ad
                    managedElementOP = CIM_ProtocolEndpointRefOP(port_get_id(port), LMI_LinkAggregator8023ad_ClassName, _cb, cc, ns);
                } else {
                    // Master connection doesn't have association to any other port than LinkAggregator8023ad
                    continue;
                }
            } else if (connection_get_type(connection) == CONNECTION_TYPE_BRIDGE) {
                isActive = active_connections_is_connection_active_on_port(activeConnections, connection, NULL);

                if (port_get_type(port) == TYPE_BRIDGE) {
                    // Association between IPAssignmentSettingData and SwitchService
                    managedElementOP = CIM_ServiceRefOP(port_get_id(port), LMI_SwitchService_ClassName, _cb, cc, ns);
                } else {
                    // Master connection doesn't have association to any other port than SwitchService
                    continue;
                }
            } else {
                isActive = active_connections_is_connection_active_on_port(activeConnections, connection, port);

                // Association between IPAssignmentSettingData and IPNetworkConnection
                managedElementOP = CIM_ProtocolEndpointRefOP(port_get_id(port), LMI_IPNetworkConnection_ClassName, _cb, cc, ns);
            }
            LMI_IPElementSettingData_SetObjectPath_ManagedElement(&w, managedElementOP);
            LMI_IPElementSettingData_Set_IsCurrent(&w,
                    isActive ?
                        LMI_IPElementSettingData_IsCurrent_Is_Current : LMI_IPElementSettingData_IsCurrent_Is_Not_Current);
            LMI_IPElementSettingData_Set_IsDefault(&w,
                    connection_get_autoconnect(connection) ?
                        LMI_IPElementSettingData_IsDefault_Is_Default : LMI_IPElementSettingData_IsDefault_Is_Not_Default);
            LMI_IPElementSettingData_Set_IsNext(&w,
                    connection_get_autoconnect(connection) ?
                        LMI_IPElementSettingData_IsNext_Is_Next : LMI_IPElementSettingData_IsNext_Is_Not_Next);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_IPElementSettingData_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }

        // free available_for_ports if needed
        if (connection_get_port(connection)) {
            ports_free(available_for_ports, false);
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_IPElementSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPElementSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPElementSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPElementSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPElementSettingDataExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_IPElementSettingDataAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPElementSettingDataAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPElementSettingDataAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_IPElementSettingData_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_IPElementSettingDataAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_IPElementSettingData_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_IPElementSettingDataReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_IPElementSettingData_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_IPElementSettingDataReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_IPElementSettingData_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_IPElementSettingData,
    LMI_IPElementSettingData,
    _cb,
    LMI_IPElementSettingDataInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_IPElementSettingData,
    LMI_IPElementSettingData,
    _cb,
    LMI_IPElementSettingDataAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPElementSettingData",
    "LMI_IPElementSettingData",
    "instance association")
