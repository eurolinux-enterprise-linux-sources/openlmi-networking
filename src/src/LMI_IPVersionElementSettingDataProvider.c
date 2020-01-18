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
#include "LMI_IPVersionElementSettingData.h"
#include "LMI_IPVersionSettingData.h"
#include "LMI_IPNetworkConnection.h"
#include "network.h"
#include "ref_factory.h"
#include "port.h"

static const CMPIBroker* _cb;

static void LMI_IPVersionElementSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPVersionElementSettingDataCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPVersionElementSettingDataEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPVersionElementSettingDataEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    LMI_IPVersionElementSettingData w;
    LMI_IPVersionElementSettingData_Init(&w, _cb, ns);

    char *ipv4instanceid = id_to_instanceid("IPv4", LMI_IPVersionSettingData_ClassName);
    CMPIObjectPath *ipv4OP = CIM_IPVersionSettingDataRefOP(ipv4instanceid, LMI_IPVersionSettingData_ClassName, _cb, cc, ns);
    char *ipv6instanceid = id_to_instanceid("IPv6", LMI_IPVersionSettingData_ClassName);
    CMPIObjectPath *ipv6OP = CIM_IPVersionSettingDataRefOP(ipv6instanceid, LMI_IPVersionSettingData_ClassName, _cb, cc, ns);

    LMI_IPVersionElementSettingData_SetObjectPath_ManagedElement(&w, lmi_get_computer_system_safe(cc));

    LMI_IPVersionElementSettingData_SetObjectPath_SettingData(&w, ipv4OP);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_IPVersionElementSettingData_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    LMI_IPVersionElementSettingData_SetObjectPath_SettingData(&w, ipv6OP);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_IPVersionElementSettingData_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    CMPIObjectPath *networkOP;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        networkOP = CIM_IPNetworkConnectionRefOP(port_get_id(ports_index(ports, i)), LMI_IPNetworkConnection_ClassName, _cb, cc, ns);
        LMI_IPVersionElementSettingData_SetObjectPath_ManagedElement(&w, networkOP);

        LMI_IPVersionElementSettingData_SetObjectPath_SettingData(&w, ipv4OP);
        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_IPVersionElementSettingData_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        }

        LMI_IPVersionElementSettingData_SetObjectPath_SettingData(&w, ipv6OP);
        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_IPVersionElementSettingData_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        }
    }
    network_unlock(network);
    free(ipv4instanceid);
    free(ipv6instanceid);
    return res;
}

static CMPIStatus LMI_IPVersionElementSettingDataGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPVersionElementSettingDataCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionElementSettingDataModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionElementSettingDataDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionElementSettingDataExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_IPVersionElementSettingDataAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPVersionElementSettingDataAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPVersionElementSettingDataAssociators(
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
        LMI_IPVersionElementSettingData_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_IPVersionElementSettingDataAssociatorNames(
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
        LMI_IPVersionElementSettingData_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_IPVersionElementSettingDataReferences(
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
        LMI_IPVersionElementSettingData_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_IPVersionElementSettingDataReferenceNames(
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
        LMI_IPVersionElementSettingData_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_IPVersionElementSettingData,
    LMI_IPVersionElementSettingData,
    _cb,
    LMI_IPVersionElementSettingDataInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_IPVersionElementSettingData,
    LMI_IPVersionElementSettingData,
    _cb,
    LMI_IPVersionElementSettingDataAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPVersionElementSettingData",
    "LMI_IPVersionElementSettingData",
    "instance association")
