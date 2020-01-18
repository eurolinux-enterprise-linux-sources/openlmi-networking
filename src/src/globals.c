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

#include "globals.h"

#include <string.h>
#include <unistd.h>

#include <uuid/uuid.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <cmpimacs.h>

GValue *gvalue_new_from_value(GType type, gconstpointer data)
{
    GValue *v = g_new0(GValue, 1);
    if (v == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    g_value_init(v, type);
    g_value_take_boxed(v, data);
    return v;
}

char *ip4ToString(uint32_t i)
{
    struct in_addr in = { i };
    char *ip = malloc(INET_ADDRSTRLEN);
    if (ip == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    if (inet_ntop(AF_INET, &in, ip, INET_ADDRSTRLEN) == NULL) {
        error("Unable to convert IPv4 address to string");
        free(ip);
        return NULL;
    }
    return ip;
}

uint32_t ip4FromString(const char *ip)
{
    if (ip == NULL) {
        error("Invalid argument (null) for ip4FromString");
        return 0;
    }
    struct in_addr inp;
    if (inet_pton(AF_INET, ip, &inp) <= 0) {
        warn("IPv4 address %s is not valid.", ip);
        return 0;
    }
    return inp.s_addr;
}

char *ip6ToString(const struct in6_addr *i)
{
    if (i == NULL) {
        error("Invalid argument (null) for ip6ToString");
        return NULL;
    }
    char *ip = malloc(INET6_ADDRSTRLEN);
    if (ip == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    if (inet_ntop(AF_INET6, i, ip, INET6_ADDRSTRLEN) == NULL) {
        error("Unable to convert IPv6 address to string");
        free(ip);
        return NULL;
    }
    return ip;

}

struct in6_addr *ip6FromString(const char *ip)
{
    if (ip == NULL) {
        error("Invalid argument (null) for ip6FromString");
        return NULL;
    }
    struct in6_addr *inp = malloc(sizeof(struct in6_addr));
    if (inp == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    if (inet_pton(AF_INET6, ip, inp) <= 0) {
        warn("IPv6 address %s is not valid.", ip);
        free(inp);
        return NULL;
    }
    return inp;
}

char *prefixToMask4(uint8_t prefix)
{
    return ip4ToString(((1 << prefix) - 1));
}

uint8_t netmaskToPrefix4(const char *netmask)
{
    if (netmask == NULL) {
        error("Invalid argument (null) for netmaskToPrefix4");
        return 0;
    }
    struct in_addr inp;
    if (inet_pton(AF_INET, netmask, &inp) <= 0) {
        warn("Invalid netmask: %s", netmask);
        return 0;
    }
    uint8_t ret = 0;
    while (inp.s_addr != 0) {
        ret += (inp.s_addr & 0x01);
        inp.s_addr >>= 1;
    }
    return ret;
}

char *macFromGByteArray(const GByteArray *mac)
{
    if (mac == NULL) {
        error("Invalid argument (null) for macFromGByteArray");
        return NULL;
    }
    char *m;
    if (asprintf(&m, "%02X:%02X:%02X:%02X:%02X:%02X",
             (int) mac->data[0], (int) mac->data[1],
             (int) mac->data[2], (int) mac->data[3],
             (int) mac->data[4], (int) mac->data[5]) < 0) {

        error("Memory allocation failed");
        return NULL;
    }
    return m;
}

GByteArray *macToGByteArray(const char *mac)
{
    if (mac == NULL) {
        error("Invalid argument (null) for macToGByteArray");
        return NULL;
    }
    unsigned int m[6];
    uint8_t mm[6];
    if (sscanf(mac, "%2X:%2X:%2X:%2X:%2X:%2X", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != 6) {
        error("MAC address is not valid: %s", mac);
        return NULL;
    }
    for (unsigned int i = 0; i < 6; ++i) {
        if (m[i] > 255) {
            error("MAC address is not valid: %s", mac);
            return NULL;
        }
        mm[i] = (uint8_t) m[i];
    }
    GByteArray *array = g_byte_array_sized_new(6);
    g_byte_array_append(array, mm, 6);
    return array;
}

char *uuid_gen(void)
{
    uuid_t uuid;
    char *buf = malloc(37 * sizeof(char));
    if (buf == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, &buf[0]);
    return buf;
}

GByteArray *ip6ArrayFromString(const char *addr)
{
    void *data;
    if (addr != NULL) {
        struct in6_addr *ip = ip6FromString(addr);
        if (ip == NULL) {
            data = calloc(sizeof(guint8), 16);
            if (data == NULL) {
                error("Memory allocation failed");
                return NULL;
            }
            return g_byte_array_new_take(data, 16);
        } else {
            GByteArray *array = g_byte_array_sized_new(16);
            if (array == NULL) {
                free(ip);
                error("Memory allocation failed");
                return NULL;
            }
            if (g_byte_array_append(array, ip->s6_addr, 16) == NULL) {
                error("Memory allocation failed");
                free(ip);
                return NULL;
            }
            free(ip);
            return array;
        }
    } else {
        data = calloc(sizeof(guint8), 16);
        if (data == NULL) {
            error("Memory allocation failed");
            return NULL;
        }
        return g_byte_array_new_take(data, 16);
    }
}

char *ip6ArrayToString(GByteArray *array)
{
    char *ip = malloc(INET6_ADDRSTRLEN);
    if (ip == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    if (inet_ntop(AF_INET6, array->data, ip, INET6_ADDRSTRLEN) == NULL) {
        free(ip);
        return NULL;
    }
    return ip;
}

GValue *g_value_new(GType t)
{
    GValue *v = calloc(1, sizeof(GValue));
    if (v == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    g_value_init(v, t);
    return v;
}

void g_value_free(GValue *v)
{
    if (v != NULL) {
        g_value_unset(v);
    }
    free(v);
}

LMIResult g_hash_table_insert_string(GHashTable *hash, const char *key, const char *value)
{
    char *k, *v;
    if ((k = strdup(key)) == NULL ||
        (v = strdup(value)) == NULL) {

        error("Memory allocation failed");
        free(k);
        return LMI_ERROR_MEMORY;
    }
    g_hash_table_insert(hash, k, v);
    return LMI_SUCCESS;
}

LMIResult g_hash_table_insert_string_value(GHashTable *hash, const char *key, const char *value)
{
    GValue *v = g_value_new(G_TYPE_STRING);
    if (v == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    char *val = strdup(value);
    if (val == NULL) {
        error("Memory allocation failed");
        g_value_free(v);
        return LMI_ERROR_MEMORY;
    }
    g_value_take_string(v, val);
    g_hash_table_insert(hash, strdup(key), v);
    return LMI_SUCCESS;
}

LMIResult g_hash_table_insert_boxed(GHashTable *hash, const char *key, GType t, gconstpointer value, bool take)
{
    GValue *v = g_value_new(t);
    if (v == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    if (take) {
        g_value_take_boxed(v, value);
    } else {
        g_value_set_boxed(v, value);
    }
    char *k;
    if ((k = strdup(key)) == NULL) {
        error("Memory allocation failed");
        g_value_free(v);
        return LMI_ERROR_MEMORY;
    }
    g_hash_table_insert(hash, k, v);
    return LMI_SUCCESS;
}

LMIResult g_hash_table_insert_uint(GHashTable *hash, const char *key, uint value)
{
    GValue *v = g_value_new(G_TYPE_UINT);
    if (v == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    g_value_set_uint(v, value);
    char *k;
    if ((k = strdup(key)) == NULL) {
        error("Memory allocation failed");
        g_value_free(v);
        return LMI_ERROR_MEMORY;
    }
    g_hash_table_insert(hash, k, v);
    return LMI_SUCCESS;
}

LMIResult g_hash_table_insert_bool(GHashTable *hash, const char *key, bool value)
{
    GValue *v = g_value_new(G_TYPE_BOOLEAN);
    if (v == NULL) {
        error("Memory allocation failed");
        return LMI_ERROR_MEMORY;
    }
    g_value_set_boolean(v, value);
    char *k;
    if ((k = strdup(key)) == NULL) {
        error("Memory allocation failed");
        g_value_free(v);
        return LMI_ERROR_MEMORY;
    }
    g_hash_table_insert(hash, k, v);
    return LMI_SUCCESS;
}

int find_index(const char *key, const char *list[])
{
    int i = 0;
    while (list[i] != NULL) {
        if (strcmp(key, list[i]) == 0) {
            return i;
        }
        i++;
    }
    // Return -1 if not found
    return -1;
}

char *id_from_instanceid(const char *instanceid, const char *cls)
{
    // Check "ORGID:" prefix
    size_t orgidlen = strlen(ORGID ":");
    if (strncmp(instanceid, ORGID ":", orgidlen) != 0) {
        error("Wrong InstanceID format: %s", instanceid);
        return NULL;
    }

    // Check "classname:"
    const char *clsname = instanceid + orgidlen;
    size_t clslen = strlen(cls);
    if (strncmp(clsname, cls, clslen) != 0) {
        error("Wrong InstanceID format: %s", instanceid);
        return NULL;
    }
    if (clsname[clslen] != ':') {
        error("Wrong InstanceID format: %s", instanceid);
        return NULL;
    }
    if (strlen(clsname) < clslen + 1) {
        error("Wrong InstanceID format: %s", instanceid);
        return NULL;
    }
    return strdup(clsname + clslen + 1);
}

char *id_from_instanceid_with_index(const char *instanceid, const char *cls, size_t *index)
{
    char *id;
    if ((id = id_from_instanceid(instanceid, cls)) == NULL) {
        return NULL;
    }
    char *indexpos;
    if ((indexpos = strchr(id, '_')) == NULL) {
        error("Wrong InstanceID format: %s", instanceid);
        free(id);
        return NULL;
    }
    if (sscanf(indexpos, "_%lu", index) < 1) {
        error("Wrong InstanceID format: %s", instanceid);
        free(id);
        return NULL;
    }
    // ID ends with first underscore
    *indexpos = '\0';
    return id;
}

char *id_from_instanceid_with_index2(const char *instanceid, const char *cls, size_t *index1, size_t *index2)
{
    char *id;
    if ((id = id_from_instanceid(instanceid, cls)) == NULL) {
        return NULL;
    }
    char *indexpos;
    if ((indexpos = strchr(id, '_')) == NULL) {
        error("Wrong InstanceID format: %s", instanceid);
        free(id);
        return NULL;
    }
    if (sscanf(indexpos, "_%lu_%lu", index1, index2) < 2) {
        error("Wrong InstanceID format: %s", instanceid);
        free(id);
        return NULL;
    }
    // ID ends with first underscore
    *indexpos = '\0';
    return id;
}

char *id_to_instanceid(const char *id, const char *cls)
{
    assert(id != NULL);
    assert(cls != NULL);
    char *instanceid;
    if (asprintf(&instanceid, ORGID ":%s:%s", cls, id) < 0) {
        return NULL;
    }
    return instanceid;
}

char *id_to_instanceid_with_index(const char *id, const char *cls, size_t index)
{
    assert(id != NULL);
    assert(cls != NULL);
    char *instanceid;
    if (asprintf(&instanceid, ORGID ":%s:%s_%ld", cls, id, index) < 0) {
        return NULL;
    }
    return instanceid;
}

char *id_to_instanceid_with_index2(const char *id, const char *cls, size_t index1, size_t index2)
{
    assert(id != NULL);
    assert(cls != NULL);
    char *instanceid;
    if (asprintf(&instanceid, ORGID ":%s:%s_%ld_%ld", cls, id, index1, index2) < 0) {
        return NULL;
    }
    return instanceid;
}

bool key_value_parse(char *input, char **key, char **value, char **saveptr)
{
    char *token = strtok_r(*saveptr != NULL ? NULL : input, ",", saveptr);
    if (token == NULL) {
        *key = NULL;
        *value = NULL;
        return false;
    }
    char *delim = strstr(token, "=");
    if (delim != NULL) {
        *delim = '\0';
        *value = delim + 1;
    } else {
        *value = NULL;
    }
    *key = token;
    return true;
}
