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
#include "LMI_NetworkRemoteServiceAccessPoint.h"
#include "network.h"
#include "activeconnection.h"
#include "connection.h"
#include "setting.h"
#include "port.h"

static const CMPIBroker* _cb = NULL;

static void LMI_NetworkRemoteServiceAccessPointInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    size_t j;
    Port *port;
    IPConfig *ipconfig;
    Address *address;
    DNSServer *dns_server;
    char *name;

    LMI_NetworkRemoteServiceAccessPoint w;
    LMI_NetworkRemoteServiceAccessPoint_Init(&w, _cb, ns);
    LMI_NetworkRemoteServiceAccessPoint_Set_CreationClassName(&w, LMI_NetworkRemoteServiceAccessPoint_ClassName);
    LMI_NetworkRemoteServiceAccessPoint_Set_SystemCreationClassName(&w, get_system_creation_class_name());
    LMI_NetworkRemoteServiceAccessPoint_Set_SystemName(&w, lmi_get_system_name_safe(cc));

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);
        for (j = 0; j < addresses_length(ipconfig->addresses); ++j) {
            address = addresses_index(ipconfig->addresses, j);

            if (address->default_gateway == NULL) {
                continue;
            }

            if (asprintf(&name, "%s_gateway_%zu", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_NetworkRemoteServiceAccessPoint_Set_Name(&w, name);
            LMI_NetworkRemoteServiceAccessPoint_Set_AccessContext(&w, LMI_NetworkRemoteServiceAccessPoint_AccessContext_Default_Gateway);
            LMI_NetworkRemoteServiceAccessPoint_Set_AccessInfo(&w, address->default_gateway);
            LMI_NetworkRemoteServiceAccessPoint_Set_InfoFormat(&w,
                    (address->type == IPv4) ? LMI_NetworkRemoteServiceAccessPoint_InfoFormat_IPv4_Address
                                            : LMI_NetworkRemoteServiceAccessPoint_InfoFormat_IPv6_Address);
            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkRemoteServiceAccessPoint_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            free(name);
        }
        for (j = 0; j < dns_servers_length(ipconfig->dns_servers); ++j) {
            dns_server = dns_servers_index(ipconfig->dns_servers, j);
            if (asprintf(&name, "%s_dns_%zu", port_get_id(port), j) < 0) {
                error("Memory allocation failed");
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            LMI_NetworkRemoteServiceAccessPoint_Set_Name(&w, name);
            LMI_NetworkRemoteServiceAccessPoint_Set_AccessContext(&w, LMI_NetworkRemoteServiceAccessPoint_AccessContext_DNS_Server);
            LMI_NetworkRemoteServiceAccessPoint_Set_AccessInfo(&w, dns_server->server);
            LMI_NetworkRemoteServiceAccessPoint_Set_InfoFormat(&w,
                    (dns_server->type == IPv4) ? LMI_NetworkRemoteServiceAccessPoint_InfoFormat_IPv4_Address
                                            : LMI_NetworkRemoteServiceAccessPoint_InfoFormat_IPv6_Address);
            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkRemoteServiceAccessPoint_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
            free(name);
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointExecQuery(
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
    LMI_NetworkRemoteServiceAccessPoint,
    LMI_NetworkRemoteServiceAccessPoint,
    _cb,
    LMI_NetworkRemoteServiceAccessPointInitialize(&mi, ctx))

static void LMI_NetworkRemoteServiceAccessPointMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkRemoteServiceAccessPointInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_NetworkRemoteServiceAccessPoint_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_NetworkRemoteServiceAccessPoint,
    LMI_NetworkRemoteServiceAccessPoint,
    _cb,
    LMI_NetworkRemoteServiceAccessPointMethodInitialize(&mi, ctx))

KUint32 LMI_NetworkRemoteServiceAccessPoint_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkRemoteServiceAccessPointRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkRemoteServiceAccessPoint",
    "LMI_NetworkRemoteServiceAccessPoint",
    "instance method")
