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
#include "LMI_IPVersionSettingData.h"
#include "network.h"

static const CMPIBroker* _cb = NULL;

static void LMI_IPVersionSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPVersionSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPVersionSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPVersionSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    const char *ns = KNameSpace(cop);

    LMI_IPVersionSettingData w;
    LMI_IPVersionSettingData_Init(&w, _cb, ns);

    char *instanceid = id_to_instanceid("IPv4", LMI_IPVersionSettingData_ClassName);
    LMI_IPVersionSettingData_Set_InstanceID(&w, instanceid);
    free(instanceid);
    LMI_IPVersionSettingData_Set_Caption(&w, "IPv4");
    LMI_IPVersionSettingData_Set_ElementName(&w, "IPv4");
    LMI_IPVersionSettingData_Set_ProtocolIFType(&w, LMI_IPVersionSettingData_ProtocolIFType_IPv4);
    LMI_IPVersionSettingData_Set_ChangeableType(&w, LMI_IPVersionSettingData_ChangeableType_Not_Changeable___Persistent);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_IPVersionSettingData_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    instanceid = id_to_instanceid("IPv6", LMI_IPVersionSettingData_ClassName);
    LMI_IPVersionSettingData_Set_InstanceID(&w, instanceid);
    free(instanceid);
    LMI_IPVersionSettingData_Set_Caption(&w, "IPv6");
    LMI_IPVersionSettingData_Set_ElementName(&w, "IPv6");
    LMI_IPVersionSettingData_Set_ProtocolIFType(&w, LMI_IPVersionSettingData_ProtocolIFType_IPv6);
    LMI_IPVersionSettingData_Set_ChangeableType(&w, LMI_IPVersionSettingData_ChangeableType_Not_Changeable___Persistent);
    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_IPVersionSettingData_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    return res;
}

static CMPIStatus LMI_IPVersionSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPVersionSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPVersionSettingDataExecQuery(
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
    LMI_IPVersionSettingData,
    LMI_IPVersionSettingData,
    _cb,
    LMI_IPVersionSettingDataInitialize(&mi, ctx))

static void LMI_IPVersionSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPVersionSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPVersionSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_IPVersionSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_IPVersionSettingData,
    LMI_IPVersionSettingData,
    _cb,
    LMI_IPVersionSettingDataMethodInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPVersionSettingData",
    "LMI_IPVersionSettingData",
    "instance method")
