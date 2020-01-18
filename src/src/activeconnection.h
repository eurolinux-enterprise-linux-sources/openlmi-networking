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

#ifndef ACTIVECONNECTION_H
#define ACTIVECONNECTION_H

#include "globals.h"

ActiveConnection *active_connection_new(Network *network);
Connection *active_connection_get_connection(const ActiveConnection *activeConnection);
const char *active_connection_get_uuid(const ActiveConnection *activeConnection);
const Ports *active_connection_get_ports(const ActiveConnection *activeConnection);
bool active_connection_is_port_active(const ActiveConnection *activeConnection, const Port *port);
void active_connection_free(ActiveConnection *activeConnection);

LIST_DECL(ActiveConnection, active_connection)

bool active_connections_is_connection_active_on_port(const ActiveConnections *activeConnections, const Connection *connection, const Port *port);
ActiveConnection *active_connections_find_by_uuid(const ActiveConnections *activeConnections, const char *uuid);

#endif
