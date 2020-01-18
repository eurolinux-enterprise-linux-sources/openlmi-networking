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

#ifndef JOB_H
#define JOB_H

#include "globals.h"

typedef enum {
    JOB_TYPE_APPLY_SETTING_DATA
} JobType;

typedef enum {
    JOB_AFFECTED_ACTIVE_CONNECTION_ID,
    JOB_AFFECTED_PORT_ID,
    JOB_AFFECTED_CONNECTION_ID
} JobAffectedElementType;

typedef struct JobAffectedElement
{
    JobAffectedElementType type;
    char *id;
} JobAffectedElement;

typedef enum {
    JOB_STATE_QUEUED,
    JOB_STATE_RUNNING,
    JOB_STATE_FINISHED_OK,
    JOB_STATE_SUSPENDED,
    JOB_STATE_FAILED,
    JOB_STATE_TERMINATED
} JobState;

typedef char JobError;
typedef struct JobErrors JobErrors;
typedef struct JobAffectedElements JobAffectedElements;

typedef struct Job {
    size_t id;
    JobType type;
    char *caption;
    char *name;
    bool delete_on_completion;
    // Time before removal in mircoseconds
    uint64_t time_before_removal;
    time_t start_time;
    time_t last_change_time;
    JobAffectedElements *affected_elements;
    JobState state;
    JobErrors *errors;
} Job;
typedef struct Jobs Jobs;

Job *job_new(JobType type);
void job_free(Job *job);

void job_add_error(Job *job, const char *error);

void job_add_affected_element(Job *job, JobAffectedElementType type, const char *id);

void job_set_state(Job *job, JobState state);

LIST_DECL(Job, job)

LIST_DECL(JobError, job_error)

LIST_DECL(JobAffectedElement, job_affected_element)

JobAffectedElement *job_affected_element_new(JobAffectedElementType type, const char *id);
JobAffectedElement *job_affected_elements_find_by_type(const JobAffectedElements *affected, JobAffectedElementType type);

#endif
