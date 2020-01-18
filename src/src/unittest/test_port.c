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

#include "port.h"
#include "port_private.h"
#include "test.h"

START_TEST(test_port_compare)
{
    Port *port1 = port_new();
    Port *port2 = port_new();

    fail_unless(port_compare(NULL, NULL) == false);
    fail_unless(port_compare(port1, NULL) == false);
    fail_unless(port_compare(NULL, port2) == false);
    fail_unless(port_compare(port1, port2) == false);

    port1->id = strdup("port1");
    fail_unless(port_compare(port1, port2) == false);

    port2->id = strdup("port2");
    fail_unless(port_compare(port1, port2) == false);

    fail_unless(port_compare(port1, port1) == true);

    port_free(port1);
    port_free(port2);
}
END_TEST

START_TEST(test_ports_find)
{
    Ports *ports = ports_new(5);
    Port *p1 = port_new();
    p1->uuid = strdup("uuid1");
    ports_add(ports, p1);
    Port *p2 = port_new();
    p2->uuid = strdup("uuid2");
    ports_add(ports, p2);

    fail_unless(ports_find_by_uuid(ports, "uuid1") == p1);
    fail_unless(ports_find_by_uuid(ports, "uuid1") != p2);
    fail_unless(ports_find_by_uuid(ports, "uuid2") == p2);
    fail_unless(ports_find_by_uuid(ports, "uuid3") == NULL);
    fail_unless(ports_find_by_uuid(ports, NULL) == NULL);
    fail_unless(ports_find_by_uuid(NULL, "uuid1") == NULL);

    ports_free(ports, true);
}
END_TEST

TCase *port_tcase(void)
{
    TCase *tc_port = tcase_create("port");
    tcase_add_test(tc_port, test_port_compare);
    tcase_add_test(tc_port, test_ports_find);
    return tc_port;
}
