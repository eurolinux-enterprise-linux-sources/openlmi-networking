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
#include "LMI_NetworkRemoteAccessAvailableToElement.h"
#include "LMI_NetworkRemoteServiceAccessPoint.h"
#include "LMI_IPNetworkConnection.h"
#include "LMI_DNSProtocolEndpoint.h"
#include "network.h"
#include "activeconnection.h"
#include "connection.h"
#include "setting.h"
#include "port.h"
#include "ref_factory.h"

static const CMPIBroker* _cb;

static void LMI_NetworkRemoteAccessAvailableToElementInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementEnumInstances(
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
    char *name;
    Port *port;
    IPConfig *ipconfig;
    Address *address;

    LMI_NetworkRemoteAccessAvailableToElement w;
    LMI_NetworkRemoteAccessAvailableToElement_Init(&w, _cb, ns);

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        ipconfig = port_get_ipconfig(port);

        CMPIObjectPath *ipNetworkConnection = CIM_ServiceAccessPointRefOP(port_get_id(port), LMI_IPNetworkConnection_ClassName, _cb, ns);

        for (j = 0; j < addresses_length(ipconfig->addresses); ++j) {
            address = addresses_index(ipconfig->addresses, j);

            if (address->default_gateway == NULL) {
                continue;
            }
            asprintf(&name, "%s_gateway_%ld", port_get_id(port), j);

            LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Antecedent(&w,
                    CIM_ServiceAccessPointRefOP(name, LMI_NetworkRemoteServiceAccessPoint_ClassName, _cb, ns));
            LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Dependent(&w, ipNetworkConnection);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkRemoteAccessAvailableToElement_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Dependent(&w, lmi_get_computer_system());

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_NetworkRemoteAccessAvailableToElement_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }

            free(name);
        }

        if (dns_servers_length(ipconfig->dns_servers) > 0) {
            CMPIObjectPath *dnsProtocolEndpoint = CIM_ServiceAccessPointRefOP(port_get_id(port), LMI_DNSProtocolEndpoint_ClassName, _cb, ns);
            for (j = 0; j < dns_servers_length(ipconfig->dns_servers); ++j) {
                asprintf(&name, "%s_dns_%ld", port_get_id(port), j);

                LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Antecedent(&w,
                        CIM_ServiceAccessPointRefOP(name, LMI_NetworkRemoteServiceAccessPoint_ClassName, _cb, ns));
                LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Dependent(&w, dnsProtocolEndpoint);

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_NetworkRemoteAccessAvailableToElement_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }

                LMI_NetworkRemoteAccessAvailableToElement_SetObjectPath_Dependent(&w, lmi_get_computer_system());

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class " LMI_NetworkRemoteAccessAvailableToElement_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }

                free(name);
            }
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_NetworkRemoteAccessAvailableToElementAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementAssociators(
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
        LMI_NetworkRemoteAccessAvailableToElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementAssociatorNames(
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
        LMI_NetworkRemoteAccessAvailableToElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementReferences(
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
        LMI_NetworkRemoteAccessAvailableToElement_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_NetworkRemoteAccessAvailableToElementReferenceNames(
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
        LMI_NetworkRemoteAccessAvailableToElement_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_NetworkRemoteAccessAvailableToElement,
    LMI_NetworkRemoteAccessAvailableToElement,
    _cb,
    LMI_NetworkRemoteAccessAvailableToElementInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_NetworkRemoteAccessAvailableToElement,
    LMI_NetworkRemoteAccessAvailableToElement,
    _cb,
    LMI_NetworkRemoteAccessAvailableToElementAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkRemoteAccessAvailableToElement",
    "LMI_NetworkRemoteAccessAvailableToElement",
    "instance association")
