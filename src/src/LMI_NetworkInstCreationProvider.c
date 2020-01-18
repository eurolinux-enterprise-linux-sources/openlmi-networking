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
#include "LMI_NetworkInstCreation.h"
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_IPNetworkConnection.h"
#include "globals.h"
#include "network.h"
#include "connection.h"
#include "indications.h"
#include "ipassignmentsettingdata.h"
#include "ipnetworkconnection.h"
#include "nm_support.h"
#include "network_job.h"

static const CMPIBroker* _cb = NULL;

void connection_added_callback(Network *network, Connection *connection, void *data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Get instance of created connection
    LMI_IPAssignmentSettingData sd;
    LMI_IPAssignmentSettingData_Init(&sd, _cb, ns);
    rc = connection_to_IPAssignmentSettingData(connection, &sd);
    if (!KOkay(rc)) {
        error("Unable to convert connection to "
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

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstCreation_ClassName, inst, NULL);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

void port_added_callback(Network *network, Port *port, void *data)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    const char *ns = data;

    // Create indication for added LMI_IPNetworkConnection
    LMI_IPNetworkConnection ipp;
    LMI_IPNetworkConnection_Init(&ipp, _cb, ns);
    port_to_IPNetworkConnection(port, &ipp, network_get_background_context(network));
    if (!KOkay(rc)) {
        error("Unable to convert port to "
              LMI_IPNetworkConnection_ClassName
              ": %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
    CMPIInstance *inst = LMI_IPNetworkConnection_ToInstance(&ipp, &rc);
    if (!KOkay(rc)) {
        error("Unable to convert "
              LMI_IPNetworkConnection_ClassName
              " to instance: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstCreation_ClassName, inst, NULL);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

void job_added_callback(Network *network, Job *job, void *data)
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

    rc = CreateIndication(_cb, network_get_background_context(network), ns, LMI_NetworkInstCreation_ClassName, inst, NULL);
    if (!KOkay(rc)) {
        error("Delivering of indication failed: %d (%s)", rc.rc, KChars(rc.msg));
        return;
    }
}

static void LMI_NetworkInstCreationInitialize(void)
{
}

static CMPIStatus LMI_NetworkInstCreationIndicationCleanup(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstCreationAuthorizeFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* cn,
    const CMPIObjectPath* op,
    const char* user)
{
    debug("NetworkInstCreation::AuthorizeFilter: %s", KChars(se->ft->getString(se, NULL)));
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstCreationMustPoll(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* ns, 
    const CMPIObjectPath* op)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkInstCreationActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* cn,
    const CMPIObjectPath* op,
    CMPIBoolean firstActivation)
{
    debug(LMI_NetworkInstCreation_ClassName " %s", cn);
    if (strcmp(cn, LMI_NetworkInstCreation_ClassName) != 0) {
        // Filter for some other class
        CMReturn(CMPI_RC_OK);
    }
    debug("NetworkInstCreation::ActivateFilter: %s", KChars(se->ft->getString(se, NULL)));

    mi->hdl = network_ref(_cb, cc);
    Network *network = mi->hdl;
    network_lock(network);
    char *nameSpace = strdup(KChars(CMGetNameSpace(op, NULL)));
    if (nameSpace == NULL) {
        error("Memory allocation failed");
        network_unlock(network);
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    // check the filter and set only affected callbacks
    network_set_connection_added_callback(network, connection_added_callback, nameSpace);
    network_set_port_added_callback(network, port_added_callback, nameSpace);
    network_set_job_added_callback(network, job_added_callback, nameSpace);
    network_unlock(network);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstCreationDeActivateFilter(
    CMPIIndicationMI* mi,
    const CMPIContext* cc,
    const CMPISelectExp* se,
    const char* cn,
    const CMPIObjectPath* op,
    CMPIBoolean lastActivation)
{
    debug("NetworkInstCreation::DeActivateFilter");
    if (mi->hdl == NULL) {
        CMReturn(CMPI_RC_OK);
    }
    Network *network = mi->hdl;
    network_lock(network);
    network_set_connection_added_callback(network, NULL, NULL);
    network_set_port_added_callback(network, NULL, NULL);
    network_set_job_added_callback(network, NULL, NULL);
    network_unlock(network);
    network_unref(network);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstCreationEnableIndications(
    CMPIIndicationMI* mi, 
    const CMPIContext* cc)
{
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkInstCreationDisableIndications(
    CMPIIndicationMI* mi, 
    const CMPIContext* cc)
{
    CMReturn(CMPI_RC_OK);
}

CMIndicationMIStub(
    LMI_NetworkInstCreation,
    LMI_NetworkInstCreation, 
    _cb, 
    LMI_NetworkInstCreationInitialize())

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkInstCreation",
    "LMI_NetworkInstCreation",
    "indication")
