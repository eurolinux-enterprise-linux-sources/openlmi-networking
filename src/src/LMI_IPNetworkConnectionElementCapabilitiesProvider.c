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
#include "LMI_IPNetworkConnectionElementCapabilities.h"
#include "globals.h"
#include "network.h"
#include "port.h"

static const CMPIBroker* _cb;

static void LMI_IPNetworkConnectionElementCapabilitiesInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);
    char *instanceid;

    Port *port;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);

        LMI_IPNetworkConnectionRef managedElement;
        LMI_IPNetworkConnectionRef_Init(&managedElement, _cb, ns);
        LMI_IPNetworkConnectionRef_Set_CreationClassName(&managedElement, LMI_IPNetworkConnection_ClassName);
        LMI_IPNetworkConnectionRef_Set_Name(&managedElement, port_get_id(port));
        LMI_IPNetworkConnectionRef_Set_SystemCreationClassName(&managedElement, get_system_creation_class_name());
        LMI_IPNetworkConnectionRef_Set_SystemName(&managedElement, get_system_name());

        LMI_IPNetworkConnectionCapabilitiesRef capabilities;
        LMI_IPNetworkConnectionCapabilitiesRef_Init(&capabilities, _cb, ns);
        instanceid = id_to_instanceid(port_get_id(port),
                LMI_IPNetworkConnectionCapabilities_ClassName);
        LMI_IPNetworkConnectionCapabilitiesRef_Set_InstanceID(&capabilities, instanceid);
        free(instanceid);

        LMI_IPNetworkConnectionElementCapabilities w;
        LMI_IPNetworkConnectionElementCapabilities_Init(&w, _cb, ns);
        LMI_IPNetworkConnectionElementCapabilities_Set_ManagedElement(&w, &managedElement);
        LMI_IPNetworkConnectionElementCapabilities_Set_Capabilities(&w, &capabilities);

        LMI_IPNetworkConnectionElementCapabilities_Init_Characteristics(&w, 1);
        LMI_IPNetworkConnectionElementCapabilities_Set_Characteristics(&w, 0,
                LMI_IPNetworkConnectionElementCapabilities_Characteristics_Current);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class "
                    LMI_IPNetworkConnectionElementCapabilities_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_IPNetworkConnectionElementCapabilitiesAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesAssociators(
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
        LMI_IPNetworkConnectionElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesAssociatorNames(
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
        LMI_IPNetworkConnectionElementCapabilities_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesReferences(
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
        LMI_IPNetworkConnectionElementCapabilities_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_IPNetworkConnectionElementCapabilitiesReferenceNames(
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
        LMI_IPNetworkConnectionElementCapabilities_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_IPNetworkConnectionElementCapabilities,
    LMI_IPNetworkConnectionElementCapabilities,
    _cb,
    LMI_IPNetworkConnectionElementCapabilitiesInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_IPNetworkConnectionElementCapabilities,
    LMI_IPNetworkConnectionElementCapabilities,
    _cb,
    LMI_IPNetworkConnectionElementCapabilitiesAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPNetworkConnectionElementCapabilities",
    "LMI_IPNetworkConnectionElementCapabilities",
    "instance association")
