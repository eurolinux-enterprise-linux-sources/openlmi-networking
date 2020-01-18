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
#include "LMI_NetworkEnabledLogicalElementCapabilities.h"
#include "network.h"

static const CMPIBroker* _cb = NULL;

static void LMI_NetworkEnabledLogicalElementCapabilitiesInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    // Capabilities for LMI_EthernetPort
    LMI_NetworkEnabledLogicalElementCapabilities w;
    LMI_NetworkEnabledLogicalElementCapabilities_Init(&w, _cb, KNameSpace(cop));
    LMI_NetworkEnabledLogicalElementCapabilities_Set_InstanceID(&w, ORGID ":NetworkPortEnabledLogicalElementCapabilities");
    LMI_NetworkEnabledLogicalElementCapabilities_Set_ElementNameEditSupported(&w, false);
    LMI_NetworkEnabledLogicalElementCapabilities_Init_RequestedStatesSupported(&w, 0);
    LMI_NetworkEnabledLogicalElementCapabilities_Set_ElementNameEditSupported(&w, false);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_NetworkEnabledLogicalElementCapabilities_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    // Capabilities for LMI_LANEndpoint
    LMI_NetworkEnabledLogicalElementCapabilities_Init(&w, _cb, KNameSpace(cop));
    LMI_NetworkEnabledLogicalElementCapabilities_Set_InstanceID(&w, ORGID ":NetworkLANEnabledLogicalElementCapabilities");
    LMI_NetworkEnabledLogicalElementCapabilities_Set_ElementNameEditSupported(&w, false);
    LMI_NetworkEnabledLogicalElementCapabilities_Init_RequestedStatesSupported(&w, 2);
    LMI_NetworkEnabledLogicalElementCapabilities_Set_RequestedStatesSupported(&w, 0,
            LMI_NetworkEnabledLogicalElementCapabilities_RequestedStatesSupported_Enabled);
    LMI_NetworkEnabledLogicalElementCapabilities_Set_RequestedStatesSupported(&w, 1,
            LMI_NetworkEnabledLogicalElementCapabilities_RequestedStatesSupported_Disabled);
    LMI_NetworkEnabledLogicalElementCapabilities_Set_ElementNameEditSupported(&w, false);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_NetworkEnabledLogicalElementCapabilities_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }
    return res;
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesExecQuery(
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
    LMI_NetworkEnabledLogicalElementCapabilities,
    LMI_NetworkEnabledLogicalElementCapabilities,
    _cb,
    LMI_NetworkEnabledLogicalElementCapabilitiesInitialize(&mi, ctx))

static void LMI_NetworkEnabledLogicalElementCapabilitiesMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkEnabledLogicalElementCapabilitiesInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_NetworkEnabledLogicalElementCapabilities_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_NetworkEnabledLogicalElementCapabilities,
    LMI_NetworkEnabledLogicalElementCapabilities,
    _cb,
    LMI_NetworkEnabledLogicalElementCapabilitiesMethodInitialize(&mi, ctx))

KUint16 LMI_NetworkEnabledLogicalElementCapabilities_CreateGoalSettings(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkEnabledLogicalElementCapabilitiesRef* self,
    const KInstanceA* TemplateGoalSettings,
    KInstanceA* SupportedGoalSettings,
    CMPIStatus* status)
{
    KUint16 result = KUINT16_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkEnabledLogicalElementCapabilities",
    "LMI_NetworkEnabledLogicalElementCapabilities",
    "instance method")
