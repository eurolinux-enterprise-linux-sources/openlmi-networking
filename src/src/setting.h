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

#ifndef SETTING_H
#define SETTING_H

#include "globals.h"
#include "ipconfig.h"

Setting *setting_new(SettingType setting_type);
Setting *setting_clone(const Setting *s);

const char *setting_get_id(const Setting *setting);
const char *setting_get_caption(const Setting *setting);
SettingType setting_get_type(const Setting *setting);

// IP
void setting_clear_addresses(Setting *setting);
LMIResult setting_add_ip_address(Setting *setting, SettingMethod method, const char *address, uint8_t prefix, const char *default_gateway);
Addresses *setting_get_addresses(const Setting *setting);

LMIResult setting_set_method(Setting *setting, SettingMethod method);
SettingMethod setting_get_method(const Setting *setting);

// DNS
void setting_clear_dns_servers(Setting *setting);
LMIResult setting_add_dns_server(Setting *setting, const char *dns_server);
size_t setting_get_dns_servers_length(Setting *setting);
const char *setting_get_dns_server(Setting *setting, size_t index);

void setting_clear_search_domains(Setting *setting);
LMIResult setting_add_search_domain(Setting *setting, const char *search_domain);
size_t setting_get_search_domains_length(Setting *setting);
const char *setting_get_search_domain(Setting *setting, size_t index);

// Routes
void setting_clear_routes(Setting *setting);
LMIResult setting_add_route(Setting *setting, const char *route, uint32_t prefix,
        const char *next_hop, uint32_t metric);
size_t setting_get_routes_length(Setting *setting);
Route *setting_get_route(Setting *setting, size_t index);
LMIResult setting_delete_route(Setting *setting, size_t index);

// DHCP
const char *setting_get_clientID(const Setting *setting);
LMIResult setting_set_clientID(Setting *setting, const char *clientID);

// Bridging/bonding
typedef enum BondMode {
    BOND_MODE_BALANCERR=0, BOND_MODE_ACTIVEBACKUP, BOND_MODE_BALANCEXOR,
    BOND_MODE_BROADCAST, BOND_MODE_8023AD, BOND_MODE_BALANCETLB,
    BOND_MODE_BALANCEALB
} BondMode;

typedef char IPAddress;
typedef struct IPAddresses IPAddresses;
LIST_DECL2(IPAddress, ip_address, IPAddresses, ip_addresses)

typedef struct BondSetting {
    char *interface_name;
    BondMode mode;
    uint64_t miimon;
    uint64_t downdelay;
    uint64_t updelay;
    uint64_t arp_interval;
    IPAddresses *arp_ip_target;
} BondSetting;

typedef struct BridgeSetting {
    char *interface_name;
    bool stp;
    unsigned int priority;
    unsigned int forward_delay;
    unsigned int hello_time;
    unsigned int max_age;
    unsigned int ageing_time;
} BridgeSetting;

typedef struct BridgeSlaveSetting {
    unsigned int priority;
    unsigned int path_cost;
    bool hairpin_mode;
} BridgeSlaveSetting;

BridgeSetting *setting_get_bridge_setting(Setting *setting);
BridgeSlaveSetting *setting_get_bridge_slave_setting(Setting *setting);
BondSetting *setting_get_bond_setting(Setting *setting);

void setting_free(Setting *setting);

LIST_DECL(Setting, setting)
Setting *settings_find_by_type(const Settings *settings, SettingType type);

#endif
