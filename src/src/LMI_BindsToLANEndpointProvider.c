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
#include "LMI_BindsToLANEndpoint.h"
#include "LMI_IPProtocolEndpoint.h"
#include "LMI_LANEndpoint.h"

#include "network.h"
#include "globals.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_BindsToLANEndpointInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BindsToLANEndpointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BindsToLANEndpointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BindsToLANEndpointEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    IPConfig *ipconfig;
    Port *port;
    char *name;
    size_t j;

    network_lock(network);
    const Ports *ports = network_get_ports(network);

    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);

        CMPIObjectPath *lanEndpointOP = CIM_LANEndpointRefOP(port_get_id(port), LMI_LANEndpoint_ClassName, _cb, cc, ns);
        if (lanEndpointOP == NULL) {
            error("Unable to get reference to " LMI_LANEndpoint_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }

        for (j = 0; j < addresses_length(ipconfig->addresses); ++j) {
            if (asprintf(&name, "%s_%zu", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            CMPIObjectPath *serviceAccessPointOP = CIM_ServiceAccessPointRefOP(name, LMI_IPProtocolEndpoint_ClassName, _cb, cc, ns);
            if (serviceAccessPointOP == NULL) {
                error("Unable to get reference to " LMI_IPProtocolEndpoint_ClassName);
                free(name);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            free(name);

            LMI_BindsToLANEndpoint w;
            LMI_BindsToLANEndpoint_Init(&w, _cb, ns);
            LMI_BindsToLANEndpoint_SetObjectPath_Dependent(&w, serviceAccessPointOP);
            LMI_BindsToLANEndpoint_SetObjectPath_Antecedent(&w, lanEndpointOP);
            LMI_BindsToLANEndpoint_Set_FrameType(&w, LMI_BindsToLANEndpoint_FrameType_Ethernet);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_BindsToLANEndpoint_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_BindsToLANEndpointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BindsToLANEndpointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BindsToLANEndpointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BindsToLANEndpointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BindsToLANEndpointExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_BindsToLANEndpointAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BindsToLANEndpointAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BindsToLANEndpointAssociators(
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
        LMI_BindsToLANEndpoint_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_BindsToLANEndpointAssociatorNames(
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
        LMI_BindsToLANEndpoint_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_BindsToLANEndpointReferences(
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
        LMI_BindsToLANEndpoint_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_BindsToLANEndpointReferenceNames(
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
        LMI_BindsToLANEndpoint_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_BindsToLANEndpoint,
    LMI_BindsToLANEndpoint,
    _cb,
    LMI_BindsToLANEndpointInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_BindsToLANEndpoint,
    LMI_BindsToLANEndpoint,
    _cb,
    LMI_BindsToLANEndpointAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_BindsToLANEndpoint",
    "LMI_BindsToLANEndpoint",
    "instance association")
