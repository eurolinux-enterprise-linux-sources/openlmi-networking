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

#ifndef NETWORK_H
#define NETWORK_H

#include "globals.h"
#include "job.h"
#include <cmpift.h>

volatile bool lmi_testing;

/**
 * Gets an instance of Network, increases reference counter.
 *
 * \param broker CMPI Broker.
 * \return a \p Network structure (free with \p network_unref).
 */
Network *network_ref(const CMPIBroker *broker, const CMPIContext *ctx);

/**
 * Decreases reference counter and eventually free the structure.
 *
 * \param network a \p Network structure.
 */
void network_unref(Network *);

/**
 * Lock the Network structure
 *
 * \note Never use any of the methods without locked Network structure
 *       (except of network_ref and network_unref)
 *
 * \param network a \p Network structure
 */
void network_lock(Network *);

/**
 * Unlock the Network structure
 *
 * \note Don't forget to always unlock the Network structure
 *
 * \param network a \p Network structure
 */
void network_unlock(Network *);

/**
 * Get list of all detected ports.
 *
 * \param network a \p Network structure.
 * \return \p Ports (don't free).
 */
const Ports *network_get_ports(Network *network);

/**
 * Get \p Port by MAC address.
 *
 * \param network a \p Network structure.
 * \param mac MAC address.
 * \return \p Port (don't free).
 */
Port *network_port_by_mac(Network *, const char *mac);

/**
 * Get list of all connections.
 *
 * \param network a \p Network structure.
 * \return \p Connections (don't free).
 */
const Connections *network_get_connections(Network *network);

/**
 * Get list of all currently active connections.
 *
 * \param network a \p Network structure.
 * \return \p ActiveConnections (don't free).
 */
const ActiveConnections *network_get_active_connections(Network *network);

/**
 * Activate given connection on specific port.
 *
 * \param network a \p Network structure.
 * \param port make connection active on this port.
 * \param connection activate this connection.
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult network_activate_connection(Network *network, const Port *port, const Connection *connection, Job **job);

/**
 * Create connection.
 *
 * \param network a \p Network structure.
 * \param connection connection which will be created.
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult network_create_connection(Network *network, Connection *connection);

/**
 * Delete connection
 *
 * \param network a \p Network structure.
 * \param connection connection which will be deleted.
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult network_delete_connection(Network *network, Connection *connection);

/**
 * Get statistics for each known network port
 *
 * \param network a \p Network structure
 * \return list of statistics for network interfaces or NULL when the statistics
 *         can't be read
 */
PortStats *network_get_ports_statistics(Network* network, LMIResult* res);

/**
 * Set connection to (not) be auto activated on given port
 *
 * \param network a \p Network structure
 * \param port affected port
 * \param connection connection which should (not) be auto activated
 * \param isnext auto activate if true
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult network_set_autoconnect(Network *network, const Port *port, const Connection *connection, bool autoconnect);

void network_set_connection_added_callback(Network *network, void (*cb)(Network *, Connection *, void *), void *data);
void network_set_connection_pre_changed_callback(Network *network, void *(*cb)(Network *, Connection *, void *), void *data);
void network_set_connection_changed_callback(Network *network, void (*cb)(Network *, Connection *, void *, void *), void *data);
void network_set_connection_deleted_callback(Network *network, void (*cb)(Network *, Connection *, void *), void *data);

void network_set_port_added_callback(Network *network, void (*cb)(Network *, Port *, void *), void *data);
void network_set_port_pre_changed_callback(Network *network, void *(*cb)(Network *, Port *, void *), void *data);
void network_set_port_changed_callback(Network *network, void (*cb)(Network *, Port *, void *, void *), void *data);
void network_set_port_deleted_callback(Network *network, void (*cb)(Network *, Port *, void *), void *data);

void network_set_job_added_callback(Network *network, void (*cb)(Network *, Job *, void *), void *data);
void network_set_job_pre_changed_callback(Network *network, void *(*cb)(Network *, Job *, void *), void *data);
void network_set_job_changed_callback(Network *network, void (*cb)(Network *, Job *, void *, void *), void *data);
void network_set_job_deleted_callback(Network *network, void (*cb)(Network *, Job *, void *), void *data);

const CMPIContext *network_get_master_context(const Network *network);
const CMPIContext *network_get_background_context(const Network *network);

const Jobs *network_get_jobs(const Network *network);
void network_cleanup_jobs(Network *network);

#endif
