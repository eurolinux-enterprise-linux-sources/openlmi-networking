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
#include "LMI_IPConfigurationService.h"
#include "LMI_IPProtocolEndpoint.h"
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_BondingMasterSettingData.h"
#include "LMI_BridgingMasterSettingData.h"
#include "LMI_IPNetworkConnection.h"
#include "LMI_NetworkJob.h"
#include "network.h"
#include "port.h"
#include "connection.h"
#include "setting.h"
#include "activeconnection.h"
#include "errors.h"

static const CMPIBroker* _cb = NULL;

CMPIObjectPath *NetworkJob_ObjectPath(Job *job, const char *ns)
{
    LMI_NetworkJobRef networkjob;
    LMI_NetworkJobRef_Init(&networkjob, _cb, ns);

    char *id = id_to_instanceid_with_index("Job", LMI_NetworkJob_ClassName, job->id);
    LMI_NetworkJobRef_Set_InstanceID(&networkjob, id);
    free(id);
    return LMI_NetworkJobRef_ToObjectPath(&networkjob, NULL);
}

static void LMI_IPConfigurationServiceInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPConfigurationServiceCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPConfigurationServiceEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_IPConfigurationServiceEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    const char *ns = KNameSpace(cop);

    LMI_IPConfigurationService w;
    LMI_IPConfigurationService_Init(&w, _cb, ns);
    LMI_IPConfigurationService_Set_SystemName(&w, get_system_name());
    LMI_IPConfigurationService_Set_SystemCreationClassName(&w, get_system_creation_class_name());
    LMI_IPConfigurationService_Set_CreationClassName(&w, LMI_IPConfigurationService_ClassName);
    LMI_IPConfigurationService_Set_Name(&w, LMI_IPConfigurationService_ClassName);

    if (!ReturnInstance(cr, w)) {
        error("Unable to return instance of class " LMI_IPConfigurationService_ClassName);
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
    }
    return res;
}

static CMPIStatus LMI_IPConfigurationServiceGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_IPConfigurationServiceCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPConfigurationServiceModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPConfigurationServiceDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_IPConfigurationServiceExecQuery(
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
    LMI_IPConfigurationService,
    LMI_IPConfigurationService,
    _cb,
    LMI_IPConfigurationServiceInitialize(&mi, ctx))

static void LMI_IPConfigurationServiceMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_IPConfigurationServiceMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_IPConfigurationServiceInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_IPConfigurationService_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_IPConfigurationService,
    LMI_IPConfigurationService,
    _cb,
    LMI_IPConfigurationServiceMethodInitialize(&mi, ctx))

KUint32 LMI_IPConfigurationService_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KUint16* RequestedState,
    KRef* Job,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_IPConfigurationService_ChangeAffectedElementsAssignedSequence(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRefA* ManagedElements,
    const KUint16A* AssignedSequence,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_IPConfigurationService_AddStaticIPv4Interface(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRef* Configuration,
    KRef* StaticSetting,
    const KString* Address,
    const KString* SubnetMask,
    const KString* Gateway,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

enum { DONT_CHANGE, YES, NO };

bool mode_to_iscurrent_isnext(int mode, int *iscurrent, int *isnext)
{

    switch (mode) {
        case 0:
        case 1:
            *isnext = YES;
            *iscurrent = YES;
            break;
        case 2:
            *isnext = YES;
            *iscurrent = DONT_CHANGE;
            break;
        case 3:
        case 4:
            *isnext = NO;
            *iscurrent = NO;
            break;
        case 5:
            *isnext = NO;
            *iscurrent = DONT_CHANGE;
            break;
        case 32768:
            *isnext = DONT_CHANGE;
            *iscurrent = YES;
            break;
        case 32769:
            *isnext = DONT_CHANGE;
            *iscurrent = NO;
            break;
        default:
            return false;
    }
    return true;
}

KUint32 LMI_IPConfigurationService_ApplySettingToIPNetworkConnection(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRef* SettingData,
    const KRef* IPVersionSettingData,
    const KRef* IPNetworkConnection,
    const KUint16* Mode,
    KRef* NetworkJob,
    CMPIStatus* status)
{
    int iscurrent = 0, isnext = 0;
    Network *network = mi->hdl;
    size_t i;
    KUint32 result = KUINT32_INIT;
    const char *ns = KChars(self->__base.ns);
    LMIResult res_current = LMI_SUCCESS, res_next = LMI_SUCCESS;

    Require(SettingData, "No SettingData has been specified", result, 2);
    Require(IPNetworkConnection, "No IPNetworkConnection has been specified", result, 2);

    if (!Mode->exists || Mode->null) {
        warn("No Mode has been specified, assuming 1");
        mode_to_iscurrent_isnext(1, &iscurrent, &isnext);
    } else {
        if (!mode_to_iscurrent_isnext(Mode->value, &iscurrent, &isnext)) {
            KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Mode has invalid value");
            return result;
        }
    }

    CIM_IPNetworkConnectionRef ipNetworkConnectionRef;
    CIM_IPNetworkConnectionRef_InitFromObjectPath(&ipNetworkConnectionRef, _cb, IPNetworkConnection->value);
    if (strcmp(ipNetworkConnectionRef.SystemName.chars, get_system_name()) != 0) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "IPNetworkConnection doesn't exists");
        return result;
    }
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port = NULL;
    for (i = 0; i < ports_length(ports); ++i) {
        if (strcmp(port_get_id(ports_index(ports, i)), ipNetworkConnectionRef.Name.chars) == 0) {
            port = ports_index(ports, i);
            break;
        }
    }

    LMI_IPAssignmentSettingDataRef settingDataRef;
    LMI_IPAssignmentSettingDataRef_InitFromObjectPath(&settingDataRef, _cb, SettingData->value);
    char *id = id_from_instanceid(settingDataRef.InstanceID.chars, LMI_IPAssignmentSettingData_ClassName);
    if (id == NULL) {
        id = id_from_instanceid(settingDataRef.InstanceID.chars, LMI_BridgingMasterSettingData_ClassName);
    }
    if (id == NULL) {
        id = id_from_instanceid(settingDataRef.InstanceID.chars, LMI_BondingMasterSettingData_ClassName);
    }
    if (id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Invalid InstanceID of " LMI_IPAssignmentSettingData_ClassName " instance");
        network_unlock(network);
        return result;
    }

    const Connections *connections = network_get_connections(network);
    Connection *connection = NULL;
    for (i = 0; i < connections_length(connections); ++i) {
        if (strcmp(connection_get_id(connections_index(connections, i)), id) == 0) {
            connection = connections_index(connections, i);
        }
    }
    free(id);

    if (port == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "IPNetworkConnection doesn't exists");
        network_unlock(network);
        return result;
    }

    if (connection == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "SettingData doesn't exists");
        network_unlock(network);
        return result;
    }

    Job *job = NULL;
    if (iscurrent == YES) {
        // connection should be made current
        res_current = network_activate_connection(network, port, connection, &job);
    } else if (iscurrent == NO) {
        const ActiveConnections *activeConnections = network_get_active_connections(network);
        // connection should be unmade current
        if (active_connections_is_connection_active_on_port(activeConnections, connection, port)) {
            res_current = port_disconnect(port);
        }
        KUint32_Set(&result, res_current);
    }

    if (res_current != LMI_SUCCESS && res_current != LMI_JOB_STARTED) {
        warn("Unable to (de)activate network connection (%d)", res_current);
        KSetStatus2(_cb, status, ERR_FAILED, iscurrent == YES ?
                    "Unable to activate network connection" :
                    "Unable to deactivate network connection");
        KUint32_Set(&result, res_current);
        network_unlock(network);
        return result;
    }

    if (isnext == YES) {
        // connection should be made next
        res_next = network_set_autoconnect(network, port, connection, true);
        KUint32_Set(&result, res_next);
    } else if (isnext == NO) {
        res_next = network_set_autoconnect(network, port, connection, false);
        KUint32_Set(&result, res_next);
    }

    if (res_next != LMI_SUCCESS) {
        warn("Unable to (un)set connection as next (%d)", res_next);
        KSetStatus2(_cb, status, ERR_FAILED, isnext == YES ?
                    "Unable to set network connection as next" :
                    "Unable to unset network connection as next");
        KUint32_Set(&result, res_next);
        network_unlock(network);
        return result;
    }

    if (res_current == LMI_JOB_STARTED) {
        KUint32_Set(&result, LMI_JOB_STARTED);
        KRef_SetObjectPath(NetworkJob, NetworkJob_ObjectPath(job, ns));
    }
    KSetStatus(status, OK);
    network_unlock(network);
    return result;
}

KUint32 LMI_IPConfigurationService_StartService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_IPConfigurationService_StopService(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_IPConfigurationService_ApplySettingToLANEndpoint(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRef* ConfigurationRef,
    const KRef* EndpointRef,
    KRef* NetworkJob,
    CMPIStatus* status)
{
    Network *network = mi->hdl;
    size_t i;
    KUint32 result = KUINT32_INIT;
    const char *ns = LMI_IPConfigurationServiceRef_NameSpace(
            (LMI_IPConfigurationServiceRef *) self);

    Require(ConfigurationRef, "No configuration has been specified", result, 2);
    Require(EndpointRef, "No endpoint has been specified", result, 2);

    CIM_LANEndpointRef lanEndpointRef;
    CIM_LANEndpointRef_InitFromObjectPath(&lanEndpointRef, _cb, EndpointRef->value);
    if (strcmp(lanEndpointRef.SystemName.chars, get_system_name()) != 0) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Endpoint doesn't exists");
        return result;
    }
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port = NULL;
    for (i = 0; i < ports_length(ports); ++i) {
        if (strcmp(port_get_id(ports_index(ports, i)), lanEndpointRef.Name.chars) == 0) {
            port = ports_index(ports, i);
            break;
        }
    }

    LMI_IPAssignmentSettingData settingDataRef;
    LMI_IPAssignmentSettingData_InitFromObjectPath(&settingDataRef, _cb, ConfigurationRef->value);
    char *id = id_from_instanceid(settingDataRef.InstanceID.chars, LMI_IPAssignmentSettingData_ClassName);
    if (id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Invalid InstanceID of " LMI_IPAssignmentSettingData_ClassName " instance");
        network_unlock(network);
        return result;
    }


    const Connections *connections = network_get_connections(network);
    Connection *connection = NULL;
    for (i = 0; i < connections_length(connections); ++i) {
        if (strcmp(connection_get_id(connections_index(connections, i)), id) == 0) {
            connection = connections_index(connections, i);
        }
    }
    free(id);

    if (port == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "LANEndpoint doesn't exists");
        network_unlock(network);
        return result;
    }

    if (connection == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Connection doesn't exists");
        network_unlock(network);
        return result;
    }

    Job *job = NULL;
    LMIResult res = network_activate_connection(network, port, connection, &job);
    if (res == LMI_SUCCESS) {
        KSetStatus(status, OK);
        KUint32_Set(&result, 0);
    } else if (res == LMI_JOB_STARTED) {
        KSetStatus(status, OK);
        KUint32_Set(&result, LMI_JOB_STARTED);
        KRef_SetObjectPath(NetworkJob, NetworkJob_ObjectPath(job, ns));
    } else {
        warn("Unable to activate network connection (%d)", result.value);
        KUint32_Set(&result, res);
        KSetStatus2(_cb, status, ERR_FAILED, "Unable to activate network connection");
    }
    network_unlock(network);
    return result;
}

KUint32 LMI_IPConfigurationService_ApplySettingToIPProtocolEndpoint(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRef* ConfigurationRef,
    const KRef* EndpointRef,
    KRef* NetworkJob,
    CMPIStatus* status)
{
    Network *network = mi->hdl;
    size_t i;
    KUint32 result = KUINT32_INIT;

    Require(ConfigurationRef, "No configuration has been specified", result, 2);
    Require(EndpointRef, "No endpoint has been specified", result, 2);

    LMI_IPProtocolEndpointRef protocolEndpointRef;
    LMI_IPProtocolEndpointRef_InitFromObjectPath(&protocolEndpointRef, _cb, EndpointRef->value);
    if (strcmp(protocolEndpointRef.SystemName.chars, get_system_name()) != 0) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Endpoint doesn't exists");
        return result;
    }
    if (!protocolEndpointRef.Name.exists || protocolEndpointRef.Name.null) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Invalid endpoint name");
        return result;
    }
    const char *p = strstr(protocolEndpointRef.Name.chars, "_");
    if (p == NULL) {
        error("Unknown endpoint name: %s", protocolEndpointRef.Name.chars);
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Unknown endpoint name");
        return result;
    }
    char *port_name = strndup(protocolEndpointRef.Name.chars, p - protocolEndpointRef.Name.chars);

    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Port *port = ports_find_by_id(ports, port_name);
    free(port_name);
    if (port == NULL) {
        error("Network port for endpoint %s doesn't exist", protocolEndpointRef.Name.chars);
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Network port for endpoint doesn't exist");
        network_unlock(network);
        return result;
    }

    LMI_IPAssignmentSettingData settingDataRef;
    LMI_IPAssignmentSettingData_InitFromObjectPath(&settingDataRef, _cb, ConfigurationRef->value);
    char *id = id_from_instanceid(settingDataRef.InstanceID.chars, LMI_IPAssignmentSettingData_ClassName);
    if (id == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Invalid InstanceID of " LMI_IPAssignmentSettingData_ClassName " instance");
        network_unlock(network);
        return result;
    }


    const Connections *connections = network_get_connections(network);
    Connection *connection = NULL;
    for (i = 0; i < connections_length(connections); ++i) {
        if (strcmp(connection_get_id(connections_index(connections, i)), id) == 0) {
            connection = connections_index(connections, i);
        }
    }
    free(id);

    if (port == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Endpoint doesn't exists");
        network_unlock(network);
        return result;
    }

    if (connection == NULL) {
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "Connection doesn't exists");
        network_unlock(network);
        return result;
    }

    Job *job = NULL;
    if ((result.value = network_activate_connection(network, port, connection, &job)) != 0) {
        warn("Unable to activate network connection (%d)", result.value);
        KSetStatus2(_cb, status, ERR_FAILED, "Unable to activate network connection");
        network_unlock(network);
        return result;
    }

    KSetStatus(status, OK);
    KUint32_Set(&result, 0);
    network_unlock(network);
    return result;
}

KUint32 LMI_IPConfigurationService_ApplySettingToComputerSystem(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_IPConfigurationServiceRef* self,
    const KRef* IPVersionSettingData,
    const KRef* ComputerSystem,
    const KUint16* Mode,
    KRef* Job,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_IPConfigurationService",
    "LMI_IPConfigurationService",
    "instance method")
