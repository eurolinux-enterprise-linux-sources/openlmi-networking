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
#include "LMI_LinkAggregationConcreteIdentity.h"
#include "network.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_LinkAggregationConcreteIdentityInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    Ports *slaves;
    Port *port, *slave;
    size_t j;
    const char *ns = KNameSpace(cop);

    LMI_LinkAggregationConcreteIdentity w;
    LMI_LinkAggregationConcreteIdentity_Init(&w, _cb, ns);

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }
        port = ports_index(ports, i);
        if (port_get_type(port) == TYPE_BOND) {
            slaves = port_get_slaves(network, port);
            for (j = 0; j < ports_length(slaves); ++j) {
                slave = ports_index(slaves, j);
                LMI_LinkAggregationConcreteIdentity_SetObjectPath_SystemElement(&w,
                        LMI_LANEndpointRefOP(port_get_id(slave), LMI_LANEndpoint_ClassName, _cb, cc, ns));
                LMI_LinkAggregationConcreteIdentity_SetObjectPath_SameElement(&w,
                        LMI_LAGPort8023adRefOP(port_get_id(slave), LMI_LAGPort8023ad_ClassName, _cb, cc, ns));

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_LinkAggregationConcreteIdentity_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
            ports_free(slaves, false);
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_LinkAggregationConcreteIdentityAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityAssociators(
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
        LMI_LinkAggregationConcreteIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityAssociatorNames(
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
        LMI_LinkAggregationConcreteIdentity_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityReferences(
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
        LMI_LinkAggregationConcreteIdentity_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_LinkAggregationConcreteIdentityReferenceNames(
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
        LMI_LinkAggregationConcreteIdentity_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_LinkAggregationConcreteIdentity,
    LMI_LinkAggregationConcreteIdentity,
    _cb,
    LMI_LinkAggregationConcreteIdentityInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_LinkAggregationConcreteIdentity,
    LMI_LinkAggregationConcreteIdentity,
    _cb,
    LMI_LinkAggregationConcreteIdentityAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_LinkAggregationConcreteIdentity",
    "LMI_LinkAggregationConcreteIdentity",
    "instance association")
