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
#include "test.h"
#include "test_activeconnection.h"
#include "activeconnection.h"
#include "activeconnection_private.h"
#include "port.h"
#include "port_private.h"


START_TEST(test_active_connection_is_port_active)
{
    ActiveConnection *activeconnection = active_connection_new(NULL);
    activeconnection->ports = ports_new(3);

    Port *port1 = port_new();
    port1->id = strdup("1");

    Port *port2 = port_new();
    port2->id = strdup("2");

    Port *port3 = port_new();
    port3->id = strdup("3");

    fail_unless(!active_connection_is_port_active(activeconnection, port1), "Port shouldn't be reported as active");
    fail_unless(!active_connection_is_port_active(activeconnection, port2), "Port shouldn't be reported as active");
    fail_unless(!active_connection_is_port_active(activeconnection, port3), "Port shouldn't be reported as active");

    ports_add(activeconnection->ports, port2);
    fail_unless(!active_connection_is_port_active(activeconnection, port1), "Port shouldn't be reported as active");
    fail_unless(active_connection_is_port_active(activeconnection, port2), "Port should be reported as active");
    fail_unless(!active_connection_is_port_active(activeconnection, port3), "Port shouldn't be reported as active");

    ports_add(activeconnection->ports, port1);
    fail_unless(active_connection_is_port_active(activeconnection, port1), "Port should be reported as active");
    fail_unless(active_connection_is_port_active(activeconnection, port2), "Port should be reported as active");
    fail_unless(!active_connection_is_port_active(activeconnection, port3), "Port shouldn't be reported as active");

    ports_free(activeconnection->ports, true);
    port_free(port3);
    activeconnection->ports = NULL;
    active_connection_free(activeconnection);
}
END_TEST

TCase *activeconnection_tcase(void)
{
    TCase *tc = tcase_create("ActiveConnection");
    tcase_add_test(tc, test_active_connection_is_port_active);
    return tc;
}
