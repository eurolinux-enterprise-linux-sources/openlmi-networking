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
#include "LMI_NetworkSAPSAPDependency.h"
#include "LMI_IPNetworkConnection.h"
#include "LMI_IPProtocolEndpoint.h"
#include "LMI_DNSProtocolEndpoint.h"
#include "network.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_NetworkSAPSAPDependencyInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    char *name;
    Port *port;
    IPConfig *ipconfig;
    size_t j;

    LMI_NetworkSAPSAPDependency w;
    LMI_NetworkSAPSAPDependency_Init(&w, _cb, ns);

    CMPIObjectPath *ipNetworkConnectionOP;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);
        ipNetworkConnectionOP = CIM_ServiceAccessPointRefOP(port_get_id(port), LMI_IPNetworkConnection_ClassName, _cb, cc, ns);

        for (j = 0; j < addresses_length(ipconfig->addresses); ++j) {
            if (asprintf(&name, "%s_%zu", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_NetworkSAPSAPDependency_SetObjectPath_Antecedent(&w, ipNetworkConnectionOP);
            LMI_NetworkSAPSAPDependency_SetObjectPath_Dependent(&w, CIM_ServiceAccessPointRefOP(name, LMI_IPProtocolEndpoint_ClassName, _cb, cc, ns));
            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkSAPSAPDependency_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_NetworkSAPSAPDependency_SetObjectPath_Antecedent(&w, CIM_ServiceAccessPointRefOP(name, LMI_IPProtocolEndpoint_ClassName, _cb, cc, ns));
            free(name);
            LMI_NetworkSAPSAPDependency_SetObjectPath_Dependent(&w, CIM_ServiceAccessPointRefOP(port_get_id(port), LMI_DNSProtocolEndpoint_ClassName, _cb, cc, ns));
            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkSAPSAPDependency_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkSAPSAPDependencyGetInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyCreateInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyModifyInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyDeleteInstance(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_NetworkSAPSAPDependencyAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyAssociators(
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
        LMI_NetworkSAPSAPDependency_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyAssociatorNames(
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
        LMI_NetworkSAPSAPDependency_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyReferences(
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
        LMI_NetworkSAPSAPDependency_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_NetworkSAPSAPDependencyReferenceNames(
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
        LMI_NetworkSAPSAPDependency_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_NetworkSAPSAPDependency,
    LMI_NetworkSAPSAPDependency,
    _cb,
    LMI_NetworkSAPSAPDependencyInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_NetworkSAPSAPDependency,
    LMI_NetworkSAPSAPDependency,
    _cb,
    LMI_NetworkSAPSAPDependencyAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkSAPSAPDependency",
    "LMI_NetworkSAPSAPDependency",
    "instance association")
