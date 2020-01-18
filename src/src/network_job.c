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

#include "network_job.h"
#include <konkret.h>

CMPIStatus job_to_NetworkJob(
    const CMPIBroker *cb,
    const Job *job,
    LMI_NetworkJob *w)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    char *id = id_to_instanceid_with_index("Job", LMI_NetworkJob_ClassName, job->id);
    if (id == NULL) {
        error("Memory allocation failed");
        CMSetStatus(&res, CMPI_RC_ERR_FAILED);
        return res;
    }
    LMI_NetworkJob_Set_InstanceID(w, id);

    LMI_NetworkJob_Set_Name(w, job->name);
    LMI_NetworkJob_Set_Caption(w, job->caption);
    LMI_NetworkJob_Set_DeleteOnCompletion(w, job->delete_on_completion);

    CMPIDateTime *dt = CMNewDateTimeFromBinary(cb, job->time_before_removal, true, &res);
    if (KOkay(res)) {
        LMI_NetworkJob_Set_TimeBeforeRemoval(w, dt);
    } else {
        error("Unable to convert time to CMPIDateTime");
    }

    switch (job->state) {
        case JOB_STATE_QUEUED:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_New);
            LMI_NetworkJob_Init_OperationalStatus(w, 1);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperationalStatus_Dormant);
            LMI_NetworkJob_Set_PercentComplete(w, 0);
            break;
        case JOB_STATE_RUNNING:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_Running);
            LMI_NetworkJob_Init_OperationalStatus(w, 1);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperationalStatus_OK);
            LMI_NetworkJob_Set_PercentComplete(w, 50);
            break;
        case JOB_STATE_FINISHED_OK:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_Completed);
            LMI_NetworkJob_Init_OperationalStatus(w, 2);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperationalStatus_OK);
            LMI_NetworkJob_Set_OperationalStatus(w, 1, LMI_NetworkJob_OperationalStatus_Completed);
            LMI_NetworkJob_Set_PercentComplete(w, 100);
            break;
        case JOB_STATE_SUSPENDED:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_Suspended);
            LMI_NetworkJob_Init_OperationalStatus(w, 1);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperationalStatus_OK);
            LMI_NetworkJob_Set_PercentComplete(w, 50);
            break;
        case JOB_STATE_FAILED:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_Exception);
            LMI_NetworkJob_Init_OperationalStatus(w, 2);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperationalStatus_Error);
            LMI_NetworkJob_Set_OperationalStatus(w, 1, LMI_NetworkJob_OperationalStatus_Completed);
            LMI_NetworkJob_Set_PercentComplete(w, 100);
            break;
        case JOB_STATE_TERMINATED:
            LMI_NetworkJob_Set_JobState(w, LMI_NetworkJob_JobState_Terminated);
            LMI_NetworkJob_Init_OperationalStatus(w, 1);
            LMI_NetworkJob_Set_OperationalStatus(w, 0, LMI_NetworkJob_OperatingStatus_Stopped);
            LMI_NetworkJob_Set_PercentComplete(w, 100);
            break;
    }
    return res;
}
