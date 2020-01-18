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
#include "globals.h"
#include "network.h"
#include "connection.h"
#include "indications.h"
#include "LMI_NetworkInstModification.h"
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_NetworkJob.h"
#include "ipassignmentsettingdata.h"
#include "ipnetworkconnection.h"
#include "network_job.h"

static const CMPIBroker* _cb = NULL;

void *connection_pre_changed_callback(Network *network, Connection *connection, void *data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Get instance of changed connection
    LMI_IPAssignmentSettingData sd;
    LMI_IPAssignmentSettingData_Init(&sd, _cb, ns);
    rc = connection_to_IPAssignmentSettingData(connection, &sd);
    if (!KOkay(rc)) {
        error("Unable to convert port to "
              LMI_IPAssignmentSettingData_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;
    }

    CMPIInstance *inst = LMI_IPAssignmentSettingData_ToInstance(&sd, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_IPAssignmentSettingData_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;
    }
    return inst;
}

void connection_changed_callback(Network *network, Connection *connection, void *data, void *pre_data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Get instance of changed connection
    LMI_IPAssignmentSettingData sd;
    LMI_IPAssignmentSettingData_Init(&sd, _cb, ns);
    rc = connection_to_IPAssignmentSettingData(connection, &sd);
    if (!KOkay(rc)) {
        error("Unable to convert port to "
              LMI_IPAssignmentSettingData_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    CMPIInstance *inst = LMI_IPAssignmentSettingData_ToInstance(&sd, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_IPAssignmentSettingData_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstModification_ClassName, inst, (CMPIInstance *) pre_data);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

void *port_pre_changed_callback(Network *network, Port *port, void *data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Get instance of changed port
    LMI_IPNetworkConnection ip;
    LMI_IPNetworkConnection_Init(&ip, _cb, ns);
    rc = port_to_IPNetworkConnection(port, &ip, network_get_background_context(network));
    if (!KOkay(rc)) {
        error("Unable to convert port to "
              LMI_IPNetworkConnection_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;

    }
    CMPIInstance *inst = LMI_IPNetworkConnection_ToInstance(&ip, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_IPNetworkConnection_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;
    }
    return inst;
}

void port_changed_callback(Network *network, Port *port, void *data, void *pre_data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Get instance of changed port
    LMI_IPNetworkConnection ip;
    LMI_IPNetworkConnection_Init(&ip, _cb, ns);
    rc = port_to_IPNetworkConnection(port, &ip, network_get_background_context(network));
    if (!KOkay(rc)) {
        error("Unable to convert port to "
              LMI_IPNetworkConnection_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    CMPIInstance *inst = LMI_IPNetworkConnection_ToInstance(&ip, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_IPAssignmentSettingData_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstModification_ClassName, inst, (CMPIInstance *) pre_data);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

void *job_pre_changed_callback(Network *network, Job *job, void *data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Create indication for added LMI_NetworkJob
    LMI_NetworkJob nj;
    LMI_NetworkJob_Init(&nj, _cb, ns);
    rc = job_to_NetworkJob(_cb, job, &nj);
    if (!KOkay(rc)) {
        error("Unable to convert job to "
              LMI_NetworkJob_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;
    }
    CMPIInstance *inst = LMI_NetworkJob_ToInstance(&nj, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_NetworkJob_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return NULL;
    }
    return inst;
}

void job_changed_callback(Network *network, Job *job, void *data, void *pre_data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Create indication for added LMI_NetworkJob
    LMI_NetworkJob nj;
    LMI_NetworkJob_Init(&nj, _cb, ns);
    rc = job_to_NetworkJob(_cb, job, &nj);
    if (!KOkay(rc)) {
        error("Unable to convert job to "
              LMI_NetworkJob_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
    CMPIInstance *inst = LMI_NetworkJob_ToInstance(&nj, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_NetworkJob_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstModification_ClassName, inst, (CMPIInstance *) pre_data);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

static void LMI_NetworkInstModificationInitialize(void)
{
}

static CMPIStatus LMI_NetworkInstModificationIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstModificationAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns,
    const CMPIObjectPath* op,
    const char* user)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstModificationMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns, 
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkInstModificationActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* cn,
    const CMPIObjectPath* op,
    CMPIBoolean firstActivation)
{
    debug(LMI_NetworkInstModification_ClassName " %s", cn);
    if (strcmp(cn, LMI_NetworkInstModification_ClassName) != 0) {
        // Filter for some other class
        CMReturn(CMPI_RC_OK);
    }
    debug("LMI_NetworkInstModification::ActivateFilter: %s", KChars(se->ft->getString(se, NULL)));

    mi->hdl = network_ref(_cb, cc);
    Network *network = mi->hdl;
    network_lock(network);
    char *nameSpace = strdup(KChars(CMGetNameSpace(op, NULL)));
    if (nameSpace == NULL) {
        error("Memory allocation failed");
        network_unlock(network);
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    network_set_connection_pre_changed_callback(network, connection_pre_changed_callback, nameSpace);
    network_set_connection_changed_callback(network, connection_changed_callback, nameSpace);
    network_set_port_pre_changed_callback(network, port_pre_changed_callback, nameSpace);
    network_set_port_changed_callback(network, port_changed_callback, nameSpace);
    network_set_job_pre_changed_callback(network, job_pre_changed_callback, nameSpace);
    network_set_job_changed_callback(network, job_changed_callback, nameSpace);
    network_unlock(network);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstModificationDeActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* cn,
    const CMPIObjectPath* op,
    CMPIBoolean lastActivation)
{
    debug("NetworkInstModification::DeActivateFilter");
    if (mi->hdl == NULL) {
        CMReturn(CMPI_RC_OK);
    }
    Network *network = mi->hdl;
    network_lock(network);
    network_set_connection_pre_changed_callback(network, NULL, NULL);
    network_set_connection_changed_callback(network, NULL, NULL);
    network_set_port_pre_changed_callback(network, NULL, NULL);
    network_set_port_changed_callback(network, NULL, NULL);
    network_set_job_pre_changed_callback(network, NULL, NULL);
    network_set_job_changed_callback(network, NULL, NULL);
    network_unlock(network);
    network_unref(network);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstModificationEnableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstModificationDisableIndications(
    CMPIIndicationMI* mi,
    const CMPIContext* cc)
{
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_NetworkInstModification,
    LMI_NetworkInstModification, 
    _cb, 
    LMI_NetworkInstModificationInitialize())

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkInstModification",
    "LMI_NetworkInstModification",
    "indication")
