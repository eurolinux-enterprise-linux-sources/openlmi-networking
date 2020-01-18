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

#include "nm_support.h"

#include <netinet/in.h>
#include "dbus_wrapper.h"

#include <stdbool.h>
#include <dbus/dbus-glib.h>
#include <string.h>

GArray *address_to_ipv4_array(Address *address)
{
    GArray *tmp = g_array_sized_new(TRUE, TRUE, sizeof(guint32), 3);
    if (tmp == NULL) {
        return NULL;
    }
    guint32 i = ip4FromString(address->addr);
    g_array_append_val(tmp, i);
    i = address->prefix;
    g_array_append_val(tmp, i);
    i = address->default_gateway == NULL ? 0 : ip4FromString(address->default_gateway);
    g_array_append_val(tmp, i);
    return tmp;
}

Address *ipv4_array_to_address(GArray *array)
{
    if (array->len <= 2) {
        return NULL;
    }
    Address *address = address_new(IPv4);
    if (address == NULL) {
        return NULL;
    }
    guint32 i = g_array_index(array, guint32, 0);
    address->addr = ip4ToString(i);
    if (address->addr == NULL) {
        address_free(address);
        return NULL;
    }
    i = g_array_index(array, guint32, 1);
    address->prefix = i;
    if (array->len > 2) {
        i = g_array_index(array, guint32, 2);
        if (i == 0) {
            address->default_gateway = NULL;
        } else {
            if ((address->default_gateway = ip4ToString(i)) == NULL) {
                address_free(address);
                return NULL;
            }
        }
    } else {
        address->default_gateway = NULL;
    }
    return address;
}

GValueArray *address_to_ipv6_array(Address *address)
{
    GByteArray *array;
    GValueArray *tmp = g_value_array_new(3);
    if (tmp == NULL) {
        return NULL;
    }
    GValue element = G_VALUE_INIT;

    g_value_init(&element, IPv6_ADDR);
    if ((array = ip6ArrayFromString(address->addr)) == NULL) {
        g_value_array_free(tmp);
        return NULL;
    }
    g_value_take_boxed(&element, array);
    g_value_array_append(tmp, &element);
    g_value_unset(&element);

    g_value_init(&element, G_TYPE_UINT);
    g_value_set_uint(&element, address->prefix);
    g_value_array_append(tmp, &element);
    g_value_unset(&element);

    g_value_init(&element, IPv6_ADDR);
    if (address->default_gateway != NULL) {
        array = ip6ArrayFromString(address->default_gateway);
    } else {
        array = ip6ArrayFromString("::");
    }
    if (array == NULL) {
        g_value_array_free(tmp);
        return NULL;
    }

    g_value_take_boxed(&element, array);

    g_value_array_append(tmp, &element);
    g_value_unset(&element);
    return tmp;
}

Address *ipv6_array_to_address(GValueArray *array)
{
    if (array->n_values <= 2) {
        return NULL;
    }
    Address *address = address_new(IPv6);
    if (address == NULL) {
        return NULL;
    }
    GValue *v = g_value_array_get_nth(array, 0);
    address->addr = ip6ArrayToString(g_value_get_boxed(v));
    v = g_value_array_get_nth(array, 1);
    address->prefix = g_value_get_uint(v);
    if (array->n_values > 2) {
        v = g_value_array_get_nth(array, 2);
        address->default_gateway = ip6ArrayToString(g_value_get_boxed(v));
    } else {
        if ((address->default_gateway = strdup("::")) == NULL) {
            error("Memory allocation failed");
            address_free(address);
            return NULL;
        }
    }
    return address;
}

LMIResult dns_servers4_fill_from_gvalue(DNSServers *dns_servers, GValue *v)
{
    LMIResult res;
    GArray *array;
    char *ipv4;
    DNSServer *dns;
    if (v != NULL && G_VALUE_HOLDS_BOXED(v)) {
        array = g_value_get_boxed(v);
        if (array) {
            for (guint i = 0; i < array->len; ++i) {
                if ((ipv4 = ip4ToString(g_array_index(array, guint32, i))) == NULL) {
                    return LMI_ERROR_MEMORY;
                }
                if ((dns = dns_server_new(IPv4, ipv4)) == NULL) {
                    free(ipv4);
                    return LMI_ERROR_MEMORY;
                }
                if ((res = dns_servers_add(dns_servers, dns)) != LMI_SUCCESS) {
                    dns_server_free(dns);
                    free(ipv4);
                    return res;
                }
                free(ipv4);
            }
        } else {
            error("Unable to read DNS servers");
            return LMI_ERROR_BACKEND;
        }
    } else {
        error("Unable to read DNS servers");
        return LMI_ERROR_BACKEND;
    }
    return LMI_SUCCESS;
}

LMIResult dns_servers6_fill_from_gvalue(DNSServers *dns_servers, GValue *v)
{
    LMIResult res;
    DNSServer *dns;
    GPtrArray *array;
    char *ipv6;
    if (v != NULL && G_VALUE_HOLDS_BOXED(v)) {
        array = g_value_get_boxed(v);
        if (array) {
            for (guint i = 0; i < array->len; ++i) {
                if ((ipv6 = ip6ArrayToString(g_ptr_array_index(array, i))) == NULL) {
                    return LMI_ERROR_MEMORY;
                }
                if ((dns = dns_server_new(IPv6, ipv6)) == NULL) {
                    free(ipv6);
                    return LMI_ERROR_MEMORY;
                }
                if ((res = dns_servers_add(dns_servers, dns)) != LMI_SUCCESS) {
                    dns_server_free(dns);
                    free(ipv6);
                    return res;
                }
                free(ipv6);
            }
        } else {
            error("Unable to read DNS servers");
            return LMI_ERROR_BACKEND;
        }
    } else {
        error("Unable to read DNS servers");
        return LMI_ERROR_BACKEND;
    }
    return LMI_SUCCESS;
}

LMIResult routes4_fill_from_gvalue(Routes *routes, GValue *v)
{
    LMIResult res = LMI_SUCCESS;
    GPtrArray *array;
    GArray *subarray;
    Route *route = NULL;
    if (v != NULL && G_VALUE_HOLDS_BOXED(v)) {
        array = g_value_get_boxed(v);
        if (array) {
            for (guint i = 0; i < array->len; ++i) {
                subarray = (GArray *) array->pdata[i];
                if ((route = route_new(IPv4)) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                if (subarray->len < 4) {
                    error("Unable to read IPv4 routes, not enough values");
                    res = LMI_ERROR_BACKEND;
                    goto err;
                }
                if ((route->route = ip4ToString(g_array_index(subarray, guint32, 0))) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                route->prefix = g_array_index(subarray, guint32, 1);
                if ((route->next_hop = ip4ToString(g_array_index(subarray, guint32, 2))) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                route->metric = g_array_index(subarray, guint32, 3);
                if ((res = routes_add(routes, route)) != LMI_SUCCESS) {
                    goto err;
                }
            }
        } else {
            error("Unable to read IPv4 routes");
            res = LMI_ERROR_BACKEND;
            goto err;
        }
    } else {
        error("Unable to read IPv4 routes");
        res = LMI_ERROR_BACKEND;
        goto err;
    }
    return LMI_SUCCESS;
err:
    route_free(route);
    return res;
}

LMIResult routes6_fill_from_gvalue(Routes *routes, GValue *v)
{
    LMIResult res = LMI_SUCCESS;
    GPtrArray *array;
    GValueArray *subarray;
    Route *route = NULL;
    if (v != NULL && G_VALUE_HOLDS_BOXED(v)) {
        array = g_value_get_boxed(v);
        if (array) {
            for (guint i = 0; i < array->len; ++i) {
                subarray = g_ptr_array_index(array, i);
                if ((route = route_new(IPv6)) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                if (subarray->n_values < 4) {
                    error("Unable to read IPv6 routes, not enough values");
                    res = LMI_ERROR_BACKEND;
                    goto err;
                }
                if ((route->route = ip6ArrayToString(g_value_get_boxed(g_value_array_get_nth(subarray, 0)))) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                route->prefix = g_value_get_uint(g_value_array_get_nth(subarray, 1));
                if ((route->next_hop = ip6ArrayToString(g_value_get_boxed(g_value_array_get_nth(subarray, 2)))) == NULL) {
                    res = LMI_ERROR_MEMORY;
                    goto err;
                }
                route->metric = g_value_get_uint(g_value_array_get_nth(subarray, 3));
                if ((res = routes_add(routes, route)) != LMI_SUCCESS) {
                    goto err;
                }
            }
        } else {
            error("Unable to read IPv6 routes");
            res = LMI_ERROR_BACKEND;
            goto err;
        }
    } else {
        error("Unable to read IPv6 routes");
        res = LMI_ERROR_BACKEND;
        goto err;
    }
    return LMI_SUCCESS;
err:
    route_free(route);
    return res;
}

static void indent(int level)
{
    for (int i = 0; i < level; i++) {
        fprintf(stderr, "\t");
    }
}

void print_hash_table(GHashTable *hash, int level)
{
    if (!lmi_read_config_boolean("Log", "Stderr")) {
        return;
    }
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, hash);
    gpointer key, value;
    GValue *v;
    indent(level);
    fprintf(stderr, "[\n");
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        indent(level + 1);
        fprintf(stderr, "%s: ", (char *) key);
        if (level == 0) {
            // always hashtable on level 0
            print_hash_table(value, level + 1);
        } else if (value == NULL) {
            fprintf(stderr, "(null)\n");
        } else if (G_IS_VALUE(value)) {
            v = value;
            if (G_VALUE_HOLDS_STRING(v)) {
                fprintf(stderr, "\"%s\"\n", g_value_get_string(v));
            } else if (G_VALUE_HOLDS_INT(v)) {
                fprintf(stderr, "%d\n", g_value_get_int(v));
            } else if (G_VALUE_HOLDS_UINT(v)) {
                fprintf(stderr, "%u\n", g_value_get_uint(v));
            } else if (G_VALUE_HOLDS_BOOLEAN(v)) {
                fprintf(stderr, "%s\n", g_value_get_boolean(v) ? "true" : "false");
            } else if (G_VALUE_HOLDS(v, G_TYPE_HASH_TABLE)) {
                print_hash_table(value, level + 1);
            } else if (G_VALUE_HOLDS(v, DBUS_TYPE_G_UCHAR_ARRAY)) {
                GByteArray *a = g_value_get_boxed(v);
                if (a == NULL) {
                    fprintf(stderr, "(null)\n");
                    continue;
                }
                fprintf(stderr, "\"");
                for (guint i = 0; i < a->len; ++i) {
                    fprintf(stderr, "%.2X", a->data[i]);
                    if (i < a->len - 1) {
                        fprintf(stderr, ":");
                    }
                }
                fprintf(stderr, "\"\n");
            } else if (G_VALUE_HOLDS(v, NM_TYPE_SEARCH_DOMAINS)) {
                fprintf(stderr, "[");
                char **search_domain_ptr, **search_domains = g_value_get_boxed(value);
                for (search_domain_ptr = search_domains; *search_domain_ptr; search_domain_ptr++) {
                    fprintf(stderr, "%s, ", *search_domain_ptr);
                }
                fprintf(stderr, "]\n");
            } else if (G_VALUE_HOLDS(v, NM_TYPE_IP4_ADDRESSES)) {
                GPtrArray *a1 = g_value_get_boxed(v);
                GArray *a2;
                fprintf(stderr, "[");
                for (guint i = 0; i < a1->len; ++i) {
                    a2 = g_ptr_array_index(a1, i);
                    fprintf(stderr, "[");
                    for (guint j = 0; j < a2->len; ++j) {
                        fprintf(stderr, "%u, ", g_array_index(a2, guint32, j));
                    }
                    fprintf(stderr, "]");
                }
                fprintf(stderr, "]\n");
            } else if (G_VALUE_HOLDS(v, NM_TYPE_IP6_ADDRESSES)) {
                GPtrArray *a1 = g_value_get_boxed(v);
                GValueArray *a2;
                GValue *v;
                GByteArray *a3;
                for (guint i = 0; i < a1->len; ++i) {
                    fprintf(stderr, "[");
                    a2 = g_ptr_array_index(a1, i);
                    v = g_value_array_get_nth(a2, 0);
                    a3 = g_value_get_boxed(v);
                    for (guint i = 0; i < a3->len; ++i) {
                        fprintf(stderr, "%.2X", a3->data[i]);
                        if (i % 2 == 1) {
                            fprintf(stderr, ":");
                        }
                    }
                    v = g_value_array_get_nth(a2, 1);
                    fprintf(stderr, ", %u, ", g_value_get_uint(v));
                    if (a2->n_values > 2) {
                        v = g_value_array_get_nth(a2, 2);
                        a3 = g_value_get_boxed(v);
                        for (guint i = 0; i < a3->len; ++i) {
                            fprintf(stderr, "%.2X", a3->data[i]);
                            if (i % 2 == 1) {
                                fprintf(stderr, ":");
                            }
                        }
                    }
                    fprintf(stderr, "],");
                }
                fprintf(stderr, "\n");
            } else if (G_VALUE_HOLDS(v, DBUS_TYPE_G_MAP_OF_STRING)) {
                GHashTable *subhash = g_value_get_boxed(v);
                GHashTableIter subiter;
                g_hash_table_iter_init(&subiter, subhash);
                gpointer subkey, subvalue;
                while (g_hash_table_iter_next(&subiter, &subkey, &subvalue)) {
                    fprintf(stderr, "%s=\"%s\",", (char *) subkey, (char *) subvalue);
                }
                fprintf(stderr, "\n");
            } else {
                fprintf(stderr, "<unknown type: %s>\n", G_VALUE_TYPE_NAME(v));
            }
        } else {
            fprintf(stderr, "Value is not GValue\n");
        }
    }
    indent(level);
    fprintf(stderr, "]\n");
}

void g_hash_table_print(GHashTable *hash)
{
    print_hash_table(hash, 0);
}
