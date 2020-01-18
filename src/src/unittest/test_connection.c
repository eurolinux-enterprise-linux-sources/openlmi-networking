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
#include "mock_nm.h"
#include "test_connection.h"
#include "test_setting.h"
#include "connection.h"
#include "connection_private.h"
#include "network_private.h"


#include "setting_nm.c"
#include "connection_nm.c"
#include "setting.h"

START_TEST(test_connections_find)
{
    Connections *connections = connections_new(5);
    Connection *c1 = connection_new(NULL, "id1", NULL);
    c1->uuid = strdup("uuid1");
    connections_add(connections, c1);
    Connection *c2 = connection_new(NULL, "id2", NULL);
    c2->uuid = strdup("uuid2");
    connections_add(connections, c2);
    Connection *c3 = connection_new(NULL, NULL, NULL);
    connections_add(connections, c3);

    fail_unless(connections_find_by_id(connections, "id1") == c1);
    fail_unless(connections_find_by_id(connections, "id1") != c2);
    fail_unless(connections_find_by_id(connections, "id2") == c2);
    fail_unless(connections_find_by_id(connections, "id3") == NULL);
    fail_unless(connections_find_by_id(connections, NULL) == NULL);
    fail_unless(connections_find_by_id(NULL, "id1") == NULL);

    fail_unless(connections_find_by_uuid(connections, "uuid1") == c1);
    fail_unless(connections_find_by_uuid(connections, "uuid1") != c2);
    fail_unless(connections_find_by_uuid(connections, "uuid2") == c2);
    fail_unless(connections_find_by_uuid(connections, "uuid3") == NULL);
    fail_unless(connections_find_by_uuid(connections, NULL) == NULL);
    fail_unless(connections_find_by_uuid(NULL, "uuid1") == NULL);

    connections_free(connections, true);
}
END_TEST

void compare_connections(Connection *c1, Connection *c2)
{
    assert_string_eq(c1->uuid, c2->uuid);
    assert_string_eq(c1->id, c2->id);
    assert_string_eq(c1->name, c2->name);
    ck_assert_int_eq(c1->type, c2->type);
    ck_assert_int_eq(c1->autoconnect, c2->autoconnect);
    ck_assert_int_eq(settings_length(c1->settings), settings_length(c2->settings));
    for (size_t i = 0; i < settings_length(c1->settings); ++i) {
        compare_settings(settings_index(c1->settings, i), settings_index(c2->settings, i));
    }
    assert_string_eq(c1->master_id, c2->master_id);
    assert_string_eq(c1->slave_type, c2->slave_type);
}

START_TEST(test_connection_hashing)
{
    LMIResult res = LMI_SUCCESS;
    // Empty connection
    Connection *connection = connection_new(NULL, "id", "name");
    GHashTable *hash = connection_to_hash(connection, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    Connection *connection2 = connection_new(NULL, NULL, NULL);
    ck_assert_int_eq(connection_read_properties(connection2, hash), LMI_SUCCESS);
    compare_connections(connection, connection2);
    g_hash_table_destroy(hash);

    // Bond slave
    connection2->uuid = strdup("uuid");
    connection_set_master_connection(connection, connection2, SETTING_TYPE_BOND);
    hash = connection_to_hash(connection, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    Connection *connection3 = connection_new(NULL, NULL, NULL);
    connection_read_properties(connection3, hash);
    compare_connections(connection, connection3);
    connection_free(connection);
    connection_free(connection2);
    connection_free(connection3);
    g_hash_table_destroy(hash);

    connection = connection_new(NULL, "id", "name");
    connection->type = CONNECTION_TYPE_UNKNOWN;
    hash = connection_to_hash(connection, &res);
    ck_assert(hash != NULL);
    ck_assert_int_eq(res, LMI_SUCCESS);
    connection_free(connection);
    g_hash_table_unref(hash);
}
END_TEST


TCase *connection_tcase(void)
{
    TCase *tc = tcase_create("Connection");
    tcase_add_test(tc, test_connections_find);
    tcase_add_test(tc, test_connection_hashing);
    return tc;
}
