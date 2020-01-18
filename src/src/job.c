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

#include "job.h"
#include "connection_private.h"
#include <time.h>
#include <string.h>

static int job_number = 0;

Job *job_new(JobType type)
{
    Job *job = malloc(sizeof(Job));
    if (job == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    job->id = job_number++;
    job->type = type;
    job->affected_elements = job_affected_elements_new(0);
    job->name = NULL;
    job->caption = NULL;
    job->delete_on_completion = true;
    job->state = JOB_STATE_QUEUED;
     // Default is 5 minutes (in microseconds)
    job->time_before_removal = 5 * 60 * 1000000;
    job->last_change_time = job->start_time = time(NULL);
    job->errors = job_errors_new(0);
    return job;
}

void job_add_error(Job *job, const char *error)
{
    job_errors_add(job->errors, strdup(error));
}

void job_add_affected_element(Job *job, JobAffectedElementType type, const char *id)
{
    job_affected_elements_add(job->affected_elements, job_affected_element_new(type, id));
}

void job_set_state(Job *job, JobState state)
{
    if (job->state != state) {
        job->state = state;
        job->last_change_time = time(NULL);
    }
}

void job_free(Job *job)
{
    if (job == NULL) {
        return;
    }
    job_affected_elements_free(job->affected_elements, true);
    job_errors_free(job->errors, true);
    free(job->name);
    free(job->caption);
    free(job);
}

LIST_IMPL(Job, job)

#define job_error_free free
LIST_IMPL(JobError, job_error)

JobAffectedElement *job_affected_element_new(JobAffectedElementType type, const char *id)
{
    JobAffectedElement *affected_element = malloc(sizeof(JobAffectedElement));
    if (affected_element == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    affected_element->type = type;
    affected_element->id = strdup(id);
    if (affected_element->id == NULL) {
        error("Memory allocation failed");
        free(affected_element);
        return NULL;
    }
    return affected_element;
}

void job_affected_element_free(JobAffectedElement *element)
{
    if (element == NULL) {
        return;
    }
    free(element->id);
    free(element);
}

LIST_IMPL(JobAffectedElement, job_affected_element)

JobAffectedElement *job_affected_elements_find_by_type(const JobAffectedElements *affected, JobAffectedElementType type)
{
    if (affected == NULL) {
        return NULL;
    }
    JobAffectedElement *element;
    for (size_t i = 0; i < job_affected_elements_length(affected); ++i) {
        element = job_affected_elements_index(affected, i);
        if (element->type == type) {
            return element;
        }
    }
    return NULL;
}
