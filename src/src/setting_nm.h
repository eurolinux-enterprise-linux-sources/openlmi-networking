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

#ifndef SETTING_NM_H
#define SETTING_NM_H

#include "globals.h"
#include "setting_private.h"

Setting *setting_new_link_local(Connection *connection, enum ProtocolType protocol);
Setting *setting_new_DHCP(Connection *connection, enum ProtocolType protocol);
Setting *setting_new_from_static_ipv4(Connection *connection, guint32 address, guint32 prefix, guint32 gateway, int index);
Setting *setting_new_from_static_ipv6(Connection *connection, GByteArray *address, guint32 prefix, GByteArray *gateway, int index);
const char *setting_get_nm_method(Setting *setting);

Setting *setting_from_hash(GHashTable *hash, const char *key, LMIResult *res);
GHashTable *setting_to_hash(Setting *setting, char **key, LMIResult *res);

GArray *address_to_ipv4_array(Address *address);
Address *ipv4_array_to_address(GArray *array);
GValueArray *address_to_ipv6_array(Address *address);
Address *ipv6_array_to_address(GValueArray *array);

#endif
