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
#include "LMI_OrderedIPAssignmentComponent.h"
#include "LMI_DNSSettingData.h"
#include "LMI_BondingSlaveSettingData.h"
#include "LMI_BridgingSlaveSettingData.h"
#include "LMI_BondingMasterSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "network.h"
#include "connection.h"
#include "ref_factory.h"
#include "setting.h"

static const CMPIBroker* _cb;

static void LMI_OrderedIPAssignmentComponentInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    size_t j, k;
    Setting *setting;
    Connection *connection, *slave, *master;
    char *instanceid, *name, *classname;
    network_lock(network);
    const Connections *connections = network_get_connections(network);
    if (connections == NULL) {
        network_unlock(network);
        CMReturn(CMPI_RC_OK);
    }
    for (size_t i = 0; i < connections_length(connections); ++i) {
        if (!KOkay(res)) {
            break;
        }
        connection = connections_index(connections, i);

        switch (connection_get_type(connection)) {
            case CONNECTION_TYPE_BOND:
                classname = LMI_BondingMasterSettingData_ClassName;
                break;
            case CONNECTION_TYPE_BRIDGE:
                classname = LMI_BridgingMasterSettingData_ClassName;
                break;
            default:
                classname = LMI_IPAssignmentSettingData_ClassName;
        }
        instanceid = id_to_instanceid(connection_get_id(connection), classname);
        CMPIObjectPath *groupComponentOP = CIM_IPAssignmentSettingDataRefOP(instanceid, classname, _cb, ns);
        free(instanceid);

        // Find if there are slave connections for this connection
        for (j = 0; j < connections_length(connections); ++j) {
            slave = connections_index(connections, j);
            master = connection_get_master_connection(slave);
            if (master != NULL && connection_get_id(master) != NULL &&
                    strcmp(connection_get_id(master), connection_get_id(connection)) == 0) {

                LMI_OrderedIPAssignmentComponent w;
                LMI_OrderedIPAssignmentComponent_Init(&w, _cb, ns);
                LMI_OrderedIPAssignmentComponent_SetObjectPath_GroupComponent(&w, groupComponentOP);
                switch (connection_get_type(connection)) {
                    case CONNECTION_TYPE_BOND:
                        classname = LMI_BondingSlaveSettingData_ClassName;
                        break;
                    case CONNECTION_TYPE_BRIDGE:
                        classname = LMI_BridgingSlaveSettingData_ClassName;
                        break;
                    default:
                        error("Master connection is not bridge or bond master");
                }

                instanceid = id_to_instanceid(connection_get_id(slave), classname);
                CMPIObjectPath *partComponentOP = CIM_IPAssignmentSettingDataRefOP(instanceid, classname, _cb, ns);
                free(instanceid);
                LMI_OrderedIPAssignmentComponent_SetObjectPath_PartComponent(&w, partComponentOP);
                LMI_OrderedIPAssignmentComponent_Set_AssignedSequence(&w, 1);

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_OrderedIPAssignmentComponent_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
        }

        for (j = 0; j < settings_length(connection_get_settings(connection)); ++j) {
            setting = settings_index(connection_get_settings(connection), j);
            if ((setting_get_type(setting) != SETTING_TYPE_IPv4) && (setting_get_type(setting) != SETTING_TYPE_IPv6)) {
                continue;
            }
            if (setting_get_method(setting) == SETTING_METHOD_DISABLED) {
                continue;
            }

            CMPIObjectPath *partComponentOP = settingToLMI_IPAssignmentSettingDataSubclassOP(setting, _cb, ns);

            LMI_OrderedIPAssignmentComponent w;
            LMI_OrderedIPAssignmentComponent_Init(&w, _cb, ns);
            LMI_OrderedIPAssignmentComponent_SetObjectPath_GroupComponent(&w, groupComponentOP);
            LMI_OrderedIPAssignmentComponent_SetObjectPath_PartComponent(&w, partComponentOP);
            LMI_OrderedIPAssignmentComponent_Set_AssignedSequence(&w, 1);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_OrderedIPAssignmentComponent_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_DNSSettingDataRef dnsSettingDataRef;
            LMI_DNSSettingDataRef_Init(&dnsSettingDataRef, _cb, ns);
            instanceid = id_to_instanceid(setting_get_id(setting), LMI_DNSSettingData_ClassName);
            LMI_DNSSettingDataRef_Set_InstanceID(&dnsSettingDataRef, instanceid);
            free(instanceid);

            CMPIStatus rc;
            CMPIObjectPath *dnsSettingDataOP = LMI_DNSSettingDataRef_ToObjectPath(&dnsSettingDataRef, &rc);
            dnsSettingDataOP->ft->setClassName(dnsSettingDataOP, LMI_DNSSettingData_ClassName);
            LMI_OrderedIPAssignmentComponent_SetObjectPath_PartComponent(&w, dnsSettingDataOP);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_OrderedIPAssignmentComponent_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            for (k = 0; k < setting_get_routes_length(setting); ++k) {
                LMI_IPRouteSettingDataRef routeRef;
                LMI_IPRouteSettingDataRef_Init(&routeRef, _cb, ns);
                if (asprintf(&name, "%s_%ld", setting_get_id(setting), k) < 0) {
                    error("Memory allocation failed");
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
                instanceid = id_to_instanceid(name, "LMI_IPRouteSettingData");
                LMI_IPRouteSettingDataRef_Set_InstanceID(&routeRef, instanceid);
                free(name);
                free(instanceid);

                CMPIObjectPath *routeOP = LMI_IPRouteSettingDataRef_ToObjectPath(&routeRef, &rc);
                routeOP->ft->setClassName(routeOP, LMI_IPRouteSettingData_ClassName);
                LMI_OrderedIPAssignmentComponent_SetObjectPath_PartComponent(&w, routeOP);

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_OrderedIPAssignmentComponent_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_OrderedIPAssignmentComponentGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_OrderedIPAssignmentComponentAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentAssociators(
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
        LMI_OrderedIPAssignmentComponent_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentAssociatorNames(
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
        LMI_OrderedIPAssignmentComponent_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentReferences(
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
        LMI_OrderedIPAssignmentComponent_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_OrderedIPAssignmentComponentReferenceNames(
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
        LMI_OrderedIPAssignmentComponent_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_OrderedIPAssignmentComponent,
    LMI_OrderedIPAssignmentComponent,
    _cb,
    LMI_OrderedIPAssignmentComponentInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_OrderedIPAssignmentComponent,
    LMI_OrderedIPAssignmentComponent,
    _cb,
    LMI_OrderedIPAssignmentComponentAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_OrderedIPAssignmentComponent",
    "LMI_OrderedIPAssignmentComponent",
    "instance association")
