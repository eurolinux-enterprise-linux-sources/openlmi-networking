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
#include "LMI_NetworkDeviceSAPImplementation.h"
#include "LMI_EthernetPort.h"
#include "LMI_LANEndpoint.h"

#include "network.h"
#include "globals.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_NetworkDeviceSAPImplementationInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    const char *ns = KNameSpace(cop);
    Network *network = mi->hdl;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        CMPIObjectPath *networkPortOP = CIM_NetworkPortRefOP(port_get_id(port), LMI_EthernetPort_ClassName, _cb, cc, ns);
        CMPIObjectPath *serviceAccessPointOP = CIM_ServiceAccessPointRefOP(port_get_id(port), LMI_LANEndpoint_ClassName, _cb, cc, ns);

        LMI_NetworkDeviceSAPImplementation w;
        LMI_NetworkDeviceSAPImplementation_Init(&w, _cb, ns);
        LMI_NetworkDeviceSAPImplementation_SetObjectPath_Antecedent(&w, networkPortOP);
        LMI_NetworkDeviceSAPImplementation_SetObjectPath_Dependent(&w, serviceAccessPointOP);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_NetworkDeviceSAPImplementation_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_NetworkDeviceSAPImplementationAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationAssociators(
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
        LMI_NetworkDeviceSAPImplementation_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationAssociatorNames(
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
        LMI_NetworkDeviceSAPImplementation_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationReferences(
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
        LMI_NetworkDeviceSAPImplementation_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_NetworkDeviceSAPImplementationReferenceNames(
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
        LMI_NetworkDeviceSAPImplementation_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_NetworkDeviceSAPImplementation,
    LMI_NetworkDeviceSAPImplementation,
    _cb,
    LMI_NetworkDeviceSAPImplementationInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_NetworkDeviceSAPImplementation,
    LMI_NetworkDeviceSAPImplementation,
    _cb,
    LMI_NetworkDeviceSAPImplementationAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkDeviceSAPImplementation",
    "LMI_NetworkDeviceSAPImplementation",
    "instance association")
