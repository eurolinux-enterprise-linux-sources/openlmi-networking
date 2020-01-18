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
#include "LMI_LinkAggregator8023ad.h"
#include "network.h"
#include "port.h"

static const CMPIBroker* _cb = NULL;

static void LMI_LinkAggregator8023adInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LinkAggregator8023adCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LinkAggregator8023adEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_LinkAggregator8023adEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        if (port_get_type(port) == TYPE_BOND) {
            LMI_LinkAggregator8023ad w;
            LMI_LinkAggregator8023ad_Init(&w, _cb, KNameSpace(cop));
            LMI_LinkAggregator8023ad_Set_Name(&w, port_get_id(port));
            LMI_LinkAggregator8023ad_Set_CreationClassName(&w, LMI_LinkAggregator8023ad_ClassName);
            LMI_LinkAggregator8023ad_Set_SystemCreationClassName(&w, get_system_creation_class_name());
            LMI_LinkAggregator8023ad_Set_SystemName(&w, lmi_get_system_name_safe(cc));

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_LinkAggregator8023ad_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_LinkAggregator8023adGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_LinkAggregator8023adCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregator8023adModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregator8023adDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_LinkAggregator8023adExecQuery(
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
    LMI_LinkAggregator8023ad,
    LMI_LinkAggregator8023ad,
    _cb,
    LMI_LinkAggregator8023adInitialize(&mi, ctx))

static void LMI_LinkAggregator8023adMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_LinkAggregator8023adMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_LinkAggregator8023adInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_LinkAggregator8023ad_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_LinkAggregator8023ad,
    LMI_LinkAggregator8023ad,
    _cb,
    LMI_LinkAggregator8023adMethodInitialize(&mi, ctx))

KUint32 LMI_LinkAggregator8023ad_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LinkAggregator8023adRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_LinkAggregator8023ad_BroadcastReset(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_LinkAggregator8023adRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_LinkAggregator8023ad",
    "LMI_LinkAggregator8023ad",
    "instance method")
