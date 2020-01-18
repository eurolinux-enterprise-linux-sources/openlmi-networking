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

#include "setting_nm.h"

#include <string.h>

#include "connection.h"

#include "dbus_wrapper.h"
#include "nm_support.h"
#include "setting_private.h"
#include "connection_private.h"

/* Must have same order as SettingType in globals.h */
const char *setting_type_strings[] = {
    "ipv4",
    "ipv6",
    "wired",
    "bond",
    "bridge",
    "bridge-port",
    NULL
};

static struct BondModeName {
    BondMode mode;
    char *name;
} bond_mode_name[] = {
    { BOND_MODE_BALANCERR, "balance-rr" },
    { BOND_MODE_ACTIVEBACKUP, "active-backup" },
    { BOND_MODE_BALANCEXOR, "balance-xor" },
    { BOND_MODE_BROADCAST, "broadcast" },
    { BOND_MODE_8023AD, "802.3ad" },
    { BOND_MODE_BALANCETLB, "balance-tlb" },
    { BOND_MODE_BALANCEALB, "balance-alb" }
};
size_t bond_mode_name_length = sizeof(bond_mode_name) / sizeof(bond_mode_name[0]);

GHashTable *setting_to_hash(Setting *setting, char **key, LMIResult *res)
{
    GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify) g_value_free);
    if (hash == NULL) {
        *res = LMI_ERROR_MEMORY;
        return NULL;
    }

    if (setting->type == SETTING_TYPE_UNKNOWN) {
        return hash;
    }
    *key = strdup(setting_type_strings[setting->type]);
    if (*key == NULL) {
        *res = LMI_ERROR_MEMORY;
        error("Memory allocation failed");
        g_hash_table_unref(hash);
        return NULL;
    }
    size_t i, j;
    switch (setting->type) {
        case SETTING_TYPE_IPv4:
        case SETTING_TYPE_IPv6:
            switch (setting->typespec.ip.method) {
                case SETTING_METHOD_DHCP:
                case SETTING_METHOD_STATELESS:
                    if ((*res = g_hash_table_insert_string_value(hash, "method", "auto")) != LMI_SUCCESS) {
                        goto err;
                    }
                    break;
                case SETTING_METHOD_DHCPv6:
                    if ((*res = g_hash_table_insert_string_value(hash, "method", "dhcp")) != LMI_SUCCESS) {
                        goto err;
                    }
                    break;
                case SETTING_METHOD_LINK_LOCAL:
                    if ((*res = g_hash_table_insert_string_value(hash, "method", "link-local")) != LMI_SUCCESS) {
                        goto err;
                    }
                    break;
                case SETTING_METHOD_DISABLED:
                    if (setting->type == SETTING_TYPE_IPv4) {
                        if ((*res = g_hash_table_insert_string_value(hash, "method", "disabled")) != LMI_SUCCESS) {
                            goto err;
                        }
                    } else {
                        if ((*res = g_hash_table_insert_string_value(hash, "method", "ignore")) != LMI_SUCCESS) {
                            goto err;
                        }
                    }
                    break;
                case SETTING_METHOD_STATIC:
                    if ((*res = g_hash_table_insert_string_value(hash, "method", "manual")) != LMI_SUCCESS) {
                        goto err;
                    }
                    if (setting->type == SETTING_TYPE_IPv4) {
                        GPtrArray *ip4addresses = g_ptr_array_new_with_free_func((GDestroyNotify) g_array_free);
                        if (ip4addresses == NULL) {
                            *res = LMI_ERROR_MEMORY;
                            goto err;
                        }
                        GArray *arr;
                        for (i = 0; i < addresses_length(setting->typespec.ip.addresses); ++i) {
                            arr = address_to_ipv4_array(addresses_index(setting->typespec.ip.addresses, i));
                            if (arr == NULL) {
                                *res = LMI_ERROR_MEMORY;
                                g_ptr_array_unref(ip4addresses);
                                goto err;
                            }
                            g_ptr_array_add(ip4addresses, g_array_ref(arr));
                        }
                        if ((*res = g_hash_table_insert_boxed(hash, "addresses", NM_TYPE_IP4_ADDRESSES, ip4addresses, true)) != LMI_SUCCESS) {
                            g_ptr_array_unref(ip4addresses);
                            goto err;
                        }

                        if (setting->typespec.ip.dns_servers != NULL) {
                            DNSServer *dns_server;
                            GArray *dns = g_array_new(true, true, sizeof(guint32));
                            if (dns == NULL) {
                                *res = LMI_ERROR_MEMORY;
                                goto err;
                            }
                            for (j = 0; j < dns_servers_length(setting->typespec.ip.dns_servers); ++j) {
                                dns_server = dns_servers_index(setting->typespec.ip.dns_servers, j);
                                if (dns_server->type == IPv4) {
                                    guint32 ip = ip4FromString(dns_server->server);
                                    g_array_append_val(dns, ip);
                                }
                            }
                            if ((*res = g_hash_table_insert_boxed(hash, "dns", NM_TYPE_DNS4_SERVERS, dns, true)) != LMI_SUCCESS) {
                                g_array_free(dns, true);
                                goto err;
                            }

                        }
                        if (setting->typespec.ip.routes != NULL) {
                            GPtrArray *routes = g_ptr_array_new_with_free_func((GDestroyNotify) g_array_free);
                            if (routes == NULL) {
                                *res = LMI_ERROR_MEMORY;
                                goto err;
                            }
                            GArray *r;
                            guint32 ip;
                            for (j = 0; j < routes_length(setting->typespec.ip.routes); ++j) {
                                Route *route = routes_index(setting->typespec.ip.routes, j);
                                r = g_array_new(true, true, sizeof(guint32));
                                ip = ip4FromString(route->route);
                                g_array_append_val(r, ip);
                                g_array_append_val(r, route->prefix);
                                ip = ip4FromString(route->next_hop);
                                g_array_append_val(r, ip);
                                g_array_append_val(r, route->metric);
                                g_array_ref(r);
                                g_ptr_array_add(routes, r);
                            }
                            if ((*res = g_hash_table_insert_boxed(hash, "routes", NM_TYPE_IP4_ROUTES, routes, true)) != LMI_SUCCESS) {
                                g_ptr_array_free(routes, true);
                                goto err;
                            }
                        }
                    } else {
                        GPtrArray *ip6addresses = g_ptr_array_new_with_free_func((GDestroyNotify) g_value_array_free);
                        if (ip6addresses == NULL) {
                            *res = LMI_ERROR_MEMORY;
                            goto err;
                        }
                        GValueArray *arr;
                        for (i = 0; i < addresses_length(setting->typespec.ip.addresses); ++i) {
                            arr = address_to_ipv6_array(addresses_index(setting->typespec.ip.addresses, i));
                            if (arr == NULL) {
                                g_ptr_array_free(ip6addresses, true);
                                *res = LMI_ERROR_MEMORY;
                                goto err;
                            }
                            g_ptr_array_add(ip6addresses, arr);
                        }
                        if ((*res = g_hash_table_insert_boxed(hash, "addresses", NM_TYPE_IP6_ADDRESSES, ip6addresses, false)) != LMI_SUCCESS) {
                            g_ptr_array_free(ip6addresses, true);
                            goto err;
                        }

                        if (setting->typespec.ip.dns_servers != NULL) {
                            GPtrArray *dns = g_ptr_array_new_with_free_func((GDestroyNotify) g_byte_array_free);
                            if (dns == NULL) {
                                *res = LMI_ERROR_MEMORY;
                                goto err;
                            }
                            DNSServer *dns_server;
                            for (j = 0; j < dns_servers_length(setting->typespec.ip.dns_servers); ++j) {
                                dns_server = dns_servers_index(setting->typespec.ip.dns_servers, j);
                                if (dns_server->type == IPv6) {
                                    GByteArray *ip = ip6ArrayFromString(dns_server->server);
                                    if (ip == NULL) {
                                        *res = LMI_ERROR_MEMORY;
                                        g_ptr_array_free(dns, TRUE);
                                        goto err;
                                    }
                                    g_byte_array_ref(ip);
                                    g_ptr_array_add(dns, ip);
                                }
                            }
                            if ((*res = g_hash_table_insert_boxed(hash, "dns", NM_TYPE_DNS6_SERVERS, dns, true)) != LMI_SUCCESS) {
                                g_ptr_array_free(dns, true);
                                goto err;
                            }
                        }
                        if (setting->typespec.ip.routes != NULL) {
                            GPtrArray *routes = g_ptr_array_new_with_free_func((GDestroyNotify) g_value_array_free);
                            if (routes == NULL) {
                                *res = LMI_ERROR_MEMORY;
                                goto err;
                            }
                            GValueArray *array;
                            for (j = 0; j < routes_length(setting->typespec.ip.routes); ++j) {
                                Route *route = routes_index(setting->typespec.ip.routes, j);
                                array = g_value_array_new(4);
                                if (array == NULL) {
                                    g_ptr_array_free(routes, true);
                                    *res = LMI_ERROR_MEMORY;
                                    goto err;
                                }
                                GValue v = G_VALUE_INIT;

                                // route
                                g_value_init(&v, DBUS_TYPE_G_UCHAR_ARRAY);
                                g_value_take_boxed(&v, ip6ArrayFromString(route->route));
                                g_value_array_append(array, &v);
                                g_value_unset(&v);
                                // prefix
                                g_value_init(&v, G_TYPE_UINT);
                                g_value_set_uint(&v, route->prefix);
                                g_value_array_append(array, &v);
                                g_value_unset(&v);
                                // next_hop
                                g_value_init(&v, DBUS_TYPE_G_UCHAR_ARRAY);
                                g_value_take_boxed(&v, ip6ArrayFromString(route->next_hop));
                                g_value_array_append(array, &v);
                                g_value_unset(&v);
                                // metric
                                g_value_init(&v, G_TYPE_UINT);
                                g_value_set_uint(&v, route->metric);
                                g_value_array_append(array, &v);
                                g_value_unset(&v);

                                g_ptr_array_add(routes, array);
                            }
                            if ((*res = g_hash_table_insert_boxed(hash, "routes", NM_TYPE_IP6_ROUTES, routes, false)) != LMI_SUCCESS) {
                                g_ptr_array_free(routes, true);
                                goto err;
                            }
                        }
                    }
                    break;
                case SETTING_METHOD_UNKNOWN:
                    error("Cannot convert UNKNOWN setting to hash");
                    *res = LMI_ERROR_BACKEND;
                    return NULL;
            }
            if (setting->typespec.ip.search_domains != NULL && search_domains_length(setting->typespec.ip.search_domains) > 0) {
                size_t len = search_domains_length(setting->typespec.ip.search_domains);
                char **search_domains = malloc((len + 1) * sizeof(char *));
                if (search_domains == NULL) {
                    error("Memory allocation failed");
                    *res = LMI_ERROR_MEMORY;
                    goto err;
                }
                for (size_t j = 0; j < len; ++j) {
                    search_domains[j] = strdup(search_domains_index(setting->typespec.ip.search_domains, j)->domain);
                    if (search_domains[j] == NULL) {
                        error("Memory allocation failed");
                        *res = LMI_ERROR_MEMORY;
                        free(search_domains);
                        goto err;
                    }
                }
                search_domains[len] = NULL;
                *res = g_hash_table_insert_boxed(hash, "dns-search", NM_TYPE_SEARCH_DOMAINS, search_domains, false);
                for (size_t j = 0; j < len; ++j) {
                    free(search_domains[j]);
                }
                free(search_domains);
                if (*res != LMI_SUCCESS) {
                    goto err;
                }
            }
            break;
        case SETTING_TYPE_WIRED:
            break;
        case SETTING_TYPE_BOND:
            if (setting->typespec.bond.interface_name) {
                if ((*res = g_hash_table_insert_string_value(hash, "interface-name", setting->typespec.bond.interface_name)) != LMI_SUCCESS) {
                    goto err;
                }
            }
            GHashTable *option_hash = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
            if ((*res = g_hash_table_insert_string(option_hash, "mode", bond_mode_name[setting->typespec.bond.mode].name)) != LMI_SUCCESS) {
                g_hash_table_unref(option_hash);
                goto err;
            }
            char s[15];
            snprintf(s, 14, "%ld", setting->typespec.bond.miimon);
            g_hash_table_insert_string(option_hash, "miimon", s);

            if (setting->typespec.bond.downdelay != 0) {
                snprintf(s, 14, "%ld", setting->typespec.bond.downdelay);
                g_hash_table_insert_string(option_hash, "downdelay", s);
            }

            if (setting->typespec.bond.updelay != 0) {
                snprintf(s, 14, "%ld", setting->typespec.bond.updelay);
                g_hash_table_insert_string(option_hash, "updelay", s);
            }
            if (setting->typespec.bond.arp_interval != 0) {
                snprintf(s, 14, "%ld", setting->typespec.bond.arp_interval);
                g_hash_table_insert_string(option_hash, "arp_interval", s);
            }

            size_t len = ip_addresses_length(setting->typespec.bond.arp_ip_target), size = 0, i;
            if (len > 0) {
                for (i = 0; i < len; ++i) {
                    size += strlen(ip_addresses_index(setting->typespec.bond.arp_ip_target, i) + 1);
                }
                char *addresses, *p;
                addresses = p = malloc(size * sizeof(char));
                char *address;
                for (i = 0; i < len; ++i) {
                    address = ip_addresses_index(setting->typespec.bond.arp_ip_target, i);
                    strcpy(p, address);
                    p += strlen(address);
                    if (i < len - 1) {
                        p[0] = ',';
                    } else {
                        p[0] = '\0';
                    }
                    p++;
                }
                char *k;
                if ((k = strdup("arp_ip_target")) == NULL) {
                    *res = LMI_ERROR_MEMORY;
                    g_hash_table_unref(option_hash);
                    free(addresses);
                    goto err;
                }
                g_hash_table_insert(option_hash, k, addresses);
            }

            if ((*res = g_hash_table_insert_boxed(hash, "options", DBUS_TYPE_G_MAP_OF_STRING, option_hash, true)) != LMI_SUCCESS) {
                g_hash_table_unref(option_hash);
                goto err;
            }
            break;
        case SETTING_TYPE_BRIDGE:
            if (setting->typespec.bridge.interface_name) {
                if ((*res = g_hash_table_insert_string_value(hash, "interface-name", setting->typespec.bridge.interface_name)) != LMI_SUCCESS) {
                    goto err;
                }
            }

            if ((*res = g_hash_table_insert_bool(hash, "stp", setting->typespec.bridge.stp)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "priority", setting->typespec.bridge.priority)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "forward-delay", setting->typespec.bridge.forward_delay)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "hello-time", setting->typespec.bridge.hello_time)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "max-age", setting->typespec.bridge.max_age)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "ageing-time", setting->typespec.bridge.ageing_time)) != LMI_SUCCESS) {
                goto err;
            }
            break;
        case SETTING_TYPE_BRIDGE_SLAVE:
            if ((*res = g_hash_table_insert_uint(hash, "priority", setting->typespec.bridge_slave.priority)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "path-cost", setting->typespec.bridge_slave.path_cost)) != LMI_SUCCESS) {
                goto err;
            }
            if ((*res = g_hash_table_insert_uint(hash, "hairpin-mode", setting->typespec.bridge_slave.hairpin_mode)) != LMI_SUCCESS) {
                goto err;
            }
            break;
        default:
            error("Can't create setting with unknown type");
            *res = LMI_ERROR_BACKEND;
            goto err;
    }
    return hash;
err:
    g_hash_table_unref(hash);
    return NULL;
}

GValue *g_hash_table_lookup_check(GHashTable *hash, const char *key, GType type)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        return NULL;
    }
    if (!G_VALUE_HOLDS(v, type)) {
        error("Wrong type for key \"%s\": %s", key, G_VALUE_TYPE_NAME(v));
        return NULL;
    }
    return v;
}

Setting *setting_from_hash(GHashTable *hash, const char *key, LMIResult *res)
{
    int setting_type = find_index(key, setting_type_strings);
    if (setting_type == -1) {
        error("Unknown setting type: %s", key);
    }
    Setting *setting = setting_new(setting_type);
    if (setting == NULL) {
        *res = LMI_ERROR_MEMORY;
        return NULL;
    }
    const char *str;
    GValue *value;
    switch (setting->type) {
        case SETTING_TYPE_IPv4:
        case SETTING_TYPE_IPv6:
            value = g_hash_table_lookup_check(hash, "method", G_TYPE_STRING);
            if (value) {
                str = g_value_get_string(value);
                if (str) {
                    if (strcmp(str, "auto") == 0) {
                        if (setting->type == SETTING_TYPE_IPv4) {
                            setting->typespec.ip.method = SETTING_METHOD_DHCP;
                        } else {
                            setting->typespec.ip.method = SETTING_METHOD_STATELESS;
                        }
                    } else if (strcmp(str, "dhcp") == 0) {
                        // DHCP option is only valid for IPv6
                        setting->typespec.ip.method = SETTING_METHOD_DHCPv6;
                    } else if (strcmp(str, "link-local") == 0) {
                        setting->typespec.ip.method = SETTING_METHOD_LINK_LOCAL;
                    } else if (strcmp(str, "manual") == 0) {
                        setting->typespec.ip.method = SETTING_METHOD_STATIC;
                    } else if ((strcmp(str, "ignore") == 0) || (strcmp(str, "disabled") == 0)) {
                        setting->typespec.ip.method = SETTING_METHOD_DISABLED;
                        return setting;
                    } else {
                        warn("Unknown method: %s", str);
                    }
                }
            }
            if (setting->type == SETTING_TYPE_IPv4) {
                value = g_hash_table_lookup_check(hash, "addresses", NM_TYPE_IP4_ADDRESSES);
            } else {
                value = g_hash_table_lookup_check(hash, "addresses", NM_TYPE_IP6_ADDRESSES);
            }
            if (value) {
                GPtrArray *addresses = g_value_get_boxed(value);
                Address *address;
                for (guint i = 0; i < addresses->len; ++i) {
                    if (setting->type == SETTING_TYPE_IPv4) {
                        address = ipv4_array_to_address(g_ptr_array_index(addresses, i));
                    } else {
                        address = ipv6_array_to_address(g_ptr_array_index(addresses, i));
                    }
                    if (address == NULL || addresses_add(setting->typespec.ip.addresses, address) != LMI_SUCCESS) {
                        *res = LMI_ERROR_MEMORY;
                        goto err;
                    }
                }
            }
            if (setting->type == SETTING_TYPE_IPv4) {
                value = g_hash_table_lookup_check(hash, "routes", NM_TYPE_IP4_ROUTES);
            } else {
                value = g_hash_table_lookup_check(hash, "routes", NM_TYPE_IP6_ROUTES);
            }
            if (value) {
                if (setting->type == SETTING_TYPE_IPv4) {
                    if ((*res = routes4_fill_from_gvalue(setting->typespec.ip.routes, value)) != LMI_SUCCESS) {
                        goto err;
                    }
                } else {
                    if ((*res = routes6_fill_from_gvalue(setting->typespec.ip.routes, value)) != LMI_SUCCESS) {
                        goto err;
                    }
                }
            }

            if (setting->type == SETTING_TYPE_IPv4) {
                value = g_hash_table_lookup_check(hash, "dns", NM_TYPE_DNS4_SERVERS);
            } else {
                value = g_hash_table_lookup_check(hash, "dns", NM_TYPE_DNS6_SERVERS);
            }
            if (value) {
                if (setting->type == SETTING_TYPE_IPv4) {
                    if ((*res = dns_servers4_fill_from_gvalue(setting->typespec.ip.dns_servers, value)) != LMI_SUCCESS) {
                        goto err;
                    }
                } else {
                    if ((*res = dns_servers6_fill_from_gvalue(setting->typespec.ip.dns_servers, value)) != LMI_SUCCESS) {
                        goto err;
                    }
                }
            }
            value = g_hash_table_lookup_check(hash, "dns-search", NM_TYPE_SEARCH_DOMAINS);
            if (value) {
                SearchDomain *domain;
                char **search_domain_ptr, **search_domains = g_value_get_boxed(value);
                for (search_domain_ptr = search_domains; *search_domain_ptr; search_domain_ptr++) {
                    domain = search_domain_new(setting->type == SETTING_TYPE_IPv4 ? IPv4 : IPv6, *search_domain_ptr);
                    if (domain == NULL) {
                        *res = LMI_ERROR_MEMORY;
                        goto err;
                    }
                    if ((*res = search_domains_add(setting->typespec.ip.search_domains, domain)) != LMI_SUCCESS) {
                        search_domain_free(domain);
                        goto err;
                    }
                }
            }

            break;
        case SETTING_TYPE_WIRED:
            str = g_hash_table_lookup(hash, "mac-address");
            if (str) {
                if ((setting->typespec.wired.mac = strdup(str)) == NULL) {
                    error("Memory allocation failed");
                    *res = LMI_ERROR_MEMORY;
                    goto err;
                }
            }
            break;
        case SETTING_TYPE_BOND:
            value = g_hash_table_lookup(hash, "interface-name");
            if (value) {
                if ((setting->typespec.bond.interface_name = strdup(g_value_get_string(value))) == NULL) {
                    *res = LMI_ERROR_MEMORY;
                    goto err;
                }
            }
            value = g_hash_table_lookup(hash, "options");
            if (value) {
                GHashTable *option_hash = g_value_get_boxed(value);
                char *subvalue = g_hash_table_lookup(option_hash, "mode");
                if (subvalue) {
                    for (size_t i = 0; i < bond_mode_name_length; ++i) {
                        if (strcmp(subvalue, bond_mode_name[i].name) == 0) {
                            setting->typespec.bond.mode = bond_mode_name[i].mode;
                        }
                    }
                } else {
                    setting->typespec.bond.mode = BOND_MODE_BALANCERR;
                }

                if ((subvalue = g_hash_table_lookup(option_hash, "miimon")) != NULL) {
                    sscanf(subvalue, "%lu", &setting->typespec.bond.miimon);
                }

                if ((subvalue = g_hash_table_lookup(option_hash, "downdelay")) != NULL) {
                    sscanf(subvalue, "%lu", &setting->typespec.bond.downdelay);
                }

                if ((subvalue = g_hash_table_lookup(option_hash, "updelay")) != NULL) {
                    sscanf(subvalue, "%lu", &setting->typespec.bond.updelay);
                }

                if ((subvalue = g_hash_table_lookup(option_hash, "arp_interval")) != NULL) {
                    sscanf(subvalue, "%lu", &setting->typespec.bond.arp_interval);
                }

                if ((subvalue = g_hash_table_lookup(option_hash, "arp_ip_target")) != NULL) {
                    if (setting->typespec.bond.arp_ip_target != NULL) {
                        ip_addresses_free(setting->typespec.bond.arp_ip_target, true);
                    }
                    setting->typespec.bond.arp_ip_target = ip_addresses_new(1);
                    char *token, *saveptr, *str;
                    for (str = subvalue; ; str = NULL) {
                        token = strtok_r(str, ",", &saveptr);
                        if (token == NULL) {
                            break;
                        }
                        ip_addresses_add(setting->typespec.bond.arp_ip_target, strdup(token));
                    }
                }
            }
            break;
        case SETTING_TYPE_BRIDGE:
            value = g_hash_table_lookup(hash, "interface-name");
            if (value) {
                if ((setting->typespec.bridge.interface_name = strdup(g_value_get_string(value))) == NULL) {
                    *res = LMI_ERROR_MEMORY;
                    goto err;
                }
            }

            value = g_hash_table_lookup(hash, "stp");
            if (value) {
                setting->typespec.bridge.stp = g_value_get_boolean(value);
            } else {
                setting->typespec.bridge.stp = true;
            }

            value = g_hash_table_lookup(hash, "priority");
            if (value) {
                setting->typespec.bridge.priority = g_value_get_uint(value);
            } else {
                setting->typespec.bridge.priority = 128;
            }

            value = g_hash_table_lookup(hash, "forward-delay");
            if (value) {
                setting->typespec.bridge.forward_delay = g_value_get_uint(value);
            } else {
                setting->typespec.bridge.forward_delay = 15;
            }

            value = g_hash_table_lookup(hash, "hello-time");
            if (value) {
                setting->typespec.bridge.hello_time = g_value_get_uint(value);
            } else {
                setting->typespec.bridge.hello_time = 2;
            }

            value = g_hash_table_lookup(hash, "max-age");
            if (value) {
                setting->typespec.bridge.max_age = g_value_get_uint(value);
            } else {
                setting->typespec.bridge.max_age = 20;
            }

            value = g_hash_table_lookup(hash, "ageing-time");
            if (value) {
                setting->typespec.bridge.ageing_time = g_value_get_uint(value);
            } else {
                setting->typespec.bridge.ageing_time = 300;
            }
            break;
        case SETTING_TYPE_BRIDGE_SLAVE:
            value = g_hash_table_lookup(hash, "priority");
            if (value) {
                setting->typespec.bridge_slave.priority = g_value_get_uint(value);
            } else {
                setting->typespec.bridge_slave.priority = 32;
            }
            value = g_hash_table_lookup(hash, "path-cost");
            if (value) {
                setting->typespec.bridge_slave.path_cost = g_value_get_uint(value);
            } else {
                setting->typespec.bridge_slave.path_cost = 100;
            }
            value = g_hash_table_lookup(hash, "hairpin-mode");
            if (value) {
                setting->typespec.bridge_slave.hairpin_mode = g_value_get_boolean(value);
            } else {
                setting->typespec.bridge_slave.hairpin_mode = false;
            }
            break;
        case SETTING_TYPE_UNKNOWN:
            warn("Setting type %s is not supported, ignoring", key);
            break;
    }
    return setting;
err:
    setting_free(setting);
    return NULL;
}
