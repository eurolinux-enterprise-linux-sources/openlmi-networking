/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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

#include "ipconfig.h"
#include "globals.h"
#include <string.h>

DNSServer *dns_server_new(ProtocolType type, const char *server)
{
    DNSServer *s = malloc(sizeof(DNSServer));
    if (s == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    s->type = type;
    if (server != NULL) {
        if ((s->server = strdup(server)) == NULL) {
            error("Memory allocation failed");
            free(s);
            return NULL;
        }
    } else {
        s->server = NULL;
    }
    return s;
}

void dns_server_free(DNSServer *server)
{
    if (server == NULL) {
        return;
    }
    free(server->server);
    free(server);
}

SearchDomain *search_domain_new(ProtocolType type, const char *domain)
{
    SearchDomain *s = malloc(sizeof(SearchDomain));
    if (s == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    s->type = type;
    if (domain != NULL) {
        if ((s->domain = strdup(domain)) == NULL) {
            error("Memory allocation failed");
            free(s);
            return NULL;
        }
    } else {
        s->domain = NULL;
    }
    return s;
}

void search_domain_free(SearchDomain *domain)
{
    if (domain == NULL) {
        return;
    }
    if (domain->domain != NULL) {
        free(domain->domain);
    }
    free(domain);
}

Address *address_new(ProtocolType type)
{
    Address *address = malloc(sizeof(Address));
    if (address == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    address->type = type;
    address->addr = NULL;
    address->prefix = 0;
    address->default_gateway = NULL;
    return address;
}

void address_free(Address *address)
{
    if (address == NULL) {
        return;
    }
    free(address->addr);
    free(address->default_gateway);
    free(address);
}

Route *route_new(ProtocolType type)
{
    Route *route = malloc(sizeof(Route));
    if (route == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    route->type = type;
    route->route = NULL;
    route->prefix = 0;
    route->next_hop = NULL;
    route->metric = 0;
    return route;
}

void route_free(Route *route)
{
    if (route == NULL) {
        return;
    }
    free(route->route);
    free(route->next_hop);
    free(route);
}

LIST_IMPL2(Address, address, Addresses, addresses)
LIST_IMPL(Route, route)
LIST_IMPL(DNSServer, dns_server)
LIST_IMPL(SearchDomain, search_domain)

IPConfig *ipconfig_new(void)
{
    IPConfig *ipconfig = malloc(sizeof(IPConfig));
    if (ipconfig == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    ipconfig->method = SETTING_METHOD_UNKNOWN;
    ipconfig->addresses = addresses_new(0);
    ipconfig->routes = routes_new(0);
    ipconfig->dns_servers = dns_servers_new(0);
    ipconfig->search_domains = search_domains_new(0);
    ipconfig->clientID = NULL;

    if (ipconfig->addresses == NULL ||
        ipconfig->routes == NULL ||
        ipconfig->dns_servers == NULL ||
        ipconfig->search_domains == NULL) {

        error("Memory allocation failed");
        ipconfig_free(ipconfig);
        return NULL;
    }
    return ipconfig;
}

void ipconfig_free(IPConfig *ipconfig)
{
    if (ipconfig == NULL) {
        return;
    }
    addresses_free(ipconfig->addresses, true);
    routes_free(ipconfig->routes, true);
    dns_servers_free(ipconfig->dns_servers, true);
    search_domains_free(ipconfig->search_domains, true);
    if (ipconfig->clientID != NULL) {
        free(ipconfig->clientID);
    }
    free(ipconfig);
}
