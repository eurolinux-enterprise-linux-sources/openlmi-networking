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
#include "mock_nm.h"
#include "test_network.h"
#include "network.h"
#include "network_private.h"
#include "port_private.h"
#include "connection.h"
#include "errors.h"
#include "connection_private.h"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

START_TEST(test_network_port_by_mac)
{
    Network *network = malloc(sizeof(Network));
    network->ports = ports_new(5);
    Port *port;

    port = port_new();
    port->id = strdup("port1");
    port->mac = strdup("00:00:00:00:00:00");
    ports_add(network->ports, port);

    port = port_new();
    port->id = strdup("port2");
    port->mac = strdup("00:11:22:33:44:55");
    ports_add(network->ports, port);

    fail_unless(network_port_by_mac(network, "FF:FF:FF:FF:FF:FF") == NULL);

    port = port_new();
    port->id = strdup("port3");
    port->mac = strdup("FF:FF:FF:FF:FF:FF");
    ports_add(network->ports, port);

    fail_unless(network_port_by_mac(network, "FF:FF:FF:FF:FF:FF") != NULL);

    ports_free(network->ports, true);
    free(network);
}
END_TEST

START_TEST(test_network_priv_activate_connection)
{
    Network *network = malloc(sizeof(Network));
    network->ports = ports_new(5);
    network->connections = connections_new(5);
    Port *port1, *port2;

    port1 = port_new();
    port1->id = strdup("port1");
    port1->uuid = strdup("port1");
    ports_add(network->ports, port1);

    port2 = port_new();
    port2->id = strdup("port2");
    port2->uuid = strdup("port2");

    Connection *connection1, *connection2;
    connection1 = connection_new(network, "Test1", "Test1");
    connection1->port = port1;
    connections_add(network->connections, connection1);

    connection2 = connection_new(network, "Test2", "Test2");
    connection2->port = NULL;

    fail_unless(network_activate_connection(network, port1, connection1, NULL) == LMI_SUCCESS);
    fail_unless(network_activate_connection(network, port2, connection1, NULL) == LMI_ERROR_CONNECTION_INVALID);
    fail_unless(network_activate_connection(network, port1, connection2, NULL) == LMI_SUCCESS);
    fail_unless(network_activate_connection(network, port2, connection2, NULL) == LMI_SUCCESS);

    connections_free(network->connections, true);
    connection_free(connection2);
    ports_free(network->ports, true);
    port_free(port2);
    free(network);
}
END_TEST

START_TEST(test_network_set_autoconnect)
{
    Network *network = malloc(sizeof(Network));
    network->connections = connections_new(3);
    network->ports = ports_new(2);

    Port *p1, *p2;
    p1 = port_new();
    p1->id = strdup("port1");
    p1->uuid = strdup("port1");
    p1->mac = strdup("00:11:AA:BB:CC:DD:EE");
    ports_add(network->ports, p1);
    p2 = port_new();
    p2->id = strdup("port2");
    p2->uuid = strdup("port2");
    ports_add(network->ports, p2);

    Connection *c1, *c2, *c3, *c4;
    c1 = connection_new(network, "Test1", "Test1");
    c1->port = p1;
    connections_add(network->connections, c1);

    c2 = connection_new(network, "Test2", "Test2");
    connections_add(network->connections, c2);

    c3 = connection_new(network, "Test3", "Test3");
    c3->port = p1;
    connections_add(network->connections, c3);

    c4 = connection_new(network, "Test4", "Test4");
    connections_add(network->connections, c4);


    network_set_autoconnect(network, p1, c1, true);
    fail_unless(c1->autoconnect == true);
    fail_unless(c2->autoconnect == false);
    fail_unless(c3->autoconnect == false);
    fail_unless(c4->autoconnect == false);

    network_set_autoconnect(network, p1, c2, true);
    fail_unless(c1->autoconnect == false);
    fail_unless(c2->autoconnect == true);
    fail_unless(c3->autoconnect == false);
    fail_unless(c4->autoconnect == false);

    network_set_autoconnect(network, p1, c2, false);
    fail_unless(c1->autoconnect == false);
    fail_unless(c2->autoconnect == false);
    fail_unless(c3->autoconnect == false);
    fail_unless(c4->autoconnect == false);

    network_set_autoconnect(network, p1, c2, true);
    fail_unless(c1->autoconnect == false);
    fail_unless(c2->autoconnect == true);
    fail_unless(c3->autoconnect == false);
    fail_unless(c4->autoconnect == false);

    network_set_autoconnect(network, p2, c4, true);
    fail_unless(c1->autoconnect == false);
    fail_unless(c2->autoconnect == false);
    fail_unless(c3->autoconnect == false);
    fail_unless(c4->autoconnect == true);

    connections_free(network->connections, true);
    ports_free(network->ports, true);
    free(network);
}
END_TEST

// forward declaration of private function
PortStats *network_get_ports_statistics_priv(Network *network, FILE *f, LMIResult *res);

#define RXB1 42079338795L
#define RXP1 37888081L
#define RXE1 100L
#define TXB1 1939895443L
#define TXP1 20661925L
#define TXE1 99L
#define TXC1 3141592L

#define DEV_HEADER "Inter-|   Receive                                                |  Transmit\n" \
                   "face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
#define DEV_IFACE1 "eth1:     2297922    10165  0  0  0   0   0       0   21884124   307573  0  0  0  0  0  0\n"
#define DEV_IFACE_WRONG1 "eth0: 1 2 3\n"
#define DEV_IFACE_WRONG2 "eth0:\n"

START_TEST(test_network_get_ports_statistics)
{
    // Create temporary file
    char tempfile[] = "openlmi_test_network_XXXXXX";
    mode_t oldmask = umask(S_IWOTH);
    int fd = mkstemp(tempfile);
    umask(oldmask);
    if (fd < 0) {
        ck_abort_msg("Unable to create temporary file");
        return;
    }

    Network *network = malloc(sizeof(Network));
    network->ports = ports_new(3);

    Port *port = port_new();
    port->id = strdup("eth0");
    ports_add(network->ports, port);

    port = port_new();
    port->id = strdup("eth1");
    ports_add(network->ports, port);

    char *eth0;
    asprintf(&eth0, "eth0: %lu %lu %lu 0 0 0 0 0 %lu %lu %lu 0 0 %lu 0 0\n", RXB1, RXP1, RXE1, TXB1, TXP1, TXE1, TXC1);

    FILE *tmp = fdopen(fd, "w+");
    if (tmp == NULL) {
        ck_abort_msg("fdopen failed");
        ports_free(network->ports, true);
        free(network);
        free(eth0);
        return;
    }
    fprintf(tmp, "%s%s%s", DEV_HEADER, eth0, DEV_IFACE1);
    rewind(tmp);
    free(eth0);

    LMIResult res;
    PortStats *stats = network_get_ports_statistics_priv(network, tmp, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    fail_unless(stats != NULL);
    ck_assert_int_eq(port_stats_length(stats), 2);
    fail_unless(strcmp(port_stats_index(stats, 0)->port->id, "eth0") == 0);
    fail_unless(port_stats_index(stats, 0)->rx_bytes == RXB1);
    fail_unless(port_stats_index(stats, 0)->rx_packets == RXP1);
    fail_unless(port_stats_index(stats, 0)->rx_errs == RXE1);
    fail_unless(port_stats_index(stats, 0)->tx_bytes == TXB1);
    fail_unless(port_stats_index(stats, 0)->tx_packets == TXP1);
    fail_unless(port_stats_index(stats, 0)->tx_errs == TXE1);
    fail_unless(port_stats_index(stats, 0)->tx_colls == TXC1);
    port_stats_free(stats, true);

    // Test wrong device line
    tmp = fopen(tempfile, "w+");
    if (tmp == NULL) {
        ck_abort_msg("fopen failed");
        ports_free(network->ports, true);
        free(network);
        return;
    }

    fprintf(tmp, "%s%s", DEV_HEADER, DEV_IFACE_WRONG1);
    rewind(tmp);
    stats = network_get_ports_statistics_priv(network, tmp, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    fail_unless(stats != NULL);
    fail_unless(port_stats_length(stats) == 0);
    port_stats_free(stats, true);

    // Test another wrong device line
    tmp = fopen(tempfile, "w+");
    if (tmp == NULL) {
        ck_abort_msg("fopen failed");
        ports_free(network->ports, true);
        free(network);
        return;
    }

    fprintf(tmp, "%s%s", DEV_HEADER, DEV_IFACE_WRONG2);
    rewind(tmp);
    stats = network_get_ports_statistics_priv(network, tmp, &res);
    ck_assert_int_eq(res, LMI_SUCCESS);
    fail_unless(stats != NULL);
    fail_unless(port_stats_length(stats) == 0);
    port_stats_free(stats, true);

    // Test empty file
    tmp = fopen(tempfile, "w+");
    if (tmp == NULL) {
        ck_abort_msg("fopen failed");
        ports_free(network->ports, true);
        free(network);
        return;
    }

    stats = network_get_ports_statistics_priv(network, tmp, &res);
    ck_assert_int_eq(res, LMI_ERROR_BACKEND);
    fail_unless(stats == NULL);
    if (stats != NULL) {
        // Just to silence Coverity issue
        port_stats_free(stats, true);
    }

    unlink(tempfile);

    ports_free(network->ports, true);
    free(network);
}
END_TEST

TCase *network_tcase(void)
{
    TCase *tc_network = tcase_create("network");
    tcase_add_test(tc_network, test_network_port_by_mac);
    tcase_add_test(tc_network, test_network_priv_activate_connection);
    tcase_add_test(tc_network, test_network_get_ports_statistics);
    tcase_add_test(tc_network, test_network_set_autoconnect);
    return tc_network;
}
