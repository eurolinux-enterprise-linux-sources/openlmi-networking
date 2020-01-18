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

#include "connection.h"

#include <string.h>

#include "setting.h"

#include "connection_private.h"
#include "network_private.h"
#include "errors.h"
#include "port.h"

Connection *connection_new(Network *network, const char *id, const char *name)
{
    Connection *connection = malloc(sizeof(Connection));
    if (connection == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    connection->network = network;
    if (id != NULL) {
        if ((connection->id = strdup(id)) == NULL) {
            error("Memory allocation failed");
            free(connection);
            return NULL;
        }
    } else {
        connection->id = NULL;
    }
    connection->type = CONNECTION_TYPE_ETHERNET;
    if (name != NULL) {
        if ((connection->name = strdup(name)) == NULL) {
            error("Memory allocation failed");
            free(connection->id);
            free(connection);
            return NULL;
        }
    } else {
        connection->name = NULL;
    }
    connection->uuid = NULL;
    connection->settings = NULL;
    connection->autoconnect = false;
    connection->port = NULL;
    connection->master_id = NULL;
    connection->slave_type = NULL;
    connection->priv = connection_priv_new();
    if (connection->priv == NULL) {
        error("Memory allocation failed");
        connection_free(connection);
        return NULL;
    }
    return connection;
}

Connection *connection_clone(Connection *c)
{
    Connection *connection = connection_new(c->network, c->id, c->name);
    if (connection == NULL) {
        return NULL;
    }
    connection->type = c->type;
    connection->autoconnect = c->autoconnect;
    connection->port = c->port;
    for (size_t i = 0; i < settings_length(c->settings); ++i) {
        if (connection_add_setting(connection, setting_clone(settings_index(c->settings, i))) != LMI_SUCCESS) {
            connection_free(connection);
            return NULL;
        }
    }
    return connection;
}

LMIResult connection_update(const Connection *connection, Connection *new_connection)
{
    return connection_priv_update(connection, new_connection);
}

const char *connection_get_uuid(const Connection *connection)
{
    return connection->uuid;
}

const char *connection_get_id(const Connection *connection)
{
    return connection->id;
}

const char *connection_get_name(const Connection *connection)
{
    return connection->name;
}

LMIResult connection_set_name(Connection *connection, const char *name)
{
    assert(name != NULL);
    if (connection->name != NULL) {
        free(connection->name);
    }
    if ((connection->name = strdup(name)) == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    return LMI_SUCCESS;
}

ConnectionType connection_get_type(const Connection *connection)
{
    return connection->type;
}

LMIResult connection_set_type(Connection *connection, ConnectionType type)
{
    connection->type = type;
    return LMI_SUCCESS;
}

bool connection_get_autoconnect(const Connection *connection)
{
    return connection->autoconnect;
}

LMIResult connection_set_autoconnect(const Connection *connection, bool autoconnect)
{
    Connection new_connection;
    if (memcpy(&new_connection, connection, sizeof(Connection)) == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    new_connection.autoconnect = autoconnect;
    debug("Setting autoconnect to %s for connection %s",
          autoconnect ? "true" : "false", connection->name);
    return connection_update(connection, &new_connection);
}

const Settings *connection_get_settings(const Connection *connection)
{
    return connection->settings;
}

const Port *connection_get_port(const Connection *connection)
{
    if (connection->type == CONNECTION_TYPE_BOND) {
        Setting *setting = settings_find_by_type(connection->settings, SETTING_TYPE_BOND);
        if (setting == NULL) {
            error("Bond connection has no bond setting");
            return NULL;
        }
        const char *name = setting_get_bond_setting(setting)->interface_name;
        if (name == NULL) {
            error("No interface-name property for bond connection");
            return NULL;
        }
        return ports_find_by_id(connection->network->ports, name);
    }
    if (connection->type == CONNECTION_TYPE_BRIDGE) {
        Setting *setting = settings_find_by_type(connection->settings, SETTING_TYPE_BRIDGE);
        if (setting == NULL) {
            error("Bridge connection has no bridge setting");
            return NULL;
        }
        const char *name = setting_get_bridge_setting(setting)->interface_name;
        if (name == NULL) {
            error("No interface-name property for bridge connection");
            return NULL;
        }
        return ports_find_by_id(connection->network->ports, name);
    }
    return connection->port;
}

LMIResult connection_add_setting(Connection *connection, Setting *setting)
{
    if (connection->settings == NULL) {
        if ((connection->settings = settings_new(1)) == NULL) {
            return LMI_ERROR_MEMORY;
        }
    }
    return settings_add(connection->settings, setting);
}

LMIResult connection_set_port(Connection *connection, Port *port)
{
    connection->port = port;
    return LMI_SUCCESS;
}

LMIResult connection_set_master_connection(Connection *connection, const Connection *master, SettingType type)
{
    if ((connection->master_id = strdup(master->id)) == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    if (type == SETTING_TYPE_BOND) {
        if ((connection->slave_type = strdup("bond")) == NULL) {
            error("Memory allocation failed");
            return LMI_ERROR_MEMORY;
        }
    }
    if (type == SETTING_TYPE_BRIDGE) {
        if ((connection->slave_type = strdup("bridge")) == NULL) {
            error("Memory allocation failed");
            return LMI_ERROR_MEMORY;
        }
    }
    return LMI_SUCCESS;
}

Connection *connection_get_master_connection(Connection *connection)
{
    if (connection->master_id == NULL) {
        return NULL;
    }
    const Connections *connections = connection->network->connections;
    Connection *c;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        c = connections_index(connections, i);
        if ((strcmp(connection->master_id, c->id)) == 0 ||
            (strcmp(connection->master_id, c->name) == 0)) {

            return c;
        }
    }
    return NULL;
}

bool connection_compare(const Connection *c1, const Connection *c2)
{
    if (c1 == NULL || c2 == NULL) {
        return false;
    }
    if (c1->id == NULL || c2->id == NULL) {
        return false;
    }

    return strcmp(c1->id, c2->id) == 0;
}

LMIResult connection_delete(const Connection *connection)
{
    return connection_priv_delete(connection);
}

void connection_free(Connection *connection)
{
    if (connection == NULL) {
        return;
    }
    settings_free(connection->settings, true);
    free(connection->uuid);
    free(connection->id);
    free(connection->name);
    free(connection->master_id);
    free(connection->slave_type);
    connection_priv_free(connection->priv);
    free(connection);
}

LIST_IMPL(Connection, connection)

Connection *connections_find_by_uuid(const Connections *connections, const char *uuid)
{
    if (uuid == NULL) {
        return NULL;
    }
    Connection *connection;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        connection = connections_index(connections, i);
        if (connection->uuid != NULL && strcmp(connection->uuid, uuid) == 0) {
            return connection;
        }
    }
    return NULL;
}

Connection *connections_find_by_id(const Connections *connections, const char *id)
{
    if (id == NULL) {
        return NULL;
    }
    Connection *connection;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        connection = connections_index(connections, i);
        if (connection->id != NULL && strcmp(connection->id, id) == 0) {
            return connection;
        }
    }
    return NULL;
}
