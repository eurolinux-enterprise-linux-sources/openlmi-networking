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

#include "network.h"

#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "activeconnection.h"
#include "connection.h"
#include "port.h"

#include "network_private.h"
#include "errors.h"
#include "connection_private.h"
#include "job.h"

static Network *_network = NULL;
static pthread_once_t network_is_initialized = PTHREAD_ONCE_INIT;

Port *network_port_by_mac(Network *network, const char *mac)
{
    if (mac == NULL) {
        return NULL;
    }
    Port *port;
    const char *portmac;
    for (size_t i = 0; i < ports_length(network->ports); ++i) {
        port = ports_index(network->ports, i);
        portmac = port_get_mac(port);
        if (portmac != NULL && strcmp(portmac, mac) == 0) {
            return port;
        }
    }
    return NULL;
}

static void *network_thread_start(void *data)
{
    Network *network = data;
    CBAttachThread(network->broker, network->background_context);
    network->priv = network_priv_new(network);
    network->thread_loop = g_main_loop_new(NULL, FALSE);

    // This mutex was locked before calling this function
    MUTEX_UNLOCK(network);

    g_main_loop_run(network->thread_loop);
    g_main_loop_unref(network->thread_loop);
    CBDetachThread(network->broker, network->background_context);
    return NULL;
}

static void network_new(void)
{
    fprintf(stderr, "network_new, pid: %d\n", getpid());
    Network *network;
    network = malloc(sizeof(Network));
    if (network == NULL) {
        error("Memory allocation failed");
        return;
    }
    network->broker = NULL;
    network->loaded = 0;
    network->ref_count = 0;
    network->ports = NULL;
    network->connections = NULL;
    network->activeConnections = NULL;
    network->connection_added_callback = NULL;
    network->connection_added_callback_data = NULL;
    network->connection_pre_changed_callback = NULL;
    network->connection_pre_changed_callback_data = NULL;
    network->connection_changed_callback = NULL;
    network->connection_changed_callback_data = NULL;
    network->connection_deleted_callback = NULL;
    network->connection_deleted_callback_data = NULL;
    network->port_added_callback = NULL;
    network->port_added_callback_data = NULL;
    network->port_pre_changed_callback = NULL;
    network->port_pre_changed_callback_data = NULL;
    network->port_changed_callback = NULL;
    network->port_changed_callback_data = NULL;
    network->port_deleted_callback = NULL;
    network->port_deleted_callback_data = NULL;
    network->job_added_callback = NULL;
    network->job_added_callback_data = NULL;
    network->job_pre_changed_callback = NULL;
    network->job_pre_changed_callback_data = NULL;
    network->job_changed_callback = NULL;
    network->job_changed_callback_data = NULL;
    network->job_deleted_callback = NULL;
    network->job_deleted_callback_data = NULL;
    network->jobs = jobs_new(0);
    network->master_context = NULL;
    network->background_context = NULL;
    pthread_mutex_init(&network->mutex, NULL);

    // Check if we're in testing mode
    lmi_testing = FALSE;
    char *env = getenv("LMI_NETWORKING_FAKE_NM");
    if (env != NULL && strcmp(env, "1") == 0) {
        lmi_testing = TRUE;
    }

    _network = network;
}

static void network_free(Network *network)
{
    if (network == NULL) {
        return;
    }
    g_main_loop_ref(network->thread_loop);
    g_main_loop_quit(network->thread_loop);
    g_main_loop_unref(network->thread_loop);

    void *res = NULL;
    pthread_join(network->thread, &res);
    pthread_mutex_destroy(&network->mutex);
    free(res);
    network_priv_free(network->priv);
    ports_free(network->ports, true);
    connections_free(network->connections, true);
    active_connections_free(network->activeConnections, true);
    jobs_free(network->jobs, true);
    free(network);
    network = NULL;
}

Network *network_ref(const CMPIBroker *broker, const CMPIContext *ctx)
{
    lmi_init("networking", broker, ctx, NULL);
    pthread_once(&network_is_initialized, network_new);

    // Initialization will lock the Network and unlock after it's initialized,
    // so the lock here will wait for initialization in the first run of network_ref
    MUTEX_LOCK(_network);
    _network->broker = broker;
    if (_network->master_context == NULL) {
        // Background thread is not yet running, fire it up
        _network->master_context = ctx;
        _network->background_context = CBPrepareAttachThread(broker, ctx);

        if (pthread_create(&_network->thread, NULL, network_thread_start, _network) > 0) {
            error("Unable to create background thread");
        }
        // Background thread will unlock the mutex after init is done
        // we're waiting for it here...
        MUTEX_LOCK(_network);
    }

    _network->ref_count++;
    MUTEX_UNLOCK(_network);
    return _network;
}

void network_unref(Network *network)
{
    if (network == NULL || _network == NULL) {
        network = _network = NULL;
        return;
    }

    MUTEX_LOCK(network);
    network->ref_count--;

    if (network->ref_count <= 0) {
        // This is the last user for Network struct, unlock is ok
        MUTEX_UNLOCK(network);
        network_free(network);
        network_is_initialized = PTHREAD_ONCE_INIT;
        network = _network = NULL;
    } else {
        MUTEX_UNLOCK(network);
    }
}

void network_lock(Network *network)
{
    MUTEX_LOCK(network);
}

void network_unlock(Network *network)
{
    MUTEX_UNLOCK(network);
}

const Ports *network_get_ports(Network *network)
{
    return network->ports;
}

const Connections *network_get_connections(Network *network)
{
    return network->connections;
}

LMIResult network_activate_connection(Network *network, const Port *port, const Connection *connection, Job **job)
{
    debug("network_activate_connection %s %s", port != NULL ? port_get_id(port) : "NULL", connection->port != NULL ? port_get_id(connection->port) : "NULL");
    ConnectionType type = connection_get_type(connection);
    if (port != NULL && (type == CONNECTION_TYPE_BOND || type == CONNECTION_TYPE_BRIDGE)) {
        // Don't use port when activating Master bridge/bond setting data
        port = NULL;
    }

    if (port != NULL && connection->port != NULL && !port_compare(port, connection->port)) {
        error("Port %s is not the same as port %s assigned to connection %s",
              port_get_id(port), port_get_id(connection->port), connection->id);
        return LMI_ERROR_CONNECTION_INVALID;
    }
    return network_priv_activate_connection(network, port, connection, job);
}

LMIResult network_deactivate_connection(Network *network, const ActiveConnection *activeConnection, Job **job)
{
    debug("network_deactivate_connection %s", active_connection_get_connection(activeConnection) != NULL ? connection_get_name(active_connection_get_connection(activeConnection)) : "NULL");
    return network_priv_deactivate_connection(network, activeConnection, job);
}

const ActiveConnections *network_get_active_connections(Network *network)
{
    if (network == NULL) {
        return NULL;
    }
    return network->activeConnections;
}

LMIResult network_create_connection(Network *network, Connection *connection)
{
    return network_priv_create_connection(network, connection);
}

LMIResult network_delete_connection(Network *network, Connection *connection)
{
    return network_priv_delete_connection(network, connection);
}

PortStats *network_get_ports_statistics_priv(Network *network, FILE *f, LMIResult *res)
{
    *res = LMI_SUCCESS;
    if (network == NULL) {
        *res = LMI_ERROR_UNKNOWN;
        return NULL;
    }
    assert(f != NULL);
    char *buffer = NULL;
    char *str;
    size_t n = 0;
    ssize_t read;
    Port *port;
    PortStats *stats = port_stats_new(ports_length(network->ports));
    if (stats == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        return NULL;
    }
    PortStat *stat;

    size_t linenr = 0;
    do {
        // first two lines are headers
        if (linenr < 2) {
            read = getline(&buffer, &n, f);
            if (read == -1) {
                // EOF when reading headers
                free(buffer);
                port_stats_free(stats, true);
                fclose(f);
                *res = LMI_ERROR_BACKEND;
                return NULL;
            }
            linenr++;
            continue;
        }
        // other lines are devices
        read = getdelim(&buffer, &n, ':', f);
        if (read < 1) {
            break;
        }
        // get rid of semicolon at the end
        str = buffer;
        str[read - 1] = '\0';
        // get rid of whilespaces at the beginning
        while (str[0] == ' ') str++;
        port = ports_find_by_id(network->ports, str);
        if (port == NULL) {
            // we don't care about this device, skip the line
            if (getline(&buffer, &n, f) == -1) {
                break;
            }
            continue;
        }

        // read the stats
        if ((read = getline(&buffer, &n, f)) == -1) {
            break;
        }
        stat = port_stat_new();
        if (sscanf(buffer, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
               &stat->rx_bytes, &stat->rx_packets, &stat->rx_errs,       &stat->rx_drop,
               &stat->rx_fifo,  &stat->rx_frame,   &stat->rx_compressed, &stat->rx_multicast,
               &stat->tx_bytes, &stat->tx_packets, &stat->tx_errs,       &stat->tx_drop,
               &stat->tx_fifo,  &stat->tx_colls,   &stat->tx_carrier,    &stat->tx_compressed) != 16) {

            warn("Wrong stats line: %s", buffer);
            port_stat_free(stat);
            break;
        }
        stat->port = port;
        port_stats_add(stats, stat);

        linenr++;
    } while (true);

    free(buffer);
    fclose(f);
    return stats;
}

PortStats *network_get_ports_statistics(Network *network, LMIResult *res)
{
    if (lmi_testing) {
        // This is just for testing purposes!
        PortStats *portStats;
        if ((portStats = port_stats_new(1)) == NULL) {
            return NULL;
        }
        PortStat *portStat;
        for (size_t i = 0; i < ports_length(network->ports); ++i) {
            portStat = port_stat_new();
            portStat->port = ports_index(network->ports, i);
            portStat->rx_bytes = 2;
            portStat->rx_packets = 4;
            portStat->rx_errs = 8;
            portStat->rx_drop = 16;
            portStat->rx_fifo = 32;
            portStat->rx_frame = 64;
            portStat->rx_compressed = 128;
            portStat->rx_multicast = 256;
            portStat->tx_bytes = 512;
            portStat->tx_packets = 1024;
            portStat->tx_errs = 2048;
            portStat->tx_drop = 4096;
            portStat->tx_fifo = 8192;
            portStat->tx_colls = 16384;
            portStat->tx_carrier = 32768;
            portStat->tx_compressed = 65536;
            if (port_stats_add(portStats, portStat)) {
                port_stats_free(portStats, true);
                return NULL;
            }
        }
        return portStats;
    }
    FILE *f = fopen("/proc/net/dev", "r");
    if (f == NULL) {
        error("Unable to open /proc/net/dev for reading");
        return NULL;
    }
    return network_get_ports_statistics_priv(network, f, res);
}

LMIResult network_set_autoconnect(Network *network, const Port *port, const Connection *connection, bool autoconnect)
{
    LMIResult res = LMI_SUCCESS;
    // Disable autoconnect for all connection on given port
    const Connections *connections = network_get_connections(network);
    const Connection *conn;
    const Port *conn_port;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        conn = connections_index(connections, i);

        if (connection_get_type(conn) == CONNECTION_TYPE_UNKNOWN) {
            // Do not modify unknown connection
            continue;
        }

        conn_port = connection_get_port(conn);
        if (conn_port == NULL || port_compare(port, conn_port)) {
            // Port is requested or not set
            if (connection_compare(connection, conn)) {
                // Set request autoconnect to given connection
                if ((res = connection_set_autoconnect(conn, autoconnect)) != LMI_SUCCESS) {
                    break;
                }
            } else {
                if (connection_get_autoconnect(conn)) {
                    // Disable autoconnect for all other connections
                    if ((res = connection_set_autoconnect(conn, false)) != LMI_SUCCESS) {
                        break;
                    }
                }
            }
        }
    }
    return res;
}

void network_set_connection_added_callback(Network *network, void (*cb)(Network *, Connection *, void *), void *data)
{
    network->connection_added_callback = cb;
    network->connection_added_callback_data = data;
}

void network_set_connection_pre_changed_callback(Network *network, void *(*cb)(Network *, Connection *, void *), void *data)
{
    network->connection_pre_changed_callback = cb;
    network->connection_pre_changed_callback_data = data;
}

void network_set_connection_changed_callback(Network *network, void (*cb)(Network *, Connection *, void *, void *), void *data)
{
    network->connection_changed_callback = cb;
    network->connection_changed_callback_data = data;
}

void network_set_connection_deleted_callback(Network *network, void (*cb)(Network *, Connection *, void *), void *data)
{
    network->connection_deleted_callback = cb;
    network->connection_deleted_callback_data = data;
}

void network_set_port_added_callback(Network *network, void (*cb)(Network *, Port *, void *), void *data)
{
    network->port_added_callback = cb;
    network->port_added_callback_data = data;
}

void network_set_port_pre_changed_callback(Network *network, void *(*cb)(Network *, Port *, void *), void *data)
{
    network->port_pre_changed_callback = cb;
    network->port_pre_changed_callback_data = data;
}

void network_set_port_changed_callback(Network *network, void (*cb)(Network *, Port *, void *, void *), void *data)
{
    network->port_changed_callback = cb;
    network->port_changed_callback_data = data;
}

void network_set_port_deleted_callback(Network *network, void (*cb)(Network *, Port *, void *), void *data)
{
    network->port_deleted_callback = cb;
    network->port_deleted_callback_data = data;
}

void network_set_job_added_callback(Network *network, void (*cb)(Network *, Job *, void *), void *data)
{
    network->job_added_callback = cb;
    network->job_added_callback_data = data;
}

void network_set_job_pre_changed_callback(Network *network, void *(*cb)(Network *, Job *, void *), void *data)
{
    network->job_pre_changed_callback = cb;
    network->job_pre_changed_callback_data = data;
}

void network_set_job_changed_callback(Network *network, void (*cb)(Network *, Job *, void *, void *), void *data)
{
    network->job_changed_callback = cb;
    network->job_changed_callback_data = data;
}

void network_set_job_deleted_callback(Network *network, void (*cb)(Network *, Job *, void *), void *data)
{
    network->job_deleted_callback = cb;
    network->job_deleted_callback_data = data;
}

const CMPIContext *network_get_master_context(const Network *network)
{
    return network->master_context;
}

const CMPIContext *network_get_background_context(const Network *network)
{
    return network->background_context;
}

const Jobs *network_get_jobs(const Network *network)
{
    return network->jobs;
}

void network_cleanup_jobs(Network *network)
{
    Job *job;
    for (size_t i = 0; i < jobs_length(network->jobs); ++i) {
        job = jobs_index(network->jobs, i);
        if (job->delete_on_completion &&
            (job->state == JOB_STATE_FINISHED_OK ||
             job->state == JOB_STATE_FAILED ||
             job->state == JOB_STATE_TERMINATED)) {

            // Time since last change in seconds
            double t = difftime(time(NULL), job->last_change_time);
            // Convert time_before_removal from microseconds to seconds
            // and check if it should be deleted
            if (t > job->time_before_removal / 1000000.0) {
                // Delete the job
                debug("Deleting job %ld", job->id);
                job_free(jobs_pop(network->jobs, i));
                --i;
            }
        }
    }
}

