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

#include "port_nm.h"

#include <string.h>

#include "dbus_wrapper.h"

#include "nm_support.h"
#include "network_nm.h"

#include "port_private.h"
#include "network_private.h"
#include "errors.h"

typedef struct PortPriv {
    DBusGProxy *proxy;
    DBusGProxy *subproxy;
    char *subinterface;
    GHashTable *properties;
    GHashTable *subproperties;
    int nm_state;
    int nm_state_reason;
    Network *network; // Needed for ability to update properties when port state changes
} PortPriv;

void port_state_changed_cb(void *proxy, unsigned int state1, unsigned int state2, unsigned int state3, Port *port);
void port_subproperties_changed_cb(void *subproxy, GHashTable *subproperties, Port *port);
const char *port_interface_from_type(PortType type);
LMIResult port_read_properties(Port *port);

/* This ugly function is required to handle DBus signal that has 3 uint arguments */
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
void
_marshal_VOID__UINT_UINT_UINT (GClosure *closure, GValue *return_value G_GNUC_UNUSED,
        guint n_param_values, const GValue *param_values,
        gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
    typedef void (*GMarshalFunc_VOID__UINT_UINT_UINT) (gpointer data1,
            guint arg_1, guint arg_2, guint arg_3, gpointer data2);
    register GMarshalFunc_VOID__UINT_UINT_UINT callback;
    register GCClosure *cc = (GCClosure *) closure;
    register gpointer data1, data2;

    g_return_if_fail (n_param_values == 4);

    if (G_CCLOSURE_SWAP_DATA(closure))
    {
        data1 = closure->data;
        data2 = g_value_peek_pointer(param_values + 0);
    } else {
        data1 = g_value_peek_pointer(param_values + 0);
        data2 = closure->data;
    }

    callback = (GMarshalFunc_VOID__UINT_UINT_UINT) (size_t) (marshal_data ? marshal_data : cc->callback);
    callback(data1,
             g_marshal_value_peek_uint(param_values + 1),
             g_marshal_value_peek_uint(param_values + 2),
             g_marshal_value_peek_uint(param_values + 3),
             data2);
}

Port *port_new_from_objectpath(Network *network, const char *objectpath)
{
    Port *port = port_new();
    if (port == NULL) {
        return NULL;
    }
    if ((port->uuid = strdup(objectpath)) == NULL) {
        error("Memory allocation failed");
        port_free(port);
        return NULL;
    }
    PortPriv *priv = malloc(sizeof(PortPriv));
    if (priv == NULL) {
        error("Memory allocation failed");
        port_free(port);
        return NULL;
    }
    port->priv = priv;
    priv->network = network;
    priv->proxy = dbus_g_proxy_new_for_name(network_priv_get_dbus_connection(network), NM_SERVICE_DBUS, objectpath, NM_DBUS_INTERFACE_DEVICE);
    if (priv->proxy == NULL) {
        error("Unable to create DBus proxy: %s %s " NM_DBUS_INTERFACE_DEVICE, NM_SERVICE_DBUS, objectpath);
        port_free(port);
        return NULL;
    }

    GValue *v = dbus_get_property(priv->proxy, NULL, NM_DBUS_INTERFACE_DEVICE, "DeviceType");
    if (v == NULL) {
        error("Unable to read property \"DeviceType\" of Device %s", port->id);
        port_free(port);
        return NULL;
    } else {
        switch (g_value_get_uint(v)) {
            case NM_DEVICE_TYPE_ETHERNET:
                port->type = TYPE_ETHERNET;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_WIRED;
                break;
            case NM_DEVICE_TYPE_WIFI:
                port->type = TYPE_WIFI;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_WIRELESS;
                break;
            case NM_DEVICE_TYPE_BT:
                port->type = TYPE_BT;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_BLUETOOTH;
                break;
            case NM_DEVICE_TYPE_OLPC_MESH:
                port->type = TYPE_OLPC_MESH;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_OLPC_MESH;
                break;
            case NM_DEVICE_TYPE_WIMAX:
                port->type = TYPE_WIMAX;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_WIMAX;
                break;
            case NM_DEVICE_TYPE_MODEM:
                port->type = TYPE_MODEM;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_MODEM;
                break;
            case NM_DEVICE_TYPE_INFINIBAND:
                port->type = TYPE_INFINIBAND;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_INFINIBAND;
                break;
            case NM_DEVICE_TYPE_BOND:
                port->type = TYPE_BOND;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_BOND;
                break;
            case NM_DEVICE_TYPE_BRIDGE:
                port->type = TYPE_BRIDGE;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_BRIDGE;
                break;
            case NM_DEVICE_TYPE_VLAN:
                port->type = TYPE_VLAN;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_VLAN;
                break;
            case NM_DEVICE_TYPE_ADSL:
                port->type = TYPE_ADSL;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_ADSL;
                break;
#ifdef NM_DBUS_INTERFACE_DEVICE_GENERIC
            case NM_DEVICE_TYPE_GENERIC:
                port->type = TYPE_GENERIC;
                priv->subinterface = NM_DBUS_INTERFACE_DEVICE_GENERIC;
                break;
#endif
            default:
                port->type = TYPE_UNKNOWN;
                priv->subinterface = NULL;
                warn("Unknown device type (%u) for device %s",
                     g_value_get_uint(v), port->uuid);
        }
    }

    dbus_g_object_register_marshaller(_marshal_VOID__UINT_UINT_UINT, G_TYPE_NONE, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(priv->proxy, "StateChanged", G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal(priv->proxy, "StateChanged", G_CALLBACK(port_state_changed_cb), port, NULL);

    if (priv->subinterface != NULL) {
        priv->subproxy = dbus_g_proxy_new_for_name(network_priv_get_dbus_connection(network), NM_SERVICE_DBUS, objectpath, priv->subinterface);
        dbus_g_proxy_add_signal(priv->subproxy, "PropertiesChanged", DBUS_TYPE_G_MAP_OF_VARIANT, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal(priv->subproxy, "PropertiesChanged", G_CALLBACK(port_subproperties_changed_cb), port, NULL);
    }

    if (port_read_properties(port) != LMI_SUCCESS) {
        error("Unable to read port properties");
        port_free(port);
        return NULL;
    }

    return port;
}

PortOperatingStatus port_status_from_nm_state(uint32_t state)
{
    switch (state) {
        // The device is in an unknown state
        case NM_DEVICE_STATE_UNKNOWN:
            return STATUS_UNKNOWN;
            break;
        // The device is recognized but not managed by NetworkManager
        case NM_DEVICE_STATE_UNMANAGED:
            return STATUS_NA;
            break;
        // The device cannot be used (carrier off, rfkill, etc)
        case NM_DEVICE_STATE_UNAVAILABLE:
            return STATUS_OFFLINE;
            break;
        // The device is not connected
        case NM_DEVICE_STATE_DISCONNECTED:
            return STATUS_STOPPED;
            break;
        // The device is preparing to connect
        case NM_DEVICE_STATE_PREPARE:
        // The device is being configured
        case NM_DEVICE_STATE_CONFIG:
        // The device is awaiting secrets necessary to continue connection
        case NM_DEVICE_STATE_NEED_AUTH:
        // The IP settings of the device are being requested and configured
        case NM_DEVICE_STATE_IP_CONFIG:
        // The device's IP connectivity ability is being determined
        case NM_DEVICE_STATE_IP_CHECK:
        // The device is waiting for secondary connections to be activated
        case NM_DEVICE_STATE_SECONDARIES:
            return STATUS_STARTING;
            break;
        // The device is active
        case NM_DEVICE_STATE_ACTIVATED:
            return STATUS_IN_SERVICE;
            break;
        // The device's network connection is being torn down
        case NM_DEVICE_STATE_DEACTIVATING:
            return STATUS_STOPPING;
            break;
        // The device is in a failure state following an attempt to activate it
        case NM_DEVICE_STATE_FAILED:
            return STATUS_ABORTED;
            break;
    }
    return STATUS_UNKNOWN;
}

void port_priv_free(void *priv)
{
    PortPriv *p = priv;

    if (p->proxy != NULL) {
        dbus_g_proxy_disconnect_signal(p->proxy, "StateChanged", G_CALLBACK(port_state_changed_cb), NULL);
    }
    if (p->subproxy != NULL) {
        dbus_g_proxy_disconnect_signal(p->subproxy, "PropertiesChanged", G_CALLBACK(port_subproperties_changed_cb), NULL);
    }

    if (p->properties != NULL) {
        g_hash_table_destroy(p->properties);
    }
    if (p->subproperties != NULL) {
        g_hash_table_destroy(p->subproperties);
    }
    if (p->proxy != NULL) {
        g_object_unref(p->proxy);
    }
    if (p->subproxy != NULL) {
        g_object_unref(p->subproxy);
    }
    free(p);
}

Ports *port_priv_get_slaves(Network *network, const Port *port)
{
    PortPriv *priv = port->priv;
    if (priv->subproperties == NULL) {
        return NULL;
    }
    GValue *v = g_hash_table_lookup(priv->subproperties, "Slaves");
    if (v == NULL) {
        return NULL;
    }
    if (!G_VALUE_HOLDS_BOXED(v)) {
        warn("Property Slaves doesn't hold boxed but %s", G_VALUE_TYPE_NAME(v));
        return NULL;
    }

    Ports *slaves = ports_new(2);
    GPtrArray *slave_names = g_value_get_boxed(v);
    if (slave_names == NULL) {
        debug("Port %s doesn't have any slaves", port->id);
        return slaves;
    }
    const char *slave_name;
    size_t j;
    const Ports *ports = network_get_ports(network);
    bool found;
    for (guint i = 0; i < slave_names->len; ++i) {
        slave_name = g_ptr_array_index(slave_names, i);
        found = false;
        for (j = 0; j < ports_length(ports); ++j) {
            if (strcmp(slave_name, port_get_uuid(ports_index(ports, j))) == 0) {
                if (ports_add(slaves, ports_index(ports, j)) != LMI_SUCCESS) {
                    ports_free(slaves, false);
                    return NULL;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            warn("No such port with this object path: %s", slave_name);
        }
    }
    return slaves;
}

LMIResult port_read_ipconfig(Port *port, const char *ip4config, const char *ip6config)
{
    LMIResult res = LMI_SUCCESS;
    PortPriv *priv = port->priv;

    if (port->ipconfig != NULL) {
        ipconfig_free(port->ipconfig);
    }
    if ((port->ipconfig = ipconfig_new()) == NULL) {
        return LMI_ERROR_MEMORY;
    }

    GPtrArray *addresses;
    GHashTable *ipproperties;

    Address *address;
    GArray *array;

    GValue *v;

    if (ip4config != NULL && strcmp(ip4config, "/") != 0) {
        ipproperties = dbus_get_properties(priv->proxy, ip4config, NM_DBUS_INTERFACE_IP4_CONFIG);
        if (ipproperties != NULL) {
            addresses = dbus_property_array(ipproperties, "Addresses");
            if (addresses != NULL) {
                for (guint i = 0; i < addresses->len; ++i) {
                    array = (GArray *) g_ptr_array_index(addresses, i);
                    address = ipv4_array_to_address(array);
                    if (address == NULL) {
                        warn("IPv4 address is invalid");
                        continue;
                    }
                    if ((res = addresses_add(port->ipconfig->addresses, address)) != LMI_SUCCESS) {
                        g_hash_table_destroy(ipproperties);
                        return res;
                    }
                }
            } else {
                warn("No address for Ip4Config on port %s", port->id);
            }

            v = g_hash_table_lookup(ipproperties, "Nameservers");
            if (v != NULL) {
                if ((res = dns_servers4_fill_from_gvalue(port->ipconfig->dns_servers, v)) != LMI_SUCCESS) {
                    g_hash_table_destroy(ipproperties);
                    return res;
                }
            }

            v = g_hash_table_lookup(ipproperties, "Routes");
            if (v != NULL) {
                if ((res = routes4_fill_from_gvalue(port->ipconfig->routes, v)) != LMI_SUCCESS) {
                    g_hash_table_destroy(ipproperties);
                    return res;
                }
            }
            g_hash_table_destroy(ipproperties);
        } else {
            error("No IPv4 properties on device %s (%s)", port->id, port->uuid);
        }
    }

    if (ip6config && strcmp(ip6config, "/") != 0) {
        ipproperties = dbus_get_properties(priv->proxy, ip6config, NM_DBUS_INTERFACE_IP6_CONFIG);
        if (ipproperties != NULL) {
            addresses = dbus_property_array(ipproperties, "Addresses");

            if (addresses) {
                for (guint i = 0; i < addresses->len; ++i) {
                    address = ipv6_array_to_address(g_ptr_array_index(addresses, i));
                    if (address == NULL) {
                        warn("IPv6 config (%s) is invalid", ip6config);
                        continue;
                    }

                    if ((res = addresses_add(port->ipconfig->addresses, address)) != LMI_SUCCESS) {
                        g_hash_table_destroy(ipproperties);
                        return res;
                    }
                }
            } else {
                warn("No address for Ip6Config on port %s", port->id);
            }

            v = g_hash_table_lookup(ipproperties, "Nameservers");
            if (v != NULL) {
                if ((res = dns_servers6_fill_from_gvalue(port->ipconfig->dns_servers, v)) != LMI_SUCCESS) {
                    g_hash_table_destroy(ipproperties);
                    return res;
                }
            }

            v = g_hash_table_lookup(ipproperties, "Routes");
            if (v != NULL) {
                if ((res = routes6_fill_from_gvalue(port->ipconfig->routes, v)) != LMI_SUCCESS) {
                    g_hash_table_destroy(ipproperties);
                    return res;
                }
            }

            g_hash_table_destroy(ipproperties);
        } else {
            error("No IPv6 properties on device %s (%s)", port->id, port->uuid);
        }
    }
    return LMI_SUCCESS;
}

LMIResult port_read_properties(Port *port)
{
    LMIResult res = LMI_SUCCESS;
    PortPriv *priv = port->priv;
    GHashTable *hash = dbus_get_properties(priv->proxy, NULL, NM_DBUS_INTERFACE_DEVICE);
    if (hash == NULL) {
        error("Unable to get properties for %s", port->uuid);
        return LMI_ERROR_BACKEND;
    }
    priv->properties = hash;

    if (priv->subinterface) {
        hash = dbus_get_properties(priv->subproxy, NULL, priv->subinterface);
        if (hash == NULL) {
            error("Unable to get subproperties for port %s (%s)", port->uuid, priv->subinterface);
            return LMI_ERROR_BACKEND;
        }
        priv->subproperties = hash;
    } else {
        priv->subproperties = NULL;
    }

    const char *iface = dbus_property_string(priv->properties, "Interface");
    if (iface == NULL) {
        error("Device with path %s don't have Interface property", port->uuid);
    } else {
        if ((port->id = strdup(iface)) == NULL) {
            error("Memory allocation failed");
            return LMI_ERROR_MEMORY;
        }
    }

    port->operatingStatus = port_status_from_nm_state(dbus_property_uint(priv->properties, "State"));

    const char *ip4config = dbus_property_objectpath(priv->properties, "Ip4Config");
    const char *ip6config = dbus_property_objectpath(priv->properties, "Ip6Config");

    if ((res = port_read_ipconfig(port, ip4config, ip6config)) != LMI_SUCCESS) {
        error("Port IP configuration is invalid");
        return res;
    }

    // Don't read subproperties if there is no interface for them
    if (priv->subproperties == NULL) {
        port->mac = NULL;
        port->permmac = NULL;
        return LMI_SUCCESS;
    }

    // Get MAC address
    const char *mac = dbus_property_string(priv->subproperties, "HwAddress");
    if (mac) {
        if ((port->mac = strdup(mac)) == NULL) {
            error("Memory allocation failed");
            return LMI_ERROR_MEMORY;
        }
    } else {
        port->mac = NULL;
    }

    // Get permananent MAC address
    mac = dbus_property_string(priv->subproperties, "PermHwAddress");
    if (mac) {
        if ((port->permmac = strdup(mac)) == NULL) {
            error("Memory allocation failed");
            return LMI_ERROR_MEMORY;
        }
    } else {
        port->permmac = NULL;
    }
    // Get Carrier
    port->carrier = dbus_property_bool(priv->subproperties, "Carrier", false);

    if (port->type == TYPE_ETHERNET) {
        port->typespec.ethernet.speed = dbus_property_uint(priv->subproperties, "Speed");
    }

    return LMI_SUCCESS;
}

void port_state_changed_cb(void *proxy, unsigned int state1, unsigned int state2, unsigned int state3, Port *port)
{
    debug("Port %s state changed: %d %d %d", port->id, state1, state2, state3);
    PortPriv *priv = port->priv;
    network_lock(priv->network);
    void *data = NULL;
    if (priv->network->port_pre_changed_callback != NULL) {
        data = priv->network->port_pre_changed_callback(priv->network, port,
                priv->network->port_pre_changed_callback_data);
    }

    priv->nm_state = state1;
    priv->nm_state_reason = state3;
    if (port_read_properties(port) != LMI_SUCCESS) {
        error("Unable to read port properties");
    }

    if (priv->network->port_changed_callback != NULL) {
        priv->network->port_changed_callback(priv->network, port,
                priv->network->port_changed_callback_data, data);
    }

    network_unlock(priv->network);
}

void port_subproperties_changed_cb(void *subproxy, GHashTable *subproperties, Port *port)
{
    debug("Port %s subproperties changed", port->id);
    PortPriv *priv = port->priv;
    network_lock(priv->network);
    void *data = NULL;
    if (priv->network->port_pre_changed_callback != NULL) {
        data = priv->network->port_pre_changed_callback(priv->network, port,
                priv->network->port_pre_changed_callback_data);
    }
    if (port_read_properties(port) != LMI_SUCCESS) {
        error("Unable to read port properties");
    }
    if (priv->network->port_changed_callback != NULL) {
        priv->network->port_changed_callback(priv->network, port,
                priv->network->port_changed_callback_data, data);
    }
    network_unlock(priv->network);
}

int port_priv_disconnect(Port *port)
{
    PortPriv *priv = port->priv;
    GError *err = NULL;
    if (!dbus_g_proxy_call(priv->proxy, "Disconnect", &err, G_TYPE_INVALID, G_TYPE_INVALID)) {
        error("Unable to disconnect port %s: %s", port->id, err->message);
        return LMI_ERROR_PORT_STATE_CHANGE_FAILED;
    }
    return LMI_SUCCESS;
}

const char *port_priv_get_state_reason(const Port *port)
{
    PortPriv *priv = port->priv;
    switch (priv->nm_state_reason) {
        case NM_DEVICE_STATE_REASON_NONE:
            warn("Unkown port state reason: %d", priv->nm_state_reason);
            return NULL;
        case NM_DEVICE_STATE_REASON_UNKNOWN:
            return "Unknown error";
        case NM_DEVICE_STATE_REASON_NOW_MANAGED:
            return "Device is now managed";
        case NM_DEVICE_STATE_REASON_NOW_UNMANAGED:
            return "Device is now unmanaged";
        case NM_DEVICE_STATE_REASON_CONFIG_FAILED:
            return "The device could not be readied for configuration";
        case NM_DEVICE_STATE_REASON_IP_CONFIG_UNAVAILABLE:
            return "IP configuration could not be reserved (no available address, timeout, etc)";
        case NM_DEVICE_STATE_REASON_IP_CONFIG_EXPIRED:
            return "The IP config is no longer valid";
        case NM_DEVICE_STATE_REASON_NO_SECRETS:
            return "Secrets were required, but not provided";
        case NM_DEVICE_STATE_REASON_SUPPLICANT_DISCONNECT:
            return "802.1x supplicant disconnected";
        case NM_DEVICE_STATE_REASON_SUPPLICANT_CONFIG_FAILED:
            return "802.1x supplicant configuration failed";
        case NM_DEVICE_STATE_REASON_SUPPLICANT_FAILED:
            return "802.1x supplicant failed";
        case NM_DEVICE_STATE_REASON_SUPPLICANT_TIMEOUT:
            return "802.1x supplicant took too long to authenticate";
        case NM_DEVICE_STATE_REASON_PPP_START_FAILED:
            return "PPP service failed to start";
        case NM_DEVICE_STATE_REASON_PPP_DISCONNECT:
            return "PPP service disconnected";
        case NM_DEVICE_STATE_REASON_PPP_FAILED:
            return "PPP failed";
        case NM_DEVICE_STATE_REASON_DHCP_START_FAILED:
            return "DHCP client failed to start";
        case NM_DEVICE_STATE_REASON_DHCP_ERROR:
            return "DHCP client error";
        case NM_DEVICE_STATE_REASON_DHCP_FAILED:
            return "DHCP client failed";
        case NM_DEVICE_STATE_REASON_SHARED_START_FAILED:
            return "Shared connection service failed to start";
        case NM_DEVICE_STATE_REASON_SHARED_FAILED:
            return "Shared connection service failed";
        case NM_DEVICE_STATE_REASON_AUTOIP_START_FAILED:
            return "AutoIP service failed to start";
        case NM_DEVICE_STATE_REASON_AUTOIP_ERROR:
            return "AutoIP service error";
        case NM_DEVICE_STATE_REASON_AUTOIP_FAILED:
            return "AutoIP service failed";
        case NM_DEVICE_STATE_REASON_MODEM_BUSY:
            return "The line is busy";
        case NM_DEVICE_STATE_REASON_MODEM_NO_DIAL_TONE:
            return "No dial tone";
        case NM_DEVICE_STATE_REASON_MODEM_NO_CARRIER:
            return "No carrier could be established";
        case NM_DEVICE_STATE_REASON_MODEM_DIAL_TIMEOUT:
            return "The dialing request timed out";
        case NM_DEVICE_STATE_REASON_MODEM_DIAL_FAILED:
            return "The dialing attempt failed";
        case NM_DEVICE_STATE_REASON_MODEM_INIT_FAILED:
            return "Modem initialization failed";
        case NM_DEVICE_STATE_REASON_GSM_APN_FAILED:
            return "Failed to select the specified APN";
        case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_NOT_SEARCHING:
            return "Not searching for networks";
        case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_DENIED:
            return "Network registration denied";
        case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_TIMEOUT:
            return "Network registration timed out";
        case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_FAILED:
            return "Failed to register with the requested network";
        case NM_DEVICE_STATE_REASON_GSM_PIN_CHECK_FAILED:
            return "PIN check failed";
        case NM_DEVICE_STATE_REASON_FIRMWARE_MISSING:
            return "Necessary firmware for the device may be missing";
        case NM_DEVICE_STATE_REASON_REMOVED:
            return "The device was removed";
        case NM_DEVICE_STATE_REASON_SLEEPING:
            return "NetworkManager went to sleep";
        case NM_DEVICE_STATE_REASON_CONNECTION_REMOVED:
            return "The device's active connection disappeared";
        case NM_DEVICE_STATE_REASON_USER_REQUESTED:
            return "Device disconnected by user or client";
        case NM_DEVICE_STATE_REASON_CARRIER:
            return "Carrier/link changed";
        case NM_DEVICE_STATE_REASON_CONNECTION_ASSUMED:
            return "The device's existing connection was assumed";
        case NM_DEVICE_STATE_REASON_SUPPLICANT_AVAILABLE:
            return "The supplicant is now available";
        case NM_DEVICE_STATE_REASON_MODEM_NOT_FOUND:
            return "The modem could not be found";
        case NM_DEVICE_STATE_REASON_BT_FAILED:
            return "The Bluetooth connection failed or timed out";
        case NM_DEVICE_STATE_REASON_GSM_SIM_NOT_INSERTED:
            return "GSM Modem's SIM Card not inserted";
        case NM_DEVICE_STATE_REASON_GSM_SIM_PIN_REQUIRED:
            return "GSM Modem's SIM Pin required";
        case NM_DEVICE_STATE_REASON_GSM_SIM_PUK_REQUIRED:
            return "GSM Modem's SIM Puk required";
        case NM_DEVICE_STATE_REASON_GSM_SIM_WRONG:
            return "GSM Modem's SIM wrong";
        case NM_DEVICE_STATE_REASON_INFINIBAND_MODE:
            return "InfiniBand device does not support connected mode";
        case NM_DEVICE_STATE_REASON_DEPENDENCY_FAILED:
            return "A dependency of the connection failed";
        case NM_DEVICE_STATE_REASON_BR2684_FAILED:
            return "Problem with the RFC 2684 Ethernet over ADSL bridge";
        case NM_DEVICE_STATE_REASON_MODEM_MANAGER_UNAVAILABLE:
            return "ModemManager not running";
        case NM_DEVICE_STATE_REASON_SSID_NOT_FOUND:
            return "The WiFi network could not be found";
        case NM_DEVICE_STATE_REASON_SECONDARY_CONNECTION_FAILED:
            return "A secondary connection of the base connection failed";
    }
    return NULL;
}
