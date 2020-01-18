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

#ifndef NETWORK_PRIVATE_H
#define NETWORK_PRIVATE_H

#include "job.h"

struct Network
{
    /** Number of references to this structure, free if zero */
    int ref_count;
    /** Pointer to backend specific private data */
    void *priv;
    pthread_mutex_t mutex;
    pthread_t thread;
    struct Ports *ports;
    struct Connections *connections;
    struct ActiveConnections *activeConnections;
    GMainLoop *thread_loop;
    volatile int loaded;
    const CMPIBroker *broker;
    const CMPIContext *master_context;
    const CMPIContext *background_context;

    // Callbacks
    void (*connection_added_callback)(Network *, Connection *, void *);
    void *connection_added_callback_data;

    void *(*connection_pre_changed_callback)(Network *, Connection *, void *);
    void *connection_pre_changed_callback_data;

    void (*connection_changed_callback)(Network *, Connection *, void *, void *);
    void *connection_changed_callback_data;

    void (*connection_deleted_callback)(Network *, Connection *, void *);
    void *connection_deleted_callback_data;

    void (*port_added_callback)(Network *, Port *, void *);
    void *port_added_callback_data;

    void *(*port_pre_changed_callback)(Network *, Port *, void *);
    void *port_pre_changed_callback_data;

    void (*port_changed_callback)(Network *, Port *, void *, void *);
    void *port_changed_callback_data;

    void (*port_deleted_callback)(Network *, Port *, void *);
    void *port_deleted_callback_data;

    void (*job_added_callback)(Network *, Job *, void *);
    void *job_added_callback_data;

    void *(*job_pre_changed_callback)(Network *, Job *, void *);
    void *job_pre_changed_callback_data;

    void (*job_changed_callback)(Network *, Job *, void *, void *);
    void *job_changed_callback_data;

    void (*job_deleted_callback)(Network *, Job *, void *);
    void *job_deleted_callback_data;

    // Jobs
    Jobs *jobs;
};

enum {
    /** Network devices has been loaded, wait if not */
    NETWORK_DEVICES_LOADED = 0x01,
    /** Network settings has been loaded, wait if not */
    NETWORK_SETTINGS_LOADED = 0x02
};

#define MUTEX_LOCK(network) pthread_mutex_lock(&network->mutex)
#define MUTEX_UNLOCK(network) pthread_mutex_unlock(&network->mutex)

void *network_priv_new(Network *);
void network_priv_free(void *);

LMIResult network_priv_get_devices(Network *network);
LMIResult network_priv_get_active_connections(Network *network);

LMIResult network_priv_activate_connection(Network *network, const Port *port, const Connection *connection, Job **job);

LMIResult network_priv_create_connection(Network *network, Connection *connection);
LMIResult network_priv_delete_connection(Network *network, Connection *connection);

#endif
