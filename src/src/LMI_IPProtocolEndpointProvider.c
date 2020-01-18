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
#include "LMI_IPProtocolEndpoint.h"

#include "network.h"
#include "port.h"
#include "setting.h"
#include "connection.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IPProtocolEndpointInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPProtocolEndpointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPProtocolEndpointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPProtocolEndpointEnumInstances(
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
    size_t j;
    Port *port;
    Address *address;
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
            address = addresses_index(ipconfig->addresses, j);
            asprintf(&name, "%s_%ld", port_get_id(port), j);
            LMI_IPProtocolEndpoint w;
            LMI_IPProtocolEndpoint_Init(&w, _cb, ns);
            LMI_IPProtocolEndpoint_Set_SystemName(&w, get_system_name());
            LMI_IPProtocolEndpoint_Set_SystemCreationClassName(&w, get_system_creation_class_name());
            LMI_IPProtocolEndpoint_Set_CreationClassName(&w, LMI_IPProtocolEndpoint_ClassName);
            LMI_IPProtocolEndpoint_Set_Name(&w, name);
            free(name);

            switch (address->type) {
                case IPv4:
                    LMI_IPProtocolEndpoint_Set_IPv4Address(&w, address->addr);
                    LMI_IPProtocolEndpoint_Set_SubnetMask(&w, prefixToMask4(address->prefix));
                    LMI_IPProtocolEndpoint_Set_ProtocolIFType(&w, LMI_IPProtocolEndpoint_ProtocolIFType_IPv4);
                    break;
                case IPv6:
                    LMI_IPProtocolEndpoint_Set_IPv6Address(&w, address->addr);
                    LMI_IPProtocolEndpoint_Set_IPv6SubnetPrefixLength(&w, address->prefix);
                    LMI_IPProtocolEndpoint_Set_ProtocolIFType(&w, LMI_IPProtocolEndpoint_ProtocolIFType_IPv6);
                    break;
                default:
                    break;
            }

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

static CMPIStatus LMI_IPProtocolEndpointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPProtocolEndpointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPProtocolEndpointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPProtocolEndpointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPProtocolEndpointExecQuery(
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
    LMI_IPProtocolEndpoint,
    LMI_IPProtocolEndpoint,
    _cb,
    LMI_IPProtocolEndpointInitialize(&mi, ctx))

static void LMI_IPProtocolEndpointMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPProtocolEndpointMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPProtocolEndpointInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_IPProtocolEndpoint_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_IPProtocolEndpoint,
    LMI_IPProtocolEndpoint,
    _cb,
    LMI_IPProtocolEndpointMethodInitialize(&mi, ctx))

KUint32 LMI_IPProtocolEndpoint_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPProtocolEndpointRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_IPProtocolEndpoint_BroadcastReset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPProtocolEndpointRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPProtocolEndpoint",
    "LMI_IPProtocolEndpoint",
    "instance method")
