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

#ifndef GLOBALS_H
#define GLOBALS_H

struct in6_addr;

// Disable glib deprecation warnings
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <openlmi.h>

// Unique identification of organization, should be trademarked, but meh.
#define ORGID "LMI"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib-object.h>

#include <assert.h>
#include <cmpidt.h>
#include "errors.h"

typedef struct Network Network;
typedef struct Port Port;
typedef struct Ports Ports;
typedef struct PortStat PortStat;
typedef struct PortStats PortStats;
typedef struct Connection Connection;
typedef struct Connections Connections;
typedef struct ActiveConnection ActiveConnection;
typedef struct ActiveConnections ActiveConnections;
typedef struct Setting Setting;
typedef struct Settings Settings;

typedef enum ProtocolType { Unknown=0, IPv4=1, IPv6=2, IPv4_IPv6=3 } ProtocolType;

/** Type of the connection
 * Numbers must be the same as in IPNetworkConnectionCapabilities
 */
typedef enum ConnectionType {
    CONNECTION_TYPE_UNKNOWN=0,
    CONNECTION_TYPE_ETHERNET=1,
    CONNECTION_TYPE_BOND=4,
    CONNECTION_TYPE_BRIDGE=5
} ConnectionType;

typedef enum SettingType {
    SETTING_TYPE_UNKNOWN=-1,
    SETTING_TYPE_IPv4=0,
    SETTING_TYPE_IPv6,
    SETTING_TYPE_WIRED,
    SETTING_TYPE_BOND,
    SETTING_TYPE_BRIDGE,
    SETTING_TYPE_BRIDGE_SLAVE
} SettingType;

/** Type of the address
 * Numbers must be the same as in IPNetworkConnectionCapabilities
 */
typedef enum SettingMethod {
    SETTING_METHOD_UNKNOWN=-1,
    SETTING_METHOD_DISABLED=0,
    SETTING_METHOD_STATIC=3,
    SETTING_METHOD_DHCP=4,
    // BOOTP=5,
    // SETTING_METHOD_LINK_LOCAL_IPv4=6,
    SETTING_METHOD_DHCPv6=7,
    // IPv6_AutoConfig=8
    SETTING_METHOD_STATELESS=9,
    SETTING_METHOD_LINK_LOCAL=10
} SettingMethod;

/**
 * Add an instance \p w to the result \p cr.
 *
 * @param cr CMPIResult where should be the instance added
 * @param w instance to add
 * @retval true if succeeds
 * @retval false if addition fails
 */
#define ReturnInstance(cr, w) KOkay(__KReturnInstance((cr), &(w).__base))

#define get_system_name lmi_get_system_name
#define get_system_creation_class_name lmi_get_system_creation_class_name

GValue *gvalue_new_from_value(GType type, gconstpointer data);

char *ip4ToString(uint32_t i);
uint32_t ip4FromString(const char *ip);

char *ip6ToString(const struct in6_addr *i);
struct in6_addr *ip6FromString(const char *);

char *prefixToMask4(uint8_t prefix);
uint8_t netmaskToPrefix4(const char *);

char *macFromGByteArray(const GByteArray *mac);
GByteArray *macToGByteArray(const char *mac);

GByteArray *ip6ArrayFromString(const char *addr);
char *ip6ArrayToString(GByteArray *ip);

GValue *g_value_new(GType t);
void g_value_free(GValue *v);

LMIResult g_hash_table_insert_string(GHashTable *hash, const char *key, const char *value);
LMIResult g_hash_table_insert_string_value(GHashTable *hash, const char *key, const char *value);
LMIResult g_hash_table_insert_boxed(GHashTable *hash, const char *key, GType t, gconstpointer value, bool take);
LMIResult g_hash_table_insert_uint(GHashTable *hash, const char *key, uint value);
LMIResult g_hash_table_insert_bool(GHashTable *hash, const char *key, bool value);

bool key_value_parse(char *input, char **key, char **value, char **saveptr);

int find_index(const char *key, const char *list[]);

/** Get id from InstanceID for given class
 *
 * Format: LMI:cls:id
 *
 * \param instanceid string with InstanceID property
 * \param cls string with class name
 * \return id of the instance (caller is responsibe to free it)
 */
char *id_from_instanceid(const char *instanceid, const char *cls);

/** Get id from InstanceID for given class
 *
 * Format: LMI:cls:id_index
 *
 * \param[in] instanceid string with InstanceID property
 * \param[in] cls string with class name to check
 * \param[out] index index of instance
 * \return id of the instance (caller is responsibe to free it)
 */
char *id_from_instanceid_with_index(const char *instanceid, const char *cls, size_t *index);

/** Get id from InstanceID for given class
 *
 * Format: LMI:cls:id_index1_index2
 *
 * \param instanceid string with InstanceID property
 * \param cls string with class name
 * \param[out] index1 first index of instance
 * \param[out] index2 second index of instance
 * \return id of the instance (caller is responsibe to free it)
 */
char *id_from_instanceid_with_index2(const char *instanceid, const char *cls, size_t *index1, size_t *index2);


/** Create InstanceID for given class
 *
 * Format: LMI:cls:id
 *
 * \param id string with unique id
 * \param cls string with class name
 * \return InstanceID (caller is responsibe to free it)
 */
char *id_to_instanceid(const char *id, const char *cls);

/** Create InstanceID for given class
 *
 * Format: LMI:cls:id_index
 *
 * \param id string with id
 * \param cls string with class name
 * \param index index of instance
 * \return InstanceID (caller is responsibe to free it)
 */
char *id_to_instanceid_with_index(const char *id, const char *cls, size_t index);

/** Create InstanceID for given class
 *
 * Format: LMI:cls:id_index1_index2
 *
 * \param id string with unique id
 * \param cls string with class name
 * \param index1 first index of instance
 * \param index2 second index of instance
 * \return InstanceID (caller is responsibe to free it)
 */
char *id_to_instanceid_with_index2(const char *id, const char *cls, size_t index1, size_t index2);

/**
 * Generate random UUID
 *
 * \return random UUID (caller is responsibe to free it)
 */
char *uuid_gen(void);

#define LOG_ID "openlmi-networking"

#define debug lmi_debug
#define warn lmi_warn
#define info lmi_info
#define error lmi_error

#define foreach(item, list, items)                                             \
    for (size_t i = 0, item = items##_index(list, 0);                          \
         i < items##_length(list);                                             \
         item = items##_index(list, i++))

/** This macro defines declaration of list operations for type \p Item and
 * member call \p item. List will be called same as the type with 's' suffixed.
 *
 * Example: LIST_DECL(Test, test) will declare following functions:
 *     Tests *tests_new(size_t preallocated);
 *     int tests_add(Tests *tests, Test *test);
 *     Test *tests_index(const Tests *tests, size_t index);
 *     size_t tests_length(const Tests *tests);
 *     Test *tests_pop(Tests *tests, size_t index);
 *     void tests_free(Tests *tests, bool deep);
 */
#define LIST_DECL(Item, item) LIST_DECL2(Item, item, Item##s, item##s)

/** Same as LIST_DECL but you can specify how will the created type be called.
 *
 * \see LIST_DECL
 */
#define LIST_DECL2(Item, item, Items, items)                                   \
Items *items##_new(size_t preallocated);                                       \
int items##_add(Items *items, Item *item);                                     \
Item *items##_index(const Items *items, size_t index);                         \
size_t items##_length(const Items *items);                                     \
Item *items##_pop(Items *items, size_t index);                                 \
void items##_free(Items *items, bool deep);

/** This macro defines implementation of list operations for structure \p Item and
 * member call \p item. Use together with LIST_DECL macro.
 *
 * \see LIST_DECL
 */
#define LIST_IMPL(Item, item) LIST_IMPL2(Item, item, Item##s, item##s)

/** Same as LIST_IMPL but you can specify how will the created type be called.
 *
 * \see LIST_DECL, LIST_IMPL
 */
#define LIST_IMPL2(Item, item, Items, items)                                   \
struct Items {                                                                 \
    Item **items;                                                              \
    size_t length;                                                             \
    size_t allocated;                                                          \
};                                                                             \
                                                                               \
Items *items##_new(size_t preallocated)                                        \
{                                                                              \
    if (preallocated > SIZE_MAX / sizeof(Item *)) {                            \
        error("Malloc overflow detected");                                     \
        return NULL;                                                           \
    }                                                                          \
    Items *items = malloc(sizeof(Items));                                      \
    if (items == NULL) {                                                       \
        error("Memory allocation failed");                                     \
        return NULL;                                                           \
    }                                                                          \
    if (preallocated > 0) {                                                    \
        items->items = malloc(preallocated * sizeof(Item *));                  \
        if (items->items == NULL) {                                            \
            error("Memory allocation failed");                                 \
            free(items);                                                       \
            return NULL;                                                       \
        }                                                                      \
    } else {                                                                   \
        items->items = NULL;                                                   \
    }                                                                          \
    items->allocated = preallocated;                                           \
    items->length = 0;                                                         \
    return items;                                                              \
}                                                                              \
                                                                               \
int items##_add(Items *items, Item *item)                                      \
{                                                                              \
    assert(items != NULL);                                                     \
    if (items->allocated <= items->length) {                                   \
        items->allocated += 5;                                                 \
        if (items->allocated > SIZE_MAX / sizeof(Item *)) {                    \
            error("Realloc overflow detected");                                \
            return LMI_ERROR_MEMORY;                                           \
        }                                                                      \
        void *tmp = realloc(items->items, items->allocated * sizeof(Item *));  \
        if (tmp == NULL) {                                                     \
            error("Memory reallocation failed");                               \
            return LMI_ERROR_MEMORY;                                           \
        }                                                                      \
        items->items = tmp;                                                    \
    }                                                                          \
    items->items[items->length++] = item;                                      \
    return LMI_SUCCESS;                                                        \
}                                                                              \
                                                                               \
Item *items##_index(const Items *items, size_t index)                          \
{                                                                              \
    assert(items != NULL);                                                     \
    if (index < items->length)                                                 \
        return items->items[index];                                            \
    else                                                                       \
        return NULL;                                                           \
}                                                                              \
                                                                               \
size_t items##_length(const Items *items)                                      \
{                                                                              \
    if (items == NULL)                                                         \
        return 0;                                                              \
    return items->length;                                                      \
}                                                                              \
                                                                               \
Item *items##_pop(Items *items, size_t index)                                  \
{                                                                              \
    assert(items != NULL);                                                     \
    assert(items->items != NULL);                                              \
    if (index >= items->length) {                                              \
        return NULL;                                                           \
    }                                                                          \
    Item *item = items->items[index];                                          \
    for (size_t i = index; i < items->length - 1; ++i) {                       \
        items->items[i] = items->items[i + 1];                                 \
    }                                                                          \
    items->length--;                                                           \
    return item;                                                               \
}                                                                              \
                                                                               \
void items##_free(Items *items, bool deep)                                     \
{                                                                              \
    if (items == NULL) {                                                       \
        return;                                                                \
    }                                                                          \
    if (deep) {                                                                \
        if (items->items != NULL) {                                            \
            for (size_t i = 0; i < items->length; ++i) {                       \
                item##_free(items->items[i]);                                  \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    if (items->items != NULL) {                                                \
        free(items->items);                                                    \
    }                                                                          \
    free(items);                                                               \
}

/**
 * Test if \p Variable is not null and exists.
 * Show \p Error and return if not.
 */
#define Require(Variable, Error, result, code)              \
if (Variable->null || !Variable->exists) {                  \
    error(Error);                                           \
    KSetStatus2(_cb, status, ERR_INVALID_PARAMETER, Error); \
    result.exists = 1;                                      \
    result.null = 0;                                        \
    result.value = code;                                    \
    return result;                                          \
}

#endif
