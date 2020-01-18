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

#include "globals.h"

#include <konkret/konkret.h>
#include "LMI_NetworkJob.h"
#include "CIM_Error.h"
#include "network.h"
#include "nm_support.h"
#include "network_job.h"

static const CMPIBroker* _cb = NULL;

static void LMI_NetworkJobInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkJobCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkJobEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_NetworkJobEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);
    network_lock(network);

    network_cleanup_jobs(network);

    const Jobs *jobs = network_get_jobs(network);
    Job *job;
    for (size_t i = 0; i < jobs_length(jobs); ++i) {
        job = jobs_index(jobs, i);
        LMI_NetworkJob w;
        LMI_NetworkJob_Init(&w, _cb, ns);
        if (!KOkay(res = job_to_NetworkJob(_cb, job, &w))) {
            error("Unable to convert job to "
                  LMI_NetworkJob_ClassName
                  ": %d %s", res.rc, KChars(res.msg));
            break;
        }

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class "
                  LMI_NetworkJob_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_NetworkJobGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_NetworkJobCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkJobModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    debug(LMI_NetworkJob_ClassName " ModifyInstance");
    CMPIStatus res = { CMPI_RC_OK, NULL };
    LMI_NetworkJobRef ref;
    if (!KOkay(LMI_NetworkJobRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_NetworkJob_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }
    LMI_NetworkJob w;
    if (!KOkay(LMI_NetworkJob_InitFromInstance(&w, _cb, ci))) {
        warn("Unable to convert instance to " LMI_NetworkJob_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    Network *network = mi->hdl;
    size_t index;
    char *id = id_from_instanceid_with_index(w.InstanceID.chars, LMI_NetworkJob_ClassName, &index);
    if (strcmp(id, "Job") != 0) {
        warn("Invalid job InstanceID");
        free(id);
        KReturn(ERR_INVALID_PARAMETER);
    }
    free(id);

    network_lock(network);
    const Jobs *jobs = network_get_jobs(network);
    Job *job = NULL;
    for (size_t i = 0; i < jobs_length(jobs); ++i) {
        if (jobs_index(jobs, i)->id == index) {
            job = jobs_index(jobs, i);
        }
    }
    if (job == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "No job with index: %ld", index);
    }

    if (w.Caption.exists && !w.Caption.null) {
        if ((job->caption = strdup(w.Caption.chars)) != NULL) {
            error("Memory allocation failed");
            network_unlock(network);
            KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
        }
    }

    if (w.TimeBeforeRemoval.exists && !w.TimeBeforeRemoval.null) {
        CMPIUint64 dt = CMGetBinaryFormat(w.TimeBeforeRemoval.value, &res);
        if (!KOkay(res)) {
            network_unlock(network);
            KReturn2(_cb, ERR_FAILED, "Unable to get time interval from TimeBeforeRemoval");
        }
        job->time_before_removal = dt;
    }

    if (w.DeleteOnCompletion.exists && !w.DeleteOnCompletion.null) {
        job->delete_on_completion = w.DeleteOnCompletion.value;
    }
    // Job is modified, set last changed time
    job->last_change_time = time(NULL);
    network_unlock(network);
    if (!KOkay(res)) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkJobDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_NetworkJobExecQuery(
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
    LMI_NetworkJob,
    LMI_NetworkJob,
    _cb,
    LMI_NetworkJobInitialize(&mi, ctx))

static void LMI_NetworkJobMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_NetworkJobMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_NetworkJobInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_NetworkJob_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_NetworkJob,
    LMI_NetworkJob,
    _cb,
    LMI_NetworkJobMethodInitialize(&mi, ctx))

KUint32 LMI_NetworkJob_KillJob(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    const KBoolean* DeleteOnKill,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_NetworkJob_RequestStateChange(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    const KUint16* RequestedState,
    const KDateTime* TimeoutPeriod,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_NetworkJob_GetError(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    KInstance* Error,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_NetworkJob_GetErrors(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    KInstanceA* Errors,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;
    Network *network = mi->hdl;
    const char *ns = LMI_NetworkJobRef_NameSpace((LMI_NetworkJobRef *) self);

    size_t index;
    // We don't care about the id, only index matters
    free(id_from_instanceid_with_index(self->InstanceID.chars, LMI_NetworkJob_ClassName, &index));


    network_lock(network);
    const Jobs *jobs = network_get_jobs(network);
    const Job *job = NULL;
    for (size_t i = 0; i < jobs_length(jobs); ++i) {
        if (index == jobs_index(jobs, i)->id) {
            job = jobs_index(jobs, i);
        }
    }
    if (job == NULL) {
        error("No such job: %s", self->InstanceID.chars);
        network_unlock(network);
        KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, "No such job exists");
        KUint32_Set(&result, 5); // Invalid parameter
        return result;
    }

    size_t nr_job_errors = job_errors_length(job->errors);
    debug("NetworkJob_GetErrors: %ld %s", nr_job_errors, ns);
    JobError *joberror;
    KInstanceA_Init(Errors, _cb, nr_job_errors);

    for (size_t i = 0; i < nr_job_errors; ++i) {
        joberror = job_errors_index(job->errors, i);
        debug("Job error: %s", (char *) joberror);
        CIM_Error w;
        CIM_Error_Init(&w, _cb, ns);
        CIM_Error_Set_Message(&w, (char *) joberror);

        KInstanceA_Set(Errors, i, CIM_Error_ToInstance(&w, NULL));
    }
    KUint32_Set(&result, 0);
    network_unlock(network);
    return result;
}

KUint32 LMI_NetworkJob_ResumeWithAction(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KUint32 LMI_NetworkJob_ResumeWithInput(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_NetworkJobRef* self,
    const KStringA* Inputs,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_NetworkJob",
    "LMI_NetworkJob",
    "instance method")
