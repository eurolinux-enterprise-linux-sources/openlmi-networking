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
#include "LMI_NetworkElementCapabilities.h"
#include "LMI_NetworkEnabledLogicalElementCapabilities.h"
#include "LMI_EthernetPort.h"
#include "LMI_LANEndpoint.h"
#include "network.h"
#include "port.h"
#include "globals.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_NetworkElementCapabilitiesInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkElementCapabilitiesCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkElementCapabilitiesEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkElementCapabilitiesEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);
    Port *port;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);

        // Capabilities of LMI_EthernetPort
        LMI_NetworkElementCapabilities w;
        LMI_NetworkElementCapabilities_Init(&w, _cb, ns);
        LMI_NetworkElementCapabilities_SetObjectPath_ManagedElement(&w, CIM_NetworkPortRefOP(port_get_id(port), LMI_EthernetPort_ClassName, _cb, ns));
        LMI_NetworkEnabledLogicalElementCapabilitiesRef r;
        LMI_NetworkEnabledLogicalElementCapabilitiesRef_Init(&r, _cb, ns);
        LMI_NetworkEnabledLogicalElementCapabilitiesRef_Set_InstanceID(&r, ORGID ":NetworkPortEnabledLogicalElementCapabilities");
        LMI_NetworkElementCapabilities_Set_Capabilities(&w, &r);
        LMI_NetworkElementCapabilities_Init_Characteristics(&w, 1);
        LMI_NetworkElementCapabilities_Set_Characteristics(&w, 0, LMI_NetworkElementCapabilities_Characteristics_Current);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_NetworkElementCapabilities_ClassName);
        }

        // Capabilities of LMI_LANEndpoint
        LMI_NetworkElementCapabilities_Init(&w, _cb, ns);
        LMI_NetworkElementCapabilities_SetObjectPath_ManagedElement(&w, CIM_LANEndpointRefOP(port_get_id(port), LMI_LANEndpoint_ClassName, _cb, ns));
        LMI_NetworkEnabledLogicalElementCapabilitiesRef_Init(&r, _cb, ns);
        LMI_NetworkEnabledLogicalElementCapabilitiesRef_Set_InstanceID(&r, ORGID ":NetworkLANEnabledLogicalElementCapabilities");
        LMI_NetworkElementCapabilities_Set_Capabilities(&w, &r);
        LMI_NetworkElementCapabilities_Init_Characteristics(&w, 1);
        LMI_NetworkElementCapabilities_Set_Characteristics(&w, 0, LMI_NetworkElementCapabilities_Characteristics_Current);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_NetworkElementCapabilities_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkElementCapabilitiesGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkElementCapabilitiesCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkElementCapabilitiesModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkElementCapabilitiesDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkElementCapabilitiesExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_NetworkElementCapabilitiesAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkElementCapabilitiesAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkElementCapabilitiesAssociators(
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
        LMI_NetworkElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_NetworkElementCapabilitiesAssociatorNames(
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
        LMI_NetworkElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_NetworkElementCapabilitiesReferences(
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
        LMI_NetworkElementCapabilities_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_NetworkElementCapabilitiesReferenceNames(
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
        LMI_NetworkElementCapabilities_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_NetworkElementCapabilities,
    LMI_NetworkElementCapabilities,
    _cb,
    LMI_NetworkElementCapabilitiesInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_NetworkElementCapabilities,
    LMI_NetworkElementCapabilities,
    _cb,
    LMI_NetworkElementCapabilitiesAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkElementCapabilities",
    "LMI_NetworkElementCapabilities",
    "instance association")
