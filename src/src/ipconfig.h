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

#ifndef IPCONFIG_H
#define IPCONFIG_H

#include "globals.h"

typedef struct DNSServer {
    ProtocolType type;
    char *server;
} DNSServer;
typedef struct DNSServers DNSServers;
LIST_DECL(DNSServer, dns_server)

DNSServer *dns_server_new(ProtocolType type, const char *server);
void dns_server_free(DNSServer *server);

typedef struct SearchDomain
{
    ProtocolType type;
    char *domain;
} SearchDomain;
typedef struct SearchDomains SearchDomains;
LIST_DECL(SearchDomain, search_domain)

SearchDomain *search_domain_new(ProtocolType type, const char *domain);
void search_domain_free(SearchDomain *domain);


typedef struct Address {
    ProtocolType type;
    char *addr;
    uint8_t prefix;
    char *default_gateway;
} Address;

typedef struct Addresses Addresses;
LIST_DECL2(Address, address, Addresses, addresses)

Address *address_new(ProtocolType type);

void address_free(Address *address);

typedef struct Route
{
    ProtocolType type;
    char *route;
    uint32_t prefix;
    char *next_hop;
    uint32_t metric;
} Route;

typedef struct Routes Routes;
LIST_DECL(Route, route)

Route *route_new(ProtocolType type);

void route_free(Route *route);

typedef struct IPConfig
{
    SettingMethod method;
    Addresses *addresses;
    Routes *routes;
    DNSServers *dns_servers;
    SearchDomains *search_domains;
    char *clientID;
} IPConfig;

IPConfig *ipconfig_new(void);
void ipconfig_free(IPConfig *ipconfig);

#endif
