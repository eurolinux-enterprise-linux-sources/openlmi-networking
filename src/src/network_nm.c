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

#include "network_nm.h"

#include <string.h>
#include <unistd.h>

#include "activeconnection.h"
#include "dbus_wrapper.h"
#include "errors.h"

#include "activeconnection_nm.h"
#include "connection_nm.h"
#include "port_nm.h"

#include "network_private.h"
#include "port_private.h"
#include "nm_support.h"
#include "connection_private.h"
#include "job.h"

int DBUS_BUS = DBUS_BUS_SYSTEM;
const char *NM_SERVICE_DBUS = NM_DBUS_SERVICE;

typedef struct _NetworkPriv {
    DBusGConnection *connection;
    DBusGProxy *managerProxy;
    DBusGProxy *connectionProxy;
    GHashTable *properties;
} NetworkPriv;

void device_added_cb(void *proxy, const char *objectpath, Network *network);
void device_removed_cb(void *proxy, const char *objectpath, Network *network);
void manager_properties_changed_cb(void *proxy, GHashTable *properties, Network *network);
void manager_state_changed_cb(void *proxy, unsigned int state, Network *network);
void connection_added_cb(void *proxy, const char *objectpath, Network *network);
void connection_properties_changed_cb(void *proxy, GHashTable *properties, Network *network);
LMIResult network_priv_get_connections(Network *network);

void *network_priv_new(Network *network)
{
    // network is already locked in the caller
    g_type_init();
    GError *err = NULL;
    NetworkPriv *priv = malloc(sizeof(NetworkPriv));
    if (priv == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    network->priv = priv;
    priv->connection = NULL;
    priv->managerProxy = NULL;
    priv->connectionProxy = NULL;
    priv->properties = NULL;

    // Necessary for DBus to work in separate thread
    dbus_g_thread_init();

    // For testing purposes it's possible to use fake NM DBus API
    if (lmi_testing) {
        DBUS_BUS = DBUS_BUS_SESSION;
        NM_SERVICE_DBUS = "org.freedesktop.FakeNetworkManager";
    }

    // Connect to DBus
    priv->connection = dbus_g_bus_get(DBUS_BUS, &err);
    if (priv->connection == NULL) {
        error("Failed to open connection to bus: %s\n", err->message);
        goto err;
    }
    priv->managerProxy = dbus_g_proxy_new_for_name(priv->connection, NM_SERVICE_DBUS, NM_DBUS_PATH, NM_DBUS_INTERFACE);
    if (priv->managerProxy == NULL) {
        error("Unable to create DBus proxy: %s " NM_DBUS_PATH " "NM_DBUS_INTERFACE, NM_SERVICE_DBUS);
        goto err;
    }
    priv->connectionProxy = dbus_g_proxy_new_for_name(priv->connection, NM_SERVICE_DBUS, NM_DBUS_PATH_SETTINGS, NM_DBUS_IFACE_SETTINGS);
    if (priv->connectionProxy == NULL) {
        error("Unable to create DBus proxy: %s " NM_DBUS_PATH " "NM_DBUS_INTERFACE, NM_SERVICE_DBUS);
        goto err;
    }

    // Read manager properties
    priv->properties = dbus_get_properties(priv->managerProxy, NM_DBUS_PATH, NM_DBUS_INTERFACE);
    if (priv->properties == NULL) {
        error("Unable to get DBus properties: %s " NM_DBUS_PATH " " NM_DBUS_INTERFACE, dbus_g_proxy_get_bus_name(priv->managerProxy));
        goto err;
    }

    // Read ports, connections and active connection from NetworkManager
    if (network_priv_get_devices(network) != LMI_SUCCESS) {
        error("Unable to get network devices");
        goto err;
    }
    if (network_priv_get_connections(network) != LMI_SUCCESS) {
        error("Unable to get network connections");
        goto err;
    }
    if (network_priv_get_active_connections(network) != LMI_SUCCESS) {
        error("Unable to get active network connections");
        goto err;
    }

    dbus_g_proxy_add_signal(priv->managerProxy, "DeviceAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->managerProxy, "DeviceAdded", G_CALLBACK(device_added_cb), network, NULL);

    dbus_g_proxy_add_signal(priv->managerProxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->managerProxy, "DeviceRemoved", G_CALLBACK(device_removed_cb), network, NULL);

    dbus_g_proxy_add_signal(priv->managerProxy, "PropertiesChanged", DBUS_TYPE_G_MAP_OF_VARIANT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->managerProxy, "PropertiesChanged", G_CALLBACK(manager_properties_changed_cb), network, NULL);

    dbus_g_proxy_add_signal(priv->managerProxy, "StateChanged", G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->managerProxy, "StateChanged", G_CALLBACK(manager_state_changed_cb), network, NULL);

    dbus_g_proxy_add_signal(priv->connectionProxy, "NewConnection", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->connectionProxy, "NewConnection", G_CALLBACK(connection_added_cb), network, NULL);

    dbus_g_proxy_add_signal(priv->connectionProxy, "PropertiesChanged", DBUS_TYPE_G_MAP_OF_VARIANT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->connectionProxy, "PropertiesChanged", G_CALLBACK(connection_properties_changed_cb), network, NULL);

    return priv;
err:
    network_priv_free(network->priv);
    return NULL;
}

DBusGConnection *network_priv_get_dbus_connection(Network *network)
{
    NetworkPriv *priv = network->priv;
    return priv->connection;
}

void network_priv_free(void *data)
{
    if (data == NULL) {
        return;
    }
    NetworkPriv *priv = (NetworkPriv *) data;
    if (priv->connection != NULL) {
        dbus_g_connection_unref(priv->connection);
    }
    if (priv->managerProxy != NULL) {
        g_object_unref(priv->managerProxy);
    }
    if (priv->connectionProxy != NULL) {
        g_object_unref(priv->connectionProxy);
    }
    if (priv->properties != NULL) {
        g_hash_table_destroy(priv->properties);
    }
    free(priv);
}

LMIResult network_priv_activate_connection(Network *network, const Port *port, const Connection *connection, Job **job)
{
    NetworkPriv *priv = network->priv;
    GError *err = NULL;
    char *activeConnection;
    if (!dbus_g_proxy_call(priv->managerProxy, "ActivateConnection", &err,
            DBUS_TYPE_G_OBJECT_PATH, connection->uuid,
            DBUS_TYPE_G_OBJECT_PATH, port_get_uuid(port),
            DBUS_TYPE_G_OBJECT_PATH, "/", G_TYPE_INVALID,
            DBUS_TYPE_G_OBJECT_PATH, &activeConnection, G_TYPE_INVALID)) {

        error("Unable to activate connection %s on port %s: %s",
              connection ? connection_get_name(connection) : "NULL",
              port ? port_get_id(port) : "NULL",
              err->message);

        char *error_name = err->message + strlen(err->message) + 1;
        if (strcmp(error_name, "org.freedesktop.NetworkManager.Error.UnknownConnection") == 0) {
            return LMI_ERROR_CONNECTION_UNKNOWN;
        } else if (strcmp(error_name, "org.freedesktop.NetworkManager.Error.UnknownDevice") == 0) {
            return LMI_ERROR_PORT_UNKNOWN;
        } else if (strcmp(error_name, "org.freedesktop.NetworkManager.Error.ConnectionActivating") == 0) {
            return LMI_ERROR_CONNECTION_ACTIVATING;
        } else if (strcmp(error_name, "org.freedesktop.NetworkManager.Error.ConnectionInvalid") == 0) {
            return LMI_ERROR_CONNECTION_INVALID;
        } else {
            return LMI_ERROR_UNKNOWN;
        }
    }

    *job = job_new(JOB_TYPE_APPLY_SETTING_DATA);
    job_add_affected_element(*job, JOB_AFFECTED_ACTIVE_CONNECTION_ID, activeConnection);
    debug("Job monitoring ActiveConnection %s started", activeConnection);
    free(activeConnection);
    job_add_affected_element(*job, JOB_AFFECTED_PORT_ID, port_get_id(port));
    job_add_affected_element(*job, JOB_AFFECTED_CONNECTION_ID, connection_get_id(connection));
    job_set_state(*job, JOB_STATE_RUNNING);
    jobs_add(network->jobs, *job);
    if (network->job_added_callback != NULL) {
        network->job_added_callback(network, *job, network->job_added_callback_data);
    }
    return LMI_JOB_STARTED;
}

/*
Connection *network_priv_get_default_connection(Network *network, Port *port)
{
    Connection *connection;
    const Connections *connections = network->connections;
    for (size_t i = 0; i < connections_length(connections); ++i) {
        connection = connections_index(connections, i);
        if (!connection->autoconnect) {
            continue;
        }
        if (connection->port && port_compare(connection->port, port)) {
            return connection;
        }
    }
    error("Port %s has no default connection.", port_get_id(port));
    return NULL;
}
*/

void device_added_cb(void *proxy, const char *objectpath, Network *network)
{
    debug("Device added: %s", objectpath);
    MUTEX_LOCK(network);

    // Check if port with same objectpath is present
    for (size_t i = 0; i < ports_length(network->ports); ++i) {
        if (strcmp(port_get_uuid(ports_index(network->ports, i)), objectpath) == 0) {
            // Remove the port
            port_free(ports_pop(network->ports, i));
        }
    }

    Port *port = port_new_from_objectpath(network, objectpath);
    ports_add(network->ports, port);
    if (network->port_added_callback != NULL) {
        network->port_added_callback(network, port,
                network->port_added_callback_data);
    }
    MUTEX_UNLOCK(network);
}

void device_removed_cb(void *proxy, const char *objectpath, Network *network)
{
    debug("Device removed: %s", objectpath);
    MUTEX_LOCK(network);
    size_t i, len = ports_length(network->ports);
    for (i = 0; i < len; i++) {
        if (strcmp(ports_index(network->ports, i)->uuid, objectpath) == 0) {
            break;
        }
    }
    if (i < len) {
        Port *port = ports_pop(network->ports, i);
        if (network->port_deleted_callback != NULL) {
            network->port_deleted_callback(network, port,
                    network->port_deleted_callback_data);
        }
        port_free(port);
    }
    MUTEX_UNLOCK(network);
}

void connection_added_cb(void *proxy, const char *objectpath, Network *network)
{
    debug("Connection added: %s", objectpath);
    LMIResult res;
    MUTEX_LOCK(network);
    Connection *connection = connection_new_from_objectpath(network, objectpath, &res);
    connections_add(network->connections, connection);
    if (network->connection_added_callback != NULL) {
        network->connection_added_callback(network, connection,
                network->connection_added_callback_data);
    }
    MUTEX_UNLOCK(network);
}

void connection_properties_changed_cb(void *proxy, GHashTable *properties, Network *network)
{
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, properties);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        debug("Connections property changed: %s", (char *) key);
    }
}

void manager_properties_changed_cb(void *proxy, GHashTable *properties, Network *network)
{
    NetworkPriv *priv = network->priv;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, properties);
    gpointer key, value;
    MUTEX_LOCK(network);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        if (strcmp((char *) key, "ActiveConnections") == 0) {
            debug("Active Connections changed");
            // Reload active connections
            priv->properties = dbus_get_properties(priv->managerProxy, NM_DBUS_PATH, NM_DBUS_INTERFACE);
            network_priv_get_active_connections(network);
        } else {
            debug("Manager - unhandled property changed: %s", (char *) key);
        }
    }
    MUTEX_UNLOCK(network);
}

void manager_state_changed_cb(void *proxy, unsigned int state, Network *network)
{
    debug("Manager state changed: %d - not implemented", state);
}

LMIResult network_priv_get_devices(Network *network)
{
    // network is already locked in the caller
    NetworkPriv *priv = network->priv;

    LMIResult res;
    char *device;
    GPtrArray *devices;
    GError *err = NULL;
    if (!dbus_g_proxy_call(priv->managerProxy, "GetDevices", &err, G_TYPE_INVALID,
        DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH, &devices, G_TYPE_INVALID)) {

        error("Calling method GetDevices failed: %s", err->message);
        return LMI_ERROR_BACKEND;
    }

    if ((network->ports = ports_new(devices->len)) == NULL) {
        return LMI_ERROR_MEMORY;
    }

    Port *port;
    for (guint i = 0; i < devices->len; ++i) {
        device = devices->pdata[i];
        port = port_new_from_objectpath(network, device);
        if (port == NULL) {
            return LMI_ERROR_MEMORY;
        }
        debug("Device: %s (%s)", port->id, device);
        if ((res = ports_add(network->ports, port)) != LMI_SUCCESS) {
            return res;
        }
    }
    g_ptr_array_free(devices, TRUE);
    return LMI_SUCCESS;
}

LMIResult network_priv_get_connections(Network *network)
{
    LMIResult res = LMI_SUCCESS;
    // network is already locked in the caller
    NetworkPriv *priv = network->priv;
    GPtrArray *connections;
    GError *err = NULL;

    if (!dbus_g_proxy_call(priv->connectionProxy, "ListConnections", &err, G_TYPE_INVALID,
        dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH), &connections, G_TYPE_INVALID)) {

        error("Calling method ListConnections failed: %s", err->message);
        return LMI_ERROR_BACKEND;
    }

    network->connections = connections_new(connections->len);
    if (network->connections == NULL) {
        return LMI_ERROR_MEMORY;
    }

    Connection *connection;
    char *path;
    for (guint i = 0; i < connections->len; ++i) {
        path = connections->pdata[i];
        connection = connection_new_from_objectpath(network, path, &res);
        if (connection == NULL) {
            break;
        }
        debug("Connection: %s (%s)", connection->name, path);
        if ((res = connections_add(network->connections, connection)) != LMI_SUCCESS) {
            break;
        }
    }
    g_ptr_array_free(connections, TRUE);
    return res;
}

LMIResult network_priv_get_active_connections(Network *network)
{
    LMIResult res = LMI_SUCCESS;
    NetworkPriv *priv = network->priv;
    GPtrArray *array = dbus_property_array(priv->properties, "ActiveConnections");

    if (array != NULL) {
        network->activeConnections = active_connections_new(array->len);

        const char *objectpath;
        ActiveConnection *activeConnection;
        for (guint i = 0; i < array->len; ++i) {
            objectpath = (char *) g_ptr_array_index(array, i);
            activeConnection = active_connection_from_objectpath(network, objectpath, &res);
            if (activeConnection == NULL) {
                continue;
            }
            active_connections_add(network->activeConnections, activeConnection);
        }
    } else {
        network->activeConnections = active_connections_new(0);
    }
    return res;
}

LMIResult network_priv_create_connection(Network *network, Connection *connection)
{
    LMIResult res;
    debug("network_priv_create_connection");
    NetworkPriv *priv = network->priv;
    char *objectpath = NULL;
    GError *err = NULL;

    if (connection_get_name(connection) == NULL) {
        // Name can't be NULL
        connection_set_name(connection, "OpenLMI connection");
    }

    GHashTable *hash = connection_to_hash(connection, &res);
    if (hash == NULL) {
        return res;
    }

    g_hash_table_print(hash);

    if (!dbus_g_proxy_call(priv->connectionProxy, "AddConnection", &err,
            DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT, hash, G_TYPE_INVALID,
            DBUS_TYPE_G_OBJECT_PATH, &objectpath, G_TYPE_INVALID)) {

        char *error_name = err->message + strlen(err->message) + 1;
        error("Creating of connection failed: %d %s %s", err->code, err->message, error_name);
        res = LMI_ERROR_BACKEND;
    }
    if (objectpath != NULL) {
        if ((connection->uuid = strdup(objectpath)) == NULL) {
            error("Memory allocation failed");
            res = LMI_ERROR_MEMORY;
        }
    }
    g_hash_table_destroy(hash);
    return res;
}

LMIResult network_priv_delete_connection(Network *network, Connection *connection)
{
    return connection_delete(connection);
}
