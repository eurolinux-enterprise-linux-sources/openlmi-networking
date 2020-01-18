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

#include "activeconnection.h"

#include "connection.h"
#include "port.h"

#include "activeconnection_private.h"
#include <string.h>

ActiveConnection *active_connection_new(Network *network)
{
    ActiveConnection *activeConnection = malloc(sizeof(ActiveConnection));
    if (activeConnection == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    activeConnection->network = network;
    activeConnection->uuid = NULL;
    activeConnection->connection = NULL;
    activeConnection->ports = NULL;
    return activeConnection;
}

Connection *active_connection_get_connection(const ActiveConnection *activeConnection)
{
    assert(activeConnection);
    return activeConnection->connection;
}

const Ports *active_connection_get_ports(const ActiveConnection *activeConnection)
{
    assert(activeConnection);
    return activeConnection->ports;
}


void active_connection_free(ActiveConnection *activeConnection)
{
    if (activeConnection == NULL) {
        return;
    }
    free(activeConnection->uuid);
    ports_free(activeConnection->ports, false);
    free(activeConnection);
}

bool active_connection_is_port_active(const ActiveConnection *activeConnection, const Port *port)
{
    assert(activeConnection);
    if (port == NULL) {
        return false;
    }
    for (size_t i = 0; i < ports_length(activeConnection->ports); ++i) {
        if (port_compare(port, ports_index(activeConnection->ports, i))) {
            return true;
        }
    }
    return false;
}

LIST_IMPL(ActiveConnection, active_connection)

bool active_connections_is_connection_active_on_port(const ActiveConnections *activeConnections, const Connection *connection, const Port *port)
{
    if (activeConnections == NULL || connection == NULL || port == NULL) {
        return false;
    }
    ActiveConnection *activeConnection;
    for (size_t i = 0; i < active_connections_length(activeConnections); ++i) {
        activeConnection = active_connections_index(activeConnections, i);

        if (connection_compare(activeConnection->connection, connection)) {
            if (active_connection_is_port_active(activeConnection, port)) {
                return true;
            }
        }
    }
    return false;
}

ActiveConnection *active_connections_find_by_uuid(const ActiveConnections *activeConnections, const char *uuid)
{
    if (uuid == NULL) {
        return NULL;
    }
    ActiveConnection *activeConnection;
    for (size_t i = 0; i < active_connections_length(activeConnections); ++i) {
        activeConnection = active_connections_index(activeConnections, i);
        debug("Compare: %s %s", activeConnection->uuid, uuid);
        if (activeConnection->uuid != NULL && strcmp(activeConnection->uuid, uuid) == 0) {
            return activeConnection;
        }
    }
    return NULL;
}
