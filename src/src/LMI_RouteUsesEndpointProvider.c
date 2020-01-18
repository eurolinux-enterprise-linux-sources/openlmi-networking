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
#include "LMI_RouteUsesEndpoint.h"
#include <LMI_IPProtocolEndpoint.h>
#include <LMI_NextHopIPRoute.h>
#include "network.h"
#include "ipconfig.h"
#include "port.h"

static const CMPIBroker* _cb;

static void LMI_RouteUsesEndpointInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_RouteUsesEndpointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RouteUsesEndpointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_RouteUsesEndpointEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    char *name, *id;
    size_t j, k;
    Port *port;
    IPConfig *ipconfig;

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);

        for (j = 0; j < addresses_length(ipconfig->addresses); ++j) {
            if (!KOkay(res)) {
                break;
            }
            if (asprintf(&name, "%s_%ld", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            LMI_IPProtocolEndpointRef endpoint;
            LMI_IPProtocolEndpointRef_Init(&endpoint, _cb, ns);
            LMI_IPProtocolEndpointRef_Set_SystemName(&endpoint, get_system_name());
            LMI_IPProtocolEndpointRef_Set_SystemCreationClassName(&endpoint, get_system_creation_class_name());
            LMI_IPProtocolEndpointRef_Set_CreationClassName(&endpoint, "LMI_IPProtocolEndpoint");
            LMI_IPProtocolEndpointRef_Set_Name(&endpoint, name);
            free(name);

            for (k = 0; k < routes_length(ipconfig->routes); ++k) {
                if (asprintf(&name, "%s_%ld", port_get_id(port), k) < 0) {
                    error("Memory allocation failed");
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }

                LMI_NextHopIPRouteRef route;
                LMI_NextHopIPRouteRef_Init(&route, _cb, ns);
                if ((id = id_to_instanceid(name, "LMI_NextHopIPRoute")) == NULL) {
                    error("Unable to get ID from InstanceID: %s", name);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
                LMI_NextHopIPRouteRef_Set_InstanceID(&route, id);
                free(name);
                free(id);

                LMI_RouteUsesEndpoint w;
                LMI_RouteUsesEndpoint_Init(&w, _cb, ns);
                LMI_RouteUsesEndpoint_Set_Antecedent(&w, &endpoint);
                LMI_RouteUsesEndpoint_Set_Dependent(&w, &route);
                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_RouteUsesEndpoint_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_RouteUsesEndpointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_RouteUsesEndpointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RouteUsesEndpointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RouteUsesEndpointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_RouteUsesEndpointExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_RouteUsesEndpointAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_RouteUsesEndpointAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_RouteUsesEndpointAssociators(
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
        LMI_RouteUsesEndpoint_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_RouteUsesEndpointAssociatorNames(
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
        LMI_RouteUsesEndpoint_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_RouteUsesEndpointReferences(
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
        LMI_RouteUsesEndpoint_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_RouteUsesEndpointReferenceNames(
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
        LMI_RouteUsesEndpoint_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_RouteUsesEndpoint,
    LMI_RouteUsesEndpoint,
    _cb,
    LMI_RouteUsesEndpointInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_RouteUsesEndpoint,
    LMI_RouteUsesEndpoint,
    _cb,
    LMI_RouteUsesEndpointAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_RouteUsesEndpoint",
    "LMI_RouteUsesEndpoint",
    "instance association")
