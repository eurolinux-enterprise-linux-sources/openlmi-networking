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

#include "dbus_wrapper.h"

#include "globals.h"
#include <string.h>

GHashTable *dbus_get_properties(DBusGProxy *proxy, const char *objectpath, const char *iface)
{
    GError *err = NULL;
    GHashTable *hash;
    DBusGProxy *prop_proxy = dbus_g_proxy_new_from_proxy(proxy, DBUS_PROP_IFACE, objectpath);
    if (prop_proxy == NULL) {
        error("Unable to create DBus proxy for: %s %s %s", dbus_g_proxy_get_bus_name(proxy), objectpath, DBUS_PROP_IFACE);
        return NULL;
    }
    if (!dbus_g_proxy_call(prop_proxy, "GetAll", &err, G_TYPE_STRING, iface, G_TYPE_INVALID, DBUS_TYPE_G_MAP_OF_VARIANT, &hash, G_TYPE_INVALID)) {
        error("Calling of method " DBUS_PROP_IFACE ".GetAll (%s, %s) failed: %s", dbus_g_proxy_get_bus_name(proxy), dbus_g_proxy_get_path(proxy), err->message);
        g_error_free(err);
        return NULL;
    }
    return hash;
}

GValue *dbus_get_property(DBusGProxy *proxy, const char *objectpath, const char *iface, const char *prop)
{
    GError *err = NULL;
    DBusGProxy *prop_proxy = dbus_g_proxy_new_from_proxy(proxy, DBUS_PROP_IFACE, objectpath);
    if (prop_proxy == NULL) {
        error("Unable to create DBus proxy for: %s %s %s", dbus_g_proxy_get_bus_name(proxy), objectpath, DBUS_PROP_IFACE);
        return NULL;
    }
    GValue *value = g_slice_new0(GValue);
    if (value == NULL) {
        return NULL;
    }
    if (!dbus_g_proxy_call(prop_proxy, "Get", &err, G_TYPE_STRING, iface, G_TYPE_STRING, prop, G_TYPE_INVALID, G_TYPE_VALUE, value, G_TYPE_INVALID)) {

        error("Calling of method " DBUS_PROP_IFACE ".Get(%s, %s) on %s, %s failed: %s",
              iface, prop, dbus_g_proxy_get_bus_name(proxy),
              objectpath, err->message);
        g_error_free(err);
        return NULL;
    }
    return value;
}

bool dbus_property_bool(GHashTable *hash, const char *key, bool default_value)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        return default_value;
    }
    if (G_VALUE_HOLDS_BOOLEAN(v)) {
        return g_value_get_boolean(v);
    } else {
        return default_value;
    }
}

uint32_t dbus_property_uint(GHashTable *hash, const char *key)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        warn("Property %s doesn't exist", key);
        return 0;
    }
    if (G_VALUE_HOLDS_UINT(v)) {
        return g_value_get_uint(v);
    } else {
        warn("Property %s doesn't hold uint", key);
        return 0;
    }
}

const char *dbus_property_string(GHashTable *hash, const char *key)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        warn("Property %s doesn't exist", key);
        return NULL;
    }
    if (G_VALUE_HOLDS_STRING(v)) {
        return g_value_get_string(v);
    } else {
        warn("Property %s doesn't hold string but %s", key, G_VALUE_TYPE_NAME(v));
        return NULL;
    }
}

const char *dbus_property_objectpath(GHashTable *hash, const char *key)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        warn("Property %s doesn't exist", key);
        return NULL;
    }
    if (G_VALUE_HOLDS(v, DBUS_TYPE_G_OBJECT_PATH)) {
        return g_value_get_boxed(v);
    } else {
        warn("Property %s doesn't hold objectpath but %s", key, G_VALUE_TYPE_NAME(v));
        return NULL;
    }
}

GPtrArray *dbus_property_array(GHashTable *hash, const char *key)
{
    GValue *v = g_hash_table_lookup(hash, key);
    if (v == NULL) {
        warn("Property %s doesn't exist", key);
        return NULL;
    }
    if (G_VALUE_HOLDS_BOXED(v)) {
        return g_value_get_boxed(v);
    } else {
        warn("Property %s doesn't hold boxed but %s", key, G_VALUE_TYPE_NAME(v));
        return NULL;
    }

}

GHashTable *dbus_property_map(GHashTable *hash, const char *key)
{
    GHashTable *h = g_hash_table_lookup(hash, key);
    if (h == NULL) {
        warn("Property %s doesn't exist", key);
        return NULL;
    }
    return h;
}
