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
#include "LMI_HostedIPConfigurationService.h"
#include "network.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_HostedIPConfigurationServiceInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_HostedIPConfigurationServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedIPConfigurationServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_HostedIPConfigurationServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    const char *ns = KNameSpace(cop);

    CMPIObjectPath *ipConfigurationServiceOP = CIM_IPConfigurationServiceRefOP(_cb, cc, ns);

    LMI_HostedIPConfigurationService w;
    LMI_HostedIPConfigurationService_Init(&w, _cb, ns);
    LMI_HostedIPConfigurationService_SetObjectPath_Antecedent(&w, lmi_get_computer_system_safe(cc));
    LMI_HostedIPConfigurationService_SetObjectPath_Dependent(&w, ipConfigurationServiceOP);

    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_HostedIPConfigurationService_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }

    return res;
}

static CMPIStatus LMI_HostedIPConfigurationServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_HostedIPConfigurationServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedIPConfigurationServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedIPConfigurationServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_HostedIPConfigurationServiceExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_HostedIPConfigurationServiceAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}


static CMPIStatus LMI_HostedIPConfigurationServiceAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_HostedIPConfigurationServiceAssociators(
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
        LMI_HostedIPConfigurationService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_HostedIPConfigurationServiceAssociatorNames(
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
        LMI_HostedIPConfigurationService_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_HostedIPConfigurationServiceReferences(
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
        LMI_HostedIPConfigurationService_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_HostedIPConfigurationServiceReferenceNames(
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
        LMI_HostedIPConfigurationService_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_HostedIPConfigurationService,
    LMI_HostedIPConfigurationService,
    _cb,
    LMI_HostedIPConfigurationServiceInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_HostedIPConfigurationService,
    LMI_HostedIPConfigurationService,
    _cb,
    LMI_HostedIPConfigurationServiceAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_HostedIPConfigurationService",
    "LMI_HostedIPConfigurationService",
    "instance association")
