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

#ifndef DBUS_WRAPPER_H
#define DBUS_WRAPPER_H

#include <dbus/dbus-glib.h>
#include <stdbool.h>

GHashTable *dbus_get_properties(DBusGProxy *proxy, const char *objectpath, const char *iface);
GValue *dbus_get_property(DBusGProxy *proxy, const char *objectpath, const char *iface, const char *prop);

bool dbus_property_bool(GHashTable *hash, const char *key, bool default_value);
unsigned int dbus_property_uint(GHashTable *hash, const char *key);
const char *dbus_property_string(GHashTable *hash, const char *key);
const char *dbus_property_objectpath(GHashTable *hash, const char *key);
GPtrArray *dbus_property_array(GHashTable *hash, const char *key);
GHashTable *dbus_property_map(GHashTable *hash, const char *key);

#define DBUS_PROP_IFACE "org.freedesktop.DBus.Properties"

#define DBUS_TYPE_G_MAP_OF_VARIANT         (dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#define DBUS_TYPE_G_MAP_OF_MAP_OF_VARIANT  (dbus_g_type_get_map("GHashTable", G_TYPE_STRING, DBUS_TYPE_G_MAP_OF_VARIANT))
#define DBUS_TYPE_G_MAP_OF_STRING          (dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_STRING))
#define DBUS_TYPE_G_ARRAY_OF_OBJECT_PATH   (dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))

#endif
