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

#ifndef NM_SUPPORT_H
#define NM_SUPPORT_H

#include "globals.h"
#include "ipconfig.h"
#include <glib.h>
#include <glib-object.h>

#define IPv4_ADDR G_TYPE_UINT
#define IPv6_ADDR DBUS_TYPE_G_UCHAR_ARRAY

#define NM_TYPE_IP4_ADDRESS (dbus_g_type_get_collection("GArray", G_TYPE_UINT))
#define NM_TYPE_IP4_ADDRESSES (dbus_g_type_get_collection("GPtrArray", NM_TYPE_IP4_ADDRESS))
#define NM_TYPE_IP6_ADDRESS (dbus_g_type_get_struct("GValueArray", IPv6_ADDR, G_TYPE_UINT, IPv6_ADDR, G_TYPE_INVALID))
#define NM_TYPE_IP6_ADDRESSES (dbus_g_type_get_collection("GPtrArray", NM_TYPE_IP6_ADDRESS))

#define NM_TYPE_IP4_ROUTE  (dbus_g_type_get_collection("GArray", IPv4_ADDR))
#define NM_TYPE_IP4_ROUTES (dbus_g_type_get_collection("GPtrArray", NM_TYPE_IP4_ROUTE))
#define NM_TYPE_IP6_ROUTE  (dbus_g_type_get_struct("GValueArray", IPv6_ADDR, G_TYPE_UINT, IPv6_ADDR, G_TYPE_UINT, G_TYPE_INVALID))
#define NM_TYPE_IP6_ROUTES (dbus_g_type_get_collection("GPtrArray", NM_TYPE_IP6_ROUTE))

#define NM_TYPE_DNS4_SERVERS (dbus_g_type_get_collection("GArray", IPv4_ADDR))
#define NM_TYPE_DNS6_SERVERS (dbus_g_type_get_collection("GPtrArray", IPv6_ADDR))

//#define NM_TYPE_SEARCH_DOMAINS (dbus_g_type_get_collection("GPtrArray", G_TYPE_STRING))
#define NM_TYPE_SEARCH_DOMAINS (G_TYPE_STRV)

GArray *address_to_ipv4_array(Address *address);
Address *ipv4_array_to_address(GArray *array);

GValueArray *address_to_ipv6_array(Address *address);
Address *ipv6_array_to_address(GValueArray *array);

LMIResult dns_servers4_fill_from_gvalue(DNSServers *dns_servers, GValue *v);
LMIResult dns_servers6_fill_from_gvalue(DNSServers *dns_servers, GValue *v);

LMIResult routes4_fill_from_gvalue(Routes *routes, GValue *v);
LMIResult routes6_fill_from_gvalue(Routes *routes, GValue *v);

void g_hash_table_print(GHashTable *hash);

#endif
