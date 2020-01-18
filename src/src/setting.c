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

#include "setting.h"

#include <string.h>

#include "port.h"

#include "setting_private.h"

Setting *setting_new(SettingType setting_type)
{
    Setting *setting = malloc(sizeof(Setting));
    if (setting == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    setting->id = NULL;
    setting->caption = NULL;
    setting->type = setting_type;
    switch (setting_type) {
        case SETTING_TYPE_UNKNOWN:
            break;
        case SETTING_TYPE_IPv4:
        case SETTING_TYPE_IPv6:
            setting->typespec.ip.method = SETTING_METHOD_UNKNOWN;
            setting->typespec.ip.addresses = addresses_new(0);
            setting->typespec.ip.routes = routes_new(0);
            setting->typespec.ip.dns_servers = dns_servers_new(0);
            setting->typespec.ip.search_domains = search_domains_new(0);

            if (setting->typespec.ip.addresses == NULL ||
                setting->typespec.ip.routes == NULL ||
                setting->typespec.ip.dns_servers == NULL ||
                setting->typespec.ip.search_domains == NULL) {

                error("Unable to create setting");
                free(setting);
                return NULL;
            }
            setting->typespec.ip.clientID = NULL;
            break;
        case SETTING_TYPE_BOND:
            setting->typespec.bond.interface_name = NULL;
            // Default values as specified by NetworkManager
            // see man nm-settings(5)
            setting->typespec.bond.mode = BOND_MODE_BALANCERR;
            setting->typespec.bond.miimon = 100;
            setting->typespec.bond.downdelay = 0;
            setting->typespec.bond.updelay = 0;
            setting->typespec.bond.arp_interval = 0;
            setting->typespec.bond.arp_ip_target = NULL;
            break;
        case SETTING_TYPE_BRIDGE:
            // Default values as specified by NetworkManager
            // see man nm-settings(5)
            setting->typespec.bridge.interface_name = NULL;
            setting->typespec.bridge.stp = true;
            setting->typespec.bridge.priority = 128;
            setting->typespec.bridge.forward_delay = 15;
            setting->typespec.bridge.hello_time = 2;
            setting->typespec.bridge.max_age = 20;
            setting->typespec.bridge.ageing_time = 500;
            break;
        case SETTING_TYPE_BRIDGE_SLAVE:
            // Default values as specified by NetworkManager
            // see man nm-settings(5)
            setting->typespec.bridge_slave.priority = 32;
            setting->typespec.bridge_slave.path_cost = 100;
            setting->typespec.bridge_slave.hairpin_mode = false;
            break;
        case SETTING_TYPE_WIRED:
            setting->typespec.wired.mac = NULL;
            break;
    }
    return setting;
}

Setting *setting_clone(const Setting *s)
{
    Setting *setting = setting_new(s->type);
    if (setting == NULL) {
        return NULL;
    }
    if (s->id != NULL) {
        setting->id = strdup(s->id);
        if (setting->id == NULL) {
            error("Memory allocation failed");
            goto err;
        }
    }
    if (s->caption != NULL) {
        setting->caption = strdup(s->caption);
        if (setting->caption == NULL) {
            error("Memory allocation failed");
            goto err;
        }
    }
    Address *address;
    size_t i;
    switch (s->type) {
        case SETTING_TYPE_IPv4:
        case SETTING_TYPE_IPv6:
            setting->typespec.ip.method = s->typespec.ip.method;
            for (i = 0; i < addresses_length(s->typespec.ip.addresses); ++i) {
                address = addresses_index(s->typespec.ip.addresses, i);
                setting_add_ip_address(setting, s->typespec.ip.method,
                    address->addr, address->prefix, address->default_gateway);
            }
            for (i = 0; i < routes_length(s->typespec.ip.routes); ++i) {
                Route *route = routes_index(s->typespec.ip.routes, i);
                if (setting_add_route(setting, route->route, route->prefix, route->next_hop, route->metric) != LMI_SUCCESS) {
                    goto err;
                }
            }
            for (i = 0; i < dns_servers_length(s->typespec.ip.dns_servers); ++i) {
                if (setting_add_dns_server(setting, dns_servers_index(s->typespec.ip.dns_servers, i)->server) != LMI_SUCCESS) {
                    goto err;
                }
            }
            for (i = 0; i < search_domains_length(s->typespec.ip.search_domains); ++i) {
                if (setting_add_search_domain(setting, search_domains_index(s->typespec.ip.search_domains, i)->domain) != LMI_SUCCESS) {
                    goto err;
                }
            }
            if (s->typespec.ip.clientID != NULL) {
                if ((setting->typespec.ip.clientID = strdup(s->typespec.ip.clientID)) == NULL) {
                    error("Memory allocation failed");
                    goto err;
                }
            }
            break;
        case SETTING_TYPE_BOND:
            if (s->typespec.bond.interface_name != NULL) {
                if ((setting->typespec.bond.interface_name = strdup(s->typespec.bond.interface_name)) == NULL) {
                    error("Memory allocation failed");
                    goto err;
                }
            }
            break;
        case SETTING_TYPE_BRIDGE:
            if (s->typespec.bridge.interface_name != NULL) {
                if ((setting->typespec.bridge.interface_name = strdup(s->typespec.bridge.interface_name)) == NULL) {
                    error("Memory allocation failed");
                    goto err;
                }
            }
            setting->typespec.bridge.stp = s->typespec.bridge.stp;
            setting->typespec.bridge.priority = s->typespec.bridge.priority;
            setting->typespec.bridge.forward_delay = s->typespec.bridge.forward_delay;
            setting->typespec.bridge.hello_time = s->typespec.bridge.hello_time;
            setting->typespec.bridge.max_age = s->typespec.bridge.max_age;
            setting->typespec.bridge.ageing_time = s->typespec.bridge.ageing_time;
            break;
        case SETTING_TYPE_BRIDGE_SLAVE:
            setting->typespec.bridge_slave.priority = s->typespec.bridge_slave.priority;
            setting->typespec.bridge_slave.path_cost = s->typespec.bridge_slave.path_cost;
            setting->typespec.bridge_slave.hairpin_mode = s->typespec.bridge_slave.hairpin_mode;
            break;
        case SETTING_TYPE_WIRED:
            if (s->typespec.wired.mac != NULL) {
                if ((setting->typespec.wired.mac = strdup(s->typespec.wired.mac)) == NULL) {
                    error("Memory allocation failed");
                    goto err;
                }
            }
            break;
        case SETTING_TYPE_UNKNOWN:
            warn("Cloning setting with UNKNOWN type");
            break;
    }
    return setting;
err:
    error("Unable to clone setting");
    if (setting != NULL) {
        setting_free(setting);
    }
    return NULL;
}

void setting_clear_addresses(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    while (addresses_length(setting->typespec.ip.addresses) > 0) {
        address_free(addresses_pop(setting->typespec.ip.addresses, 0));
    }
}

LMIResult setting_add_ip_address(Setting *setting, SettingMethod method,
                              const char *address, uint8_t prefix,
                              const char *default_gateway)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);

    setting->typespec.ip.method = method;
    Address *addr = address_new(setting->type == SETTING_TYPE_IPv4 ? IPv4 : IPv6);
    if (addr == NULL) {
        return LMI_ERROR_MEMORY;
    }
    if ((addr->addr = strdup(address)) == NULL) {
        error("Memory allocation failed");
        free(addr);
        return 1;
    }
    addr->prefix = prefix;
    if (default_gateway != NULL) {
        if ((addr->default_gateway = strdup(default_gateway)) == NULL) {
            error("Memory allocation failed");
            free(addr->addr);
            free(addr);
            return LMI_ERROR_MEMORY;
        }
    }
    return addresses_add(setting->typespec.ip.addresses, addr);
}

SettingType setting_get_type(const Setting *setting)
{
    return setting->type;
}

const char *setting_get_id(const Setting *setting)
{
    return setting->id;
}

const char *setting_get_caption(const Setting *setting)
{
    return setting->caption;
}

Addresses *setting_get_addresses(const Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return setting->typespec.ip.addresses;
}

SettingMethod setting_get_method(const Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return setting->typespec.ip.method;
}


LMIResult setting_set_method(Setting *setting, SettingMethod method)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    setting->typespec.ip.method = method;
    return LMI_SUCCESS;
}

const char *setting_get_clientID(const Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return setting->typespec.ip.clientID;
}

LMIResult setting_set_clientID(Setting *setting, const char *clientID)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    if ((setting->typespec.ip.clientID = strdup(clientID)) == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    return LMI_SUCCESS;
}

void setting_clear_dns_servers(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    while (dns_servers_length(setting->typespec.ip.dns_servers) > 0) {
        dns_server_free(dns_servers_pop(setting->typespec.ip.dns_servers, 0));
    }
}

LMIResult setting_add_dns_server(Setting *setting, const char *dns_server)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    DNSServer *dns = dns_server_new(setting->type == SETTING_TYPE_IPv4 ? IPv4 : IPv6, dns_server);
    if (dns == NULL) {
        return LMI_ERROR_MEMORY;
    }
    LMIResult res;
    if ((res = dns_servers_add(setting->typespec.ip.dns_servers, dns)) != LMI_SUCCESS) {
        dns_server_free(dns);
        return LMI_ERROR_MEMORY;
    }
    return LMI_SUCCESS;
}

size_t setting_get_dns_servers_length(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return dns_servers_length(setting->typespec.ip.dns_servers);
}

const char *setting_get_dns_server(Setting *setting, size_t index)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    assert(index < dns_servers_length(setting->typespec.ip.dns_servers));
    return dns_servers_index(setting->typespec.ip.dns_servers, index)->server;
}

void setting_clear_search_domains(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    while (search_domains_length(setting->typespec.ip.search_domains) > 0) {
        search_domain_free(search_domains_pop(setting->typespec.ip.search_domains, 0));
    }
}

LMIResult setting_add_search_domain(Setting *setting, const char *search_domain)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    SearchDomain *domain = search_domain_new(setting->type == SETTING_TYPE_IPv4 ? IPv4 : IPv6, search_domain);
    if (domain == NULL) {
        return LMI_ERROR_MEMORY;
    }
    LMIResult res;
    if ((res = search_domains_add(setting->typespec.ip.search_domains, domain)) != LMI_SUCCESS) {
        search_domain_free(domain);
        return res;
    }
    return LMI_SUCCESS;
}

size_t setting_get_search_domains_length(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return search_domains_length(setting->typespec.ip.search_domains);
}

const char *setting_get_search_domain (Setting *setting, size_t index)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    assert(index < search_domains_length(setting->typespec.ip.search_domains));
    return search_domains_index(setting->typespec.ip.search_domains, index)->domain;
}

LMIResult setting_add_route(Setting *setting, const char *route, uint32_t prefix, const char *next_hop, uint32_t metric)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    Route *r = route_new(setting->type == SETTING_TYPE_IPv4 ? IPv4 : IPv6);
    if (r == NULL) {
        return LMI_ERROR_MEMORY;
    }
    LMIResult res;
    if (route != NULL) {
        if ((r->route = strdup(route)) == NULL) {
            error("Memory allocation failed");
            res = LMI_ERROR_MEMORY;
            goto err;
        }
    }
    r->prefix = prefix;
    if (next_hop != NULL) {
        if ((r->next_hop = strdup(next_hop)) == NULL) {
            error("Memory allocation failed");
            res = LMI_ERROR_MEMORY;
            goto err;
        }
    }
    r->metric = metric;
    if ((res = routes_add(setting->typespec.ip.routes, r)) != LMI_SUCCESS) {
        goto err;
    }
    return LMI_SUCCESS;
err:
    route_free(r);
    return res;
}

void setting_clear_routes(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    while (routes_length(setting->typespec.ip.routes) > 0) {
        route_free(routes_pop(setting->typespec.ip.routes, 0));
    }
}

size_t setting_get_routes_length(Setting *setting)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    return routes_length(setting->typespec.ip.routes);
}

Route *setting_get_route(Setting *setting, size_t index)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    assert(index < routes_length(setting->typespec.ip.routes));
    return routes_index(setting->typespec.ip.routes, index);
}

LMIResult setting_delete_route(Setting *setting, size_t index)
{
    assert(setting->type == SETTING_TYPE_IPv4 || setting->type == SETTING_TYPE_IPv6);
    assert(index < routes_length(setting->typespec.ip.routes));
    Route *route = routes_pop(setting->typespec.ip.routes, index);
    if (route == NULL) {
        return LMI_ERROR_CONNECTION_DELETE_FAILED;
    }
    route_free(route);
    return LMI_SUCCESS;
}

const char *setting_get_interface_name(const Setting *setting)
{
    switch (setting->type) {
        case SETTING_TYPE_BOND:
            return setting->typespec.bond.interface_name;
        case SETTING_TYPE_BRIDGE:
            return setting->typespec.bridge.interface_name;
        default:
            return NULL;
    }
}

void setting_free(Setting *setting)
{
    if (setting == NULL) {
        return;
    }
    free(setting->id);
    free(setting->caption);
    switch (setting->type) {
        case SETTING_TYPE_IPv4:
        case SETTING_TYPE_IPv6:
            addresses_free(setting->typespec.ip.addresses, true);
            routes_free(setting->typespec.ip.routes, true);
            dns_servers_free(setting->typespec.ip.dns_servers, true);
            search_domains_free(setting->typespec.ip.search_domains, true);
            free(setting->typespec.ip.clientID);
            break;
        case SETTING_TYPE_BOND:
            free(setting->typespec.bond.interface_name);
            ip_addresses_free(setting->typespec.bond.arp_ip_target, true);
            break;
        case SETTING_TYPE_BRIDGE:
            free(setting->typespec.bridge.interface_name);
            break;
        case SETTING_TYPE_WIRED:
            free(setting->typespec.wired.mac);
            break;
        case SETTING_TYPE_BRIDGE_SLAVE:
            // Nothing to free
            break;
        case SETTING_TYPE_UNKNOWN:
            break;
    }
    free(setting);
}

#define ip_address_free free
LIST_IMPL2(IPAddress, ip_address, IPAddresses, ip_addresses)

LIST_IMPL(Setting, setting)

Setting *settings_find_by_type(const Settings *settings, SettingType type)
{
    if (settings == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < settings->length; ++i) {
        if (settings->settings[i]->type == type) {
            return settings->settings[i];
        }
    }
    return NULL;
}

BridgeSetting *setting_get_bridge_setting(Setting *setting)
{
    return &(setting->typespec.bridge);
}

BridgeSlaveSetting *setting_get_bridge_slave_setting(Setting *setting)
{
    return &(setting->typespec.bridge_slave);
}

BondSetting *setting_get_bond_setting(Setting *setting)
{
    return &(setting->typespec.bond);
}


