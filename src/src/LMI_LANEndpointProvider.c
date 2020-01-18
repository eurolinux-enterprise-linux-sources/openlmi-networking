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
#include "LMI_LANEndpoint.h"

#include "network.h"
#include "port.h"
#include "errors.h"
#include "ipnetworkconnection.h"

#include <string.h>

static const CMPIBroker* _cb = NULL;

static void LMI_LANEndpointInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LANEndpointCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LANEndpointEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_LANEndpointEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);
    return IPNetworkConnectionEnumInstances(LMI_LANEndpoint_Type,
                                            network, cr, _cb, ns);
}

static CMPIStatus LMI_LANEndpointGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_LANEndpointCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LANEndpointModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LANEndpointDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LANEndpointExecQuery(
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
    LMI_LANEndpoint,
    LMI_LANEndpoint,
    _cb,
    LMI_LANEndpointInitialize(&mi, ctx))

static void LMI_LANEndpointMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LANEndpointMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LANEndpointInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_LANEndpoint_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_LANEndpoint,
    LMI_LANEndpoint,
    _cb,
    LMI_LANEndpointMethodInitialize(&mi, ctx))

enum {
    LMI_LANEndpoint_RequestStateChange_No_Error=0,
    LMI_LANEndpoint_RequestStateChange_Not_Supported=1,
    LMI_LANEndpoint_RequestStateChange_Unknown_Error=2,
    LMI_LANEndpoint_RequestStateChange_Failed=4,
    LMI_LANEndpoint_RequestStateChange_Invalid_Parameter=5,
    LMI_LANEndpoint_RequestStateChange_Timeout_Not_Supported=4098
};

KUint32 LMI_LANEndpoint_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LANEndpointRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    Network *network = mi->hdl;
    KUint32 result = KUINT32_INIT;

    if (TimeoutPeriod->exists && !TimeoutPeriod->null) {
        KSetStatus2(_cb, status, ERR_NOT_SUPPORTED, "Use of Timeout Parameter Not Supported");
        KUint32_Set(&result, LMI_LANEndpoint_RequestStateChange_Timeout_Not_Supported);
        return result;
    }

    Require(RequestedState, "No state has been requested", result, LMI_LANEndpoint_RequestStateChange_Invalid_Parameter);

    network_lock(network);
    Port *port = NULL;
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (strcmp(port_get_id(ports_index(ports, i)), self->Name.chars) == 0) {
            port = ports_index(ports, i);
        }
    }
    if (port == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "No such " LMI_LANEndpoint_ClassName);
        KUint32_Set(&result, LMI_LANEndpoint_RequestStateChange_Invalid_Parameter);
        network_unlock(network);
        return result;
    }

    int res = LMI_SUCCESS;
    switch (RequestedState->value) {
        case LMI_LANEndpoint_RequestedState_Disabled:
            res = port_request_state(port, STATE_DISABLED);
            break;
        case LMI_LANEndpoint_RequestedState_Enabled:
            res = port_request_state(port, STATE_ENABLED);
            break;
        default:
            KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Invalid state requested");
            KUint32_Set(&result, LMI_LANEndpoint_RequestStateChange_Invalid_Parameter);
            network_unlock(network);
            return result;
    }
    network_unlock(network);

    if (res != LMI_SUCCESS) {
        KSetStatus2(_cb, status, ERR_FAILED, "Unable to set state");
        KUint32_Set(&result, LMI_LANEndpoint_RequestStateChange_Failed);
        return result;
    }

    KSetStatus(status, OK);
    KUint32_Set(&result, LMI_LANEndpoint_RequestStateChange_No_Error);
    return result;
}

KUint32 LMI_LANEndpoint_BroadcastReset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LANEndpointRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_LANEndpoint",
    "LMI_LANEndpoint",
    "instance method")
