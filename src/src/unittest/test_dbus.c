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

#include "test.h"
#include "test_dbus.h"
#include "setting_nm.h"
#include "dbus_wrapper.h"

START_TEST(test_ipv4)
{
    Address *address = address_new(IPv4);
    address->addr = strdup("192.168.1.100");
    address->prefix = 24;
    address->default_gateway = strdup("192.168.1.1");
    GArray *ip4array = address_to_ipv4_array(address);
    fail_unless(ip4array->len == 3, "Unable to create IPv4 dbus array");
    address_free(address);
    g_array_free(ip4array, true);

    address = address_new(IPv6);
    address->addr = strdup("2001:0db8:0000:2219:0210:18ff:fe97:2222");
    address->prefix = 24;
    address->default_gateway = strdup("2001:0db8:0000:2219:0210:18ff:fe97:0001");
    GValueArray *ip6array = address_to_ipv6_array(address);
    fail_unless(ip6array->n_values == 3, "Unable to create IPv6 dbus array");
    GValue *v = g_value_array_get_nth(ip6array, 0);
    fail_unless(v != NULL, "");
    fail_unless(G_VALUE_HOLDS(v, DBUS_TYPE_G_UCHAR_ARRAY));
    g_value_array_free(ip6array);
    address_free(address);
}
END_TEST

TCase *dbus_tcase(void)
{
    TCase *tc_globals = tcase_create("dbus");
    tcase_add_test(tc_globals, test_ipv4);
    return tc_globals;
}
