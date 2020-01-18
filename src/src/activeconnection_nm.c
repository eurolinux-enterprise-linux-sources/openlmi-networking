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

#include "activeconnection_nm.h"

#include <string.h>
#include <sys/socket.h>

#include "activeconnection.h"
#include "connection.h"
#include "dbus_wrapper.h"
#include "port.h"

#include "network_nm.h"

#include "activeconnection_private.h"
#include "network_private.h"
#include "errors.h"

LMIResult active_connection_read_properties(ActiveConnection *activeConnection, GHashTable *properties);
void active_connection_changed_cb(void *proxy, GHashTable *properties, ActiveConnection *activeConnection);

ActiveConnectionStatus nm_state_to_status(unsigned int state) {
    switch (state) {
        case NM_ACTIVE_CONNECTION_STATE_UNKNOWN:
            return ACTIVE_CONNECTION_STATE_UNKNOWN;
        case NM_ACTIVE_CONNECTION_STATE_ACTIVATING:
            return ACTIVE_CONNECTION_STATE_ACTIVATING;
        case NM_ACTIVE_CONNECTION_STATE_ACTIVATED:
            return ACTIVE_CONNECTION_STATE_ACTIVATED;
        case NM_ACTIVE_CONNECTION_STATE_DEACTIVATING:
            return ACTIVE_CONNECTION_STATE_DEACTIVATING;
        case NM_ACTIVE_CONNECTION_STATE_DEACTIVATED:
            return ACTIVE_CONNECTION_STATE_DEACTIVATED;
    }
    return ACTIVE_CONNECTION_STATE_UNKNOWN;
}

ActiveConnection *active_connection_from_objectpath(Network *network, const char *objectpath, LMIResult *res)
{
    ActiveConnection *activeConnection = NULL;
    activeConnection = active_connection_new(network);
    if (activeConnection == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }
    if ((activeConnection->uuid = strdup(objectpath)) == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }

    DBusGProxy *proxy = dbus_g_proxy_new_for_name(network_priv_get_dbus_connection(network), NM_SERVICE_DBUS, objectpath, NM_DBUS_INTERFACE_ACTIVE_CONNECTION);
    if (proxy == NULL) {
        error("Unable to create DBus proxy: %s %s NM_DBUS_INTERFACE_ACTIVE_CONNECTION", NM_SERVICE_DBUS, objectpath);
        *res = LMI_ERROR_BACKEND;
        goto err;
    }

    dbus_g_proxy_add_signal(proxy, "PropertiesChanged", DBUS_TYPE_G_MAP_OF_VARIANT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(proxy, "PropertiesChanged", G_CALLBACK(active_connection_changed_cb), activeConnection, NULL);

    GHashTable *properties = dbus_get_properties(proxy, objectpath, NM_DBUS_INTERFACE_ACTIVE_CONNECTION);
    if (properties == NULL) {
        error("Unable to get properties for object %s", objectpath);
        *res = LMI_ERROR_BACKEND;
        goto err;
    }

    active_connection_read_properties(activeConnection, properties);
    return activeConnection;
err:
    active_connection_free(activeConnection);
    return NULL;
}

LMIResult active_connection_read_properties(ActiveConnection *activeConnection, GHashTable *properties)
{
    LMIResult res = LMI_SUCCESS;
    GPtrArray *array = dbus_property_array(properties, "Devices");
    if (array != NULL && array->len != 0) {
        // When activeConnection is disconnected, it has the list of devices cleared
        // But we want to read the state of the devices
        char *devicepath;
        Port *port;
        ports_free(activeConnection->ports, false);
        activeConnection->ports = ports_new(array->len);
        for (guint i = 0; i < array->len; ++i) {
            devicepath = (char *) g_ptr_array_index(array, i);
            port = ports_find_by_uuid(activeConnection->network->ports, devicepath);
            if (port == NULL) {
                warn("No such port: %s", devicepath);
                continue;
            }
            if ((res = ports_add(activeConnection->ports, port)) != LMI_SUCCESS) {
                error("Unable to add port to activeConnection");
                break;
            }
        }
    }
    const char *connectionpath = dbus_property_objectpath(properties, "Connection");
    if (connectionpath != NULL) {
        Connection *connection = connections_find_by_uuid(activeConnection->network->connections, connectionpath);
        if (connection != NULL) {
            activeConnection->connection = connection;
        } else {
            warn("No such connection: %s", connectionpath);
        }
    }
    GValue *v = g_hash_table_lookup(properties, "State");
    if (v != NULL) {
        debug("ActiveConnection %s state %d", activeConnection->uuid, g_value_get_uint(v));
        activeConnection->status = nm_state_to_status(g_value_get_uint(v));
    }
    return res;
}

void active_connection_changed_cb(void *proxy, GHashTable *properties, ActiveConnection *activeConnection)
{
    Network *network = activeConnection->network;
    network_lock(network);
    // Check if we have job that monitors the activeConnection
    Job *job;
    Jobs *affected_jobs = jobs_new(0);
    JobAffectedElement *element;
    size_t i, j;
    for (i = 0; i < jobs_length(network->jobs); ++i) {
        job = jobs_index(network->jobs, i);
        if (job->state == JOB_STATE_RUNNING &&
            job->type == JOB_TYPE_APPLY_SETTING_DATA) {

            element = job_affected_elements_find_by_type(job->affected_elements, JOB_AFFECTED_ACTIVE_CONNECTION_ID);
            if (element != NULL && strcmp(element->id, activeConnection->uuid) == 0) {
                jobs_add(affected_jobs, job);
            }
        }
    }

    // Call prechanged callback
    void **job_data = malloc(sizeof(void *) * jobs_length(affected_jobs));
    for (i = 0; i < jobs_length(affected_jobs); ++i) {
        job = jobs_index(affected_jobs, i);
        if (network->job_pre_changed_callback != NULL) {
            job_data[i] = network->job_pre_changed_callback(network, job,
                    network->job_pre_changed_callback_data);
        }
    }

    // Read the changed properties
    active_connection_read_properties(activeConnection, properties);

    // Update the job and call postchanged callback
    Port *port;
    const char *reason;
    for (i = 0; i < jobs_length(affected_jobs); ++i) {
        job = jobs_index(affected_jobs, i);

        switch (activeConnection->status) {
            case ACTIVE_CONNECTION_STATE_ACTIVATED:
                job_set_state(job, JOB_STATE_FINISHED_OK);
                break;
            case ACTIVE_CONNECTION_STATE_ACTIVATING:
                job_set_state(job, JOB_STATE_RUNNING);
                break;
            case ACTIVE_CONNECTION_STATE_DEACTIVATING:
            case ACTIVE_CONNECTION_STATE_DEACTIVATED:
                job_set_state(job, JOB_STATE_FAILED);
                for (j = 0; j < ports_length(activeConnection->ports); ++j) {
                    port = ports_index(activeConnection->ports, j);
                    reason = port_get_state_reason(port);
                    job_add_error(job, reason != NULL ?
                                  reason :
                                  "Uknown error");
                }
                break;
            case ACTIVE_CONNECTION_STATE_UNKNOWN:
                job_set_state(job, JOB_STATE_FAILED);
                break;
        }

        if (network->job_changed_callback != NULL) {
            network->job_changed_callback(network, job,
                    network->job_changed_callback_data, job_data[i]);
        }
    }
    jobs_free(affected_jobs, false);
    free(job_data);
    network_unlock(network);
}
