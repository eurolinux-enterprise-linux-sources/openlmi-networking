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

#include "mock_nm.h"

#include <glib-object.h>

#include "globals.h"
#include "connection_private.h"

// NetworkPriv
typedef struct NetworkPriv {
    ActiveConnections *activeConnections;
} NetworkPriv;

#include "network.h"
#include "network_private.h"
#include <port_private.h>
#include <activeconnection.h>
#include <errors.h>


void *network_priv_new(Network *network)
{
    NetworkPriv *priv = malloc(sizeof(NetworkPriv));
    if (priv == NULL) {
        return priv;
    }
    priv->activeConnections = active_connections_new(0);
    return priv;
}

void network_priv_free(void *priv)
{
    active_connections_free(((NetworkPriv *) priv)->activeConnections, true);
    free(priv);
}

LMIResult network_priv_get_devices(Network *network)
{
    warn("network_priv_get_devices not implemented");
    return LMI_SUCCESS;
}

void network_priv_get_dbus_connection(Network *network)
{
}

LMIResult network_priv_activate_connection(Network *network, const Port *port, const Connection *connection, Job **job)
{
    return LMI_SUCCESS;
}

LMIResult network_priv_deactivate_connection(Network *network, const ActiveConnection *activeConnection, Job **job)
{
    return LMI_SUCCESS;
}

LMIResult network_priv_get_active_connections(Network *network)
{
    warn("network_priv_get_active_connections not implemented");
    return LMI_SUCCESS;
}

LMIResult network_priv_create_connection(Network *network, Connection *connection)
{
    warn("network_priv_create_connection not implemented");
    return LMI_SUCCESS;
}

LMIResult network_priv_delete_connection(Network *network, Connection *connection)
{
    warn("network_priv_delete_connection not implemented");
    return LMI_SUCCESS;
}

void active_connection_priv_free(void *priv)
{
}

void port_priv_free(void *priv)
{
}

const char *port_priv_get_state_reason(const Port *port)
{
    return NULL;
}

Ports *port_priv_get_slaves(Network *network, const Port *port)
{
    return NULL;
}

int port_priv_disconnect(Port *port)
{
    return LMI_SUCCESS;
}

LMIResult connection_priv_update(const Connection *connection, Connection *new_connection)
{
    ((Connection *) connection)->autoconnect = new_connection->autoconnect;
    return LMI_SUCCESS;
}
