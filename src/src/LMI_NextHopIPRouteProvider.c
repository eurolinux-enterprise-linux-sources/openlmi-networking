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
#include "LMI_NextHopIPRoute.h"
#include <LMI_IPProtocolEndpoint.h>
#include "network.h"
#include "port.h"

static const CMPIBroker* _cb = NULL;

static void LMI_NextHopIPRouteInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NextHopIPRouteCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NextHopIPRouteEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NextHopIPRouteEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    char *name, *id, *mask;
    size_t j;
    Port *port;
    Route *route;
    IPConfig *ipconfig;

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);

        for (j = 0; j < routes_length(ipconfig->routes); ++j) {
            route = routes_index(ipconfig->routes, j);
            if (asprintf(&name, "%s_%zu", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_NextHopIPRoute w;
            LMI_NextHopIPRoute_Init(&w, _cb, ns);
            id = id_to_instanceid(name, "LMI_NextHopIPRoute");
            LMI_NextHopIPRoute_Set_InstanceID(&w, id);
            free(name);
            free(id);

            LMI_NextHopIPRoute_Set_DestinationAddress(&w, route->route);
            if (route->type == IPv4) {
                if ((mask = prefixToMask4(route->prefix)) == NULL) {
                    error("Memory allocation failed");
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
                LMI_NextHopIPRoute_Set_DestinationMask(&w, mask);
                free(mask);
            } else {
                LMI_NextHopIPRoute_Set_PrefixLength(&w, route->prefix);
            }
            LMI_NextHopIPRoute_Set_RouteMetric(&w, route->metric);
            LMI_NextHopIPRoute_Set_AddressType(&w,
                    route->type == IPv4 ?
                    LMI_NextHopIPRoute_AddressType_IPv4 :
                    LMI_NextHopIPRoute_AddressType_IPv6);
            LMI_NextHopIPRoute_Set_NextHop(&w, route->next_hop);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_IPProtocolEndpoint_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NextHopIPRouteGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NextHopIPRouteCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NextHopIPRouteModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NextHopIPRouteDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NextHopIPRouteExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_NextHopIPRoute,
    LMI_NextHopIPRoute,
    _cb,
    LMI_NextHopIPRouteInitialize(&mi, ctx))

static void LMI_NextHopIPRouteMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NextHopIPRouteMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NextHopIPRouteInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_NextHopIPRoute_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_NextHopIPRoute,
    LMI_NextHopIPRoute,
    _cb,
    LMI_NextHopIPRouteMethodInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NextHopIPRoute",
    "LMI_NextHopIPRoute",
    "instance method")
