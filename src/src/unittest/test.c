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

#include <check.h>
#include <stdlib.h>
#include "test_globals.h"
#include "test_activeconnection.h"
#include "test_connection.h"
#include "test_network.h"
#include "test_port.h"
#include "test_setting.h"
#include "test_dbus.h"

Suite *cim_networking_suite(void)
{
    Suite *suite = suite_create("cim-networking");

    suite_add_tcase(suite, globals_tcase());
    suite_add_tcase(suite, activeconnection_tcase());
    suite_add_tcase(suite, connection_tcase());
    suite_add_tcase(suite, network_tcase());
    suite_add_tcase(suite, port_tcase());
    suite_add_tcase(suite, setting_tcase());
    suite_add_tcase(suite, dbus_tcase());
    return suite;
}

int main(int argc, char *argv[])
{
    g_type_init();
    Suite *suite = cim_networking_suite();
    SRunner *runner = srunner_create(suite);
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        srunner_set_fork_status(runner, CK_NOFORK);
    }
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
