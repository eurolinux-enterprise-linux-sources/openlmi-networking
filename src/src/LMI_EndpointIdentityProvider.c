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
#include "LMI_EndpointIdentity.h"
#include "LMI_LANEndpoint.h"
#include "LMI_SwitchPort.h"
#include "network.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_EndpointIdentityInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_EndpointIdentityCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EndpointIdentityEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_EndpointIdentityEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    LMI_EndpointIdentity w;
    LMI_EndpointIdentity_Init(&w, _cb, ns);

    Port *port, *slave;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Ports *slaves;
    size_t j;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }

        port = ports_index(ports, i);
        if (port_get_type(port) != TYPE_BRIDGE) {
            continue;
        }
        slaves = port_get_slaves(network, port);
        for (j = 0; j < ports_length(slaves); ++j) {
            slave = ports_index(slaves, j);

            LMI_EndpointIdentity_SetObjectPath_SystemElement(&w,
                    CIM_ProtocolEndpointRefOP(port_get_id(slave),
                                              LMI_LANEndpoint_ClassName, _cb, cc, ns));
            LMI_EndpointIdentity_SetObjectPath_SameElement(&w,
                    CIM_ProtocolEndpointRefOP(port_get_id(slave),
                                              LMI_SwitchPort_ClassName, _cb, cc, ns));

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_EndpointIdentity_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
        ports_free(slaves, false);
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_EndpointIdentityGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_EndpointIdentityCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EndpointIdentityModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EndpointIdentityDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EndpointIdentityExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_EndpointIdentityAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_EndpointIdentityAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EndpointIdentityAssociators(
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
        LMI_EndpointIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_EndpointIdentityAssociatorNames(
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
        LMI_EndpointIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_EndpointIdentityReferences(
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
        LMI_EndpointIdentity_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_EndpointIdentityReferenceNames(
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
        LMI_EndpointIdentity_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_EndpointIdentity,
    LMI_EndpointIdentity,
    _cb,
    LMI_EndpointIdentityInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_EndpointIdentity,
    LMI_EndpointIdentity,
    _cb,
    LMI_EndpointIdentityAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_EndpointIdentity",
    "LMI_EndpointIdentity",
    "instance association")
