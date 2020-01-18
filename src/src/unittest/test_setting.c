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

#include "test_setting.h"
#include "setting.h"
#include "setting_nm.h"
#include "setting_private.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "dbus_wrapper.h"
#include "nm_support.h"

void compare_settings(Setting *s1, Setting *s2)
{
    ck_assert_int_eq(s1->type, s2->type);
    ck_assert_int_eq(s1->typespec.ip.method, s2->typespec.ip.method);
    ck_assert_int_eq(addresses_length(s1->typespec.ip.addresses), addresses_length(s2->typespec.ip.addresses));
    Address *a1, *a2;
    size_t i;
    for (i = 0; i < addresses_length(s1->typespec.ip.addresses); ++i) {
        a1 = addresses_index(s1->typespec.ip.addresses, i);
        a2 = addresses_index(s2->typespec.ip.addresses, i);
        assert_string_eq(a1->addr, a2->addr);
        ck_assert_int_eq(a1->prefix, a2->prefix);
        assert_string_eq(a1->default_gateway, a2->default_gateway);
    }
    Route *r1, *r2;
    ck_assert_int_eq(routes_length(s1->typespec.ip.routes), routes_length(s2->typespec.ip.routes));
    for (i = 0; i < routes_length(s1->typespec.ip.routes); ++i) {
        r1 = routes_index(s1->typespec.ip.routes, i);
        r2 = routes_index(s2->typespec.ip.routes, i);
        assert_string_eq(r1->route, r2->route);
        ck_assert_int_eq(r1->prefix, r2->prefix);
        assert_string_eq(r1->next_hop, r2->next_hop);
        ck_assert_int_eq(r1->metric, r2->metric);
    }

    DNSServer *d1, *d2;
    ck_assert_int_eq(dns_servers_length(s1->typespec.ip.dns_servers), dns_servers_length(s2->typespec.ip.dns_servers));
    for (i = 0; i < dns_servers_length(s1->typespec.ip.dns_servers); ++i) {
        d1 = dns_servers_index(s1->typespec.ip.dns_servers, i);
        d2 = dns_servers_index(s2->typespec.ip.dns_servers, i);
        ck_assert_int_eq(d1->type, d2->type);
        assert_string_eq(d1->server, d2->server);
    }

    SearchDomain *sd1, *sd2;
    ck_assert_int_eq(search_domains_length(s1->typespec.ip.search_domains), search_domains_length(s2->typespec.ip.search_domains));
    for (i = 0; i < search_domains_length(s1->typespec.ip.search_domains); ++i) {
        sd1 = search_domains_index(s1->typespec.ip.search_domains, i);
        sd2 = search_domains_index(s2->typespec.ip.search_domains, i);
        ck_assert_int_eq(sd1->type, sd2->type);
        assert_string_eq(sd1->domain, sd2->domain);
    }
}

START_TEST(test_setting_hashing)
{
    // IPv4 static setting
    Setting *setting = setting_new(SETTING_TYPE_IPv4);
    setting->typespec.ip.method = SETTING_METHOD_STATIC;

    Address *address = address_new(IPv4);
    address->addr = strdup("192.168.1.100");
    address->prefix = 24;
    address->default_gateway = strdup("192.168.1.1");
    addresses_add(setting->typespec.ip.addresses, address);
    address = address_new(IPv4);
    address->addr = strdup("192.168.1.101");
    address->prefix = 16;
    addresses_add(setting->typespec.ip.addresses, address);

    Route *route = route_new(IPv4);
    route->route = strdup("192.168.2.1");
    route->prefix = 24;
    route->next_hop = strdup("192.168.200.1");
    route->metric = 10;
    routes_add(setting->typespec.ip.routes, route);
    route = route_new(IPv4);
    route->route = strdup("192.168.3.1");
    route->prefix = 24;
    route->next_hop = strdup("192.168.250.1");
    route->metric = 20;
    routes_add(setting->typespec.ip.routes, route);

    dns_servers_add(setting->typespec.ip.dns_servers, dns_server_new(IPv4, "192.168.1.1"));
    dns_servers_add(setting->typespec.ip.dns_servers, dns_server_new(IPv4, "192.168.1.2"));

    search_domains_add(setting->typespec.ip.search_domains, search_domain_new(IPv4, "search.com"));
    search_domains_add(setting->typespec.ip.search_domains, search_domain_new(IPv4, "search.org"));

    char *key;
    LMIResult res = LMI_SUCCESS;
    GHashTable *hash = setting_to_hash(setting, &key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    assert_string_eq(key, "ipv4");
    Setting *setting2 = setting_from_hash(hash, key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    compare_settings(setting, setting2);
    g_hash_table_unref(hash);
    setting_free(setting);
    setting_free(setting2);
    free(key);
    key = NULL;

    // IPv6 static setting
    setting = setting_new(SETTING_TYPE_IPv6);
    setting->typespec.ip.method = SETTING_METHOD_STATIC;

    address = address_new(IPv6);
    address->addr = strdup("2001:db8:0:2219:210:18ff:fe97:2222");
    address->prefix = 24;
    address->default_gateway = strdup("2001:db8:0:2219:210:18ff:fe97:1");
    addresses_add(setting->typespec.ip.addresses, address);
    address = address_new(IPv6);
    address->addr = strdup("2001:db8:0:2219:210:18ff:fe97:2223");
    address->prefix = 16;
    address->default_gateway = strdup("::");
    addresses_add(setting->typespec.ip.addresses, address);

    route = route_new(IPv6);
    route->route = strdup("2001:db8:0:2219:210:18ff:fe98:1");
    route->prefix = 24;
    route->next_hop = strdup("2001:db8:0:2219:210:18ff:fe97:2");
    route->metric = 10;
    routes_add(setting->typespec.ip.routes, route);
    route = route_new(IPv6);
    route->route = strdup("2001:db8:0:2219:210:18ff:fe98:5");
    route->prefix = 24;
    route->next_hop = strdup("2001:db8:0:2219:210:18ff:fe97:55");
    route->metric = 20;
    routes_add(setting->typespec.ip.routes, route);

    dns_servers_add(setting->typespec.ip.dns_servers, dns_server_new(IPv6, "2001:db8:0:2219:210:18ff:fe97:1000"));
    dns_servers_add(setting->typespec.ip.dns_servers, dns_server_new(IPv6, "2001:db8:0:2219:210:18ff:fe97:1001"));

    search_domains_add(setting->typespec.ip.search_domains, search_domain_new(IPv6, "search.com"));
    search_domains_add(setting->typespec.ip.search_domains, search_domain_new(IPv6, "search.org"));

    hash = setting_to_hash(setting, &key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    assert_string_eq(key, "ipv6");
    setting2 = setting_from_hash(hash, key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    free(key);
    key = NULL;
    GHashTable *t2 = setting_to_hash(setting2, &key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    compare_settings(setting, setting2);
    setting_free(setting);
    setting_free(setting2);
    g_hash_table_unref(hash);
    g_hash_table_unref(t2);

    // Test hashing unknown settings
    setting = setting_new(SETTING_TYPE_UNKNOWN);
    hash = setting_to_hash(setting, &key, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    setting_free(setting);
    g_hash_table_unref(hash);
    free(key);
}
END_TEST


TCase *setting_tcase(void)
{
    TCase *tc_setting = tcase_create("setting");
    tcase_add_test(tc_setting, test_setting_hashing);
    return tc_setting;
}
