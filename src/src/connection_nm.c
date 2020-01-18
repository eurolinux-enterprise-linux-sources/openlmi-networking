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

#include "connection_nm.h"

#include <string.h>

#include "dbus_wrapper.h"
#include "errors.h"

#include "network_nm.h"
#include "nm_support.h"
#include "setting_nm.h"

#include "connection_private.h"
#include "port_private.h"
#include "network_private.h"
#include "setting_private.h"

struct {
    ConnectionType type;
    const char *name;
} ConnectionTypeStrings[] = {
    { CONNECTION_TYPE_ETHERNET, "802-3-ethernet" },
    { CONNECTION_TYPE_BOND, "bond" },
    { CONNECTION_TYPE_BRIDGE, "bridge" }
};

const char *connection_type_to_string(ConnectionType type)
{
    for (size_t i = 0; i < sizeof(ConnectionTypeStrings) / sizeof(ConnectionTypeStrings[0]); ++i) {
        if (ConnectionTypeStrings[i].type == type) {
            return ConnectionTypeStrings[i].name;
        }
    }
    error("connection_type_to_string called with undefined type %d", type);
    return "unknown";
}

ConnectionType connection_type_from_string(const char *s)
{
    for (size_t i = 0; i < sizeof(ConnectionTypeStrings) / sizeof(ConnectionTypeStrings[0]); ++i) {
        if (strcmp(ConnectionTypeStrings[i].name, s) == 0) {
            return ConnectionTypeStrings[i].type;
        }
    }
    error("connection_type_from_string called with undefined string %s", s);
    return CONNECTION_TYPE_UNKNOWN;
}

typedef struct ConnectionPriv {
    DBusGProxy *proxy;
} ConnectionPriv;

LMIResult connection_get_properties(Connection *connection);
void connection_updated_cb(void *proxy, Connection *connection);
void connection_removed_cb(void *proxy, Connection *connection);

Connection *connection_new_from_objectpath(Network *network, const char *objectpath, LMIResult *res)
{
    // Id and name will be filled later
    Connection *connection = connection_new(network, NULL, NULL);
    if (connection == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        return NULL;
    }
    ConnectionPriv *priv = connection->priv;
    connection->uuid = strdup(objectpath);
    if (connection->uuid == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        connection_free(connection);
        return NULL;
    }
    priv->proxy = dbus_g_proxy_new_for_name(network_priv_get_dbus_connection(network), NM_SERVICE_DBUS, objectpath, NM_DBUS_IFACE_SETTINGS_CONNECTION);
    if (priv->proxy == NULL) {
        error("Cannot create DBus proxy for: %s %s %s", NM_SERVICE_DBUS, objectpath, NM_DBUS_IFACE_SETTINGS_CONNECTION);
        *res = LMI_ERROR_BACKEND;
        connection_free(connection);
        return NULL;
    }

    if ((*res = connection_get_properties(connection)) != LMI_SUCCESS) {
        error("Unable to get connection properties");
        connection_free(connection);
        return NULL;
    }

    dbus_g_proxy_add_signal(priv->proxy, "Updated", G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->proxy, "Updated", G_CALLBACK(connection_updated_cb), connection, NULL);

    dbus_g_proxy_add_signal(priv->proxy, "Removed", G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->proxy, "Removed", G_CALLBACK(connection_removed_cb), connection, NULL);
    return connection;
}

LMIResult connection_read_properties(Connection *connection, GHashTable *hash)
{
    LMIResult res = LMI_SUCCESS;
    GHashTableIter iter;
    gpointer key, value;
    Setting *setting;
    const char *type, *s;
    GValue *v;
    char *id, *caption;
    const char *str;
    g_hash_table_iter_init(&iter, hash);
    if (connection->settings != NULL) {
        settings_free(connection->settings, true);
    }
    if ((connection->settings = settings_new(3)) == NULL) {
        return LMI_ERROR_MEMORY;
    }

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        if (strcmp(key, "connection") == 0) {
            GHashTable *connection_map = value;
            if (!connection_map) {
                error("Key \"connection\" not present in connection hash");
                res = LMI_ERROR_BACKEND;
                goto err;
            }

            if ((str = dbus_property_string(connection_map, "uuid")) == NULL) {
                goto err;
            }
            free(connection->id);
            if ((connection->id = strdup(str)) == NULL) {
                error("Memory allocation failed");
                res = LMI_ERROR_MEMORY;
                goto err;
            }

            if ((str = dbus_property_string(connection_map, "id")) == NULL) {
                error("No such key \"id\" in \"connection\" subhash");
                res = LMI_ERROR_BACKEND;
                goto err;
            }
            free(connection->name);
            if ((connection->name = strdup(str)) == NULL) {
                error("Memory allocation failed");
                res = LMI_ERROR_MEMORY;
                goto err;
            }

            GValue *v = g_hash_table_lookup(connection_map, "autoconnect");
            if (v) {
                connection->autoconnect = g_value_get_boolean(v);
            } else {
                connection->autoconnect = true;
            }
            type = dbus_property_string(connection_map, "type");
            if (type == NULL) {
                connection->type = CONNECTION_TYPE_UNKNOWN;
            } else {
                connection->type = connection_type_from_string(type);
            }
            if (connection->type == CONNECTION_TYPE_UNKNOWN) {
                warn("Connection %s has unknown type: %s", connection->id, type);
            }

            v = g_hash_table_lookup(connection_map, "master");
            if (v) {
                s = g_value_get_string(v);
                if (s != NULL) {
                    free(connection->master_id);
                    if ((connection->master_id = strdup(s)) == NULL) {
                        error("Memory allocation failed");
                        res = LMI_ERROR_MEMORY;
                        goto err;
                    }
                }
            }
            v = g_hash_table_lookup(connection_map, "slave-type");
            if (v) {
                s = g_value_get_string(v);
                if (s != NULL) {
                    free(connection->slave_type);
                    if ((connection->slave_type = strdup(s)) == NULL) {
                        error("Memory allocation failed");
                        res = LMI_ERROR_MEMORY;
                        goto err;
                    }
                }
            }
        } else if (strcmp(key, "802-3-ethernet") == 0) {
            v = g_hash_table_lookup((GHashTable *) value, "mac-address");
            if (v != NULL) {
                GByteArray *mac = g_value_get_boxed(v);
                char *mac_str = macFromGByteArray(mac);
                if (mac_str == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                connection->port = network_port_by_mac(connection->network, mac_str);
                free(mac_str);
            }
        } else {
            if ((setting = setting_from_hash(value, key, &res)) == NULL) {
                goto err;
            }
            if ((res = connection_add_setting(connection, setting)) != LMI_SUCCESS) {
                goto err;
            }
        }
    }
    // Add ids and captions to the settings
    for (size_t i = 0; i < settings_length(connection->settings); ++i) {
        setting = settings_index(connection->settings, i);
        if (asprintf(&id, "%s_%ld", connection->id, i) < 0) {
            res = LMI_ERROR_MEMORY;
            goto err;
        }
        setting->id = id;
        if (asprintf(&caption, "%s %ld", connection->name, i) < 0) {
            res = LMI_ERROR_MEMORY;
            goto err;
        }
        setting->caption = caption;
    }
    return LMI_SUCCESS;
err:
    // What cleanup should be done here?
    return res;
}

GHashTable *connection_to_hash(Connection *connection, LMIResult *res)
{
    GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) g_hash_table_destroy);
    if (hash == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        return NULL;
    }
    GHashTable *subhash;
    Setting *setting;
    char *s;

    // connection
    subhash = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) g_value_free);
    if (subhash == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }
    if ((*res = g_hash_table_insert_string_value(subhash, "id", connection->name)) != LMI_SUCCESS) {
        goto err;
    }

    if (connection->id == NULL) {
        if ((connection->id = uuid_gen()) == NULL) {
            error("Memory allocation failed");
            *res = LMI_ERROR_MEMORY;
            goto err;
        }
    }
    if ((*res = g_hash_table_insert_string_value(subhash, "uuid", connection->id)) != LMI_SUCCESS) {
        goto err;
    }

    GValue *v = g_value_new(G_TYPE_BOOLEAN);
    if (v == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }
    g_value_set_boolean(v, connection->autoconnect);
    if ((s = strdup("autoconnect")) == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }
    g_hash_table_insert(subhash, s, v);

    if ((*res = g_hash_table_insert_string_value(subhash, "type", connection_type_to_string(connection->type))) != LMI_SUCCESS) {
        goto err;
    }

    if (connection->master_id != NULL) {
        if ((*res = g_hash_table_insert_string_value(subhash, "master", connection->master_id)) != LMI_SUCCESS) {
            goto err;
        }
    }
    if (connection->slave_type != NULL) {
        if ((*res = g_hash_table_insert_string_value(subhash, "slave-type", connection->slave_type)) != LMI_SUCCESS) {
            goto err;
        }
    }

    if ((s = strdup("connection")) == NULL) {
        error("Memory allocation failed");
        *res = LMI_ERROR_MEMORY;
        goto err;
    }
    g_hash_table_insert(hash, s, subhash);

    // connection type specific
    if (connection->type == CONNECTION_TYPE_ETHERNET) {
        subhash = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) g_value_free);
        if (subhash == NULL) {
            error("Memory allocation failed");
            *res = LMI_ERROR_MEMORY;
            goto err;
        }
        if (connection->port != NULL) {
            const char *mac = connection->port->permmac;
            if (mac == NULL) {
                mac = connection->port->mac;
            }
            if ((*res = g_hash_table_insert_boxed(subhash, "mac-address",
                    DBUS_TYPE_G_UCHAR_ARRAY,
                    macToGByteArray(mac), true)) != LMI_SUCCESS) {

                goto err;
            }
        }
        if ((s = strdup("802-3-ethernet")) == NULL) {
            error("Memory allocation failed");
            *res = LMI_ERROR_MEMORY;
            goto err;
        }
        g_hash_table_insert(hash, s, subhash);
    }

    for (size_t i = 0; i < settings_length(connection->settings); ++i) {
        setting = settings_index(connection->settings, i);
        if (setting->type == SETTING_TYPE_UNKNOWN) {
            error("Can't convert unknown setting to hash");
            continue;
        }
        char *key = NULL;
        subhash = setting_to_hash(setting, &key, res);
        if (subhash != NULL) {
            g_hash_table_insert(hash, key, subhash);
        } else {
            free(key);
            goto err;
        }
    }
    g_hash_table_print(hash);
    return hash;
err:
    g_hash_table_unref(hash);
    return NULL;
}

void *connection_priv_new(void)
{
    ConnectionPriv *p = malloc(sizeof(ConnectionPriv));
    if (p == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    p->proxy = NULL;
    return p;
}

void connection_priv_free(void *priv)
{
    if (priv == NULL) {
        return;
    }
    ConnectionPriv *p = priv;
    if (p->proxy != NULL) {
        g_object_unref(p->proxy);
    }
    free(p);
}

LMIResult connection_get_properties(Connection *connection)
{
    ConnectionPriv *priv = connection->priv;
    GHashTable *hash;
    GError *err = NULL;
    if (!dbus_g_proxy_call(priv->proxy, "GetSettings", &err, G_TYPE_INVALID,
        DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT, &hash, G_TYPE_INVALID)) {

        error("Call GetSetting of %s failed: %s", NM_DBUS_IFACE_SETTINGS_CONNECTION, err->message);
        return LMI_ERROR_BACKEND;
    }
    if (hash == NULL) {
        error("Connection %s doesn't have any settings", connection->uuid);
        return LMI_ERROR_BACKEND;
    }
    return connection_read_properties(connection, hash);
}

void connection_updated_cb(void *proxy, Connection *connection)
{
    Network *network = connection->network;
    network_lock(network);
    debug("Connection updated: %s (%s)", connection->id, connection->name);

    void *data = NULL;
    if (network->connection_pre_changed_callback != NULL) {
        data = network->connection_pre_changed_callback(network, connection,
                network->connection_pre_changed_callback_data);
    }

    if (connection_get_properties(connection) != LMI_SUCCESS) {
        error("Connection update failed");
    }
    if (network->connection_changed_callback != NULL) {
        network->connection_changed_callback(network, connection,
                network->connection_changed_callback_data, data);
    }

    network_unlock(network);
}

void connection_removed_cb(void *proxy, Connection *connection)
{
    assert(connection != NULL);
    Network *network = connection->network;
    network_lock(network);

    if (connection->uuid == NULL) {
        error("Trying to delete uknown connection");
        network_unlock(network);
        return;
    }
    // NetworkManager sometimes sends Removed signal twice, we need to workaround it
    if (connection == NULL || strcmp(dbus_g_proxy_get_path(proxy), connection->uuid) != 0) {
        debug("Connection already deleted");
        network_unlock(network);
        return;
    }
    debug("Connection deleted: %s (%s)", connection->id, connection->name);

    size_t i = 0;
    const char *uuid;
    Connections *connections = connection->network->connections;
    for (i = 0; i < connections_length(connections); ++i) {
        uuid = connections_index(connections, i)->uuid;
        if (uuid == NULL) {
            continue;
        }
        if (strcmp(uuid, connection->uuid) == 0) {
            break;
        }
    }
    if (network->connection_deleted_callback != NULL) {
        network->connection_deleted_callback(network, connection,
                network->connection_deleted_callback_data);
    }
    if (i < connections_length(connections)) {
        connection_free(connections_pop(connections, i));
        connection = NULL;
    }
    network_unlock(network);
}

LMIResult connection_priv_delete(const Connection *connection)
{
    ConnectionPriv *priv = connection->priv;
    GError *err = NULL;
    if (!dbus_g_proxy_call(priv->proxy, "Delete", &err, G_TYPE_INVALID, G_TYPE_INVALID)) {
        error("Deleting of connection failed: %s", err->message);
        return LMI_ERROR_CONNECTION_DELETE_FAILED;
    }
    return LMI_SUCCESS;
}

#ifndef TESTING
LMIResult connection_priv_update(const Connection *connection, Connection *new_connection)
{
    LMIResult res = LMI_SUCCESS;
    ConnectionPriv *priv = connection->priv;
    GError *err = NULL;
    GHashTable *new_hash = connection_to_hash(new_connection, &res);
    if (new_hash == NULL) {
        error("Unable to convert connection %s to hash", new_connection->id);
        return res;
    }

    if (!dbus_g_proxy_call(priv->proxy, "Update", &err, DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT, new_hash, G_TYPE_INVALID, G_TYPE_INVALID)) {
        error("Unable to update connection %s: %s",
              connection != NULL ? connection_get_name(connection) : "NULL",
              err != NULL ? err->message : "Unknown error");
        res = LMI_ERROR_CONNECTION_UPDATE_FAILED;
    }

    return res;
}
#endif
