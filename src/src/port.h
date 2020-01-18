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

#ifndef PORT_H
#define PORT_H

#include "globals.h"
#include "ipconfig.h"

typedef enum PortType {
    TYPE_UNKNOWN = 0,
    TYPE_ETHERNET,
    TYPE_WIFI,
    TYPE_BT,
    TYPE_OLPC_MESH,
    TYPE_WIMAX,
    TYPE_MODEM,
    TYPE_INFINIBAND,
    TYPE_BOND,
    TYPE_BRIDGE,
    TYPE_VLAN,
    TYPE_ADSL,
    TYPE_GENERIC
} PortType;

typedef enum PortOperatingStatus {
    STATUS_UNKNOWN = 0, STATUS_NA, STATUS_IN_SERVICE, STATUS_STARTING,
    STATUS_STOPPING, STATUS_STOPPED, STATUS_ABORTED, STATUS_OFFLINE
} PortOperatingStatus;

typedef enum PortEnabledState {
    STATE_ENABLED = 2,
    STATE_DISABLED = 3,
    STATE_NO_CHANGE = 5,
    STATE_ENABLED_BUT_OFFLINE = 6,
} PortEnabledState;

Port *port_new(void);

void *port_get_priv(const Port *port);
const char *port_get_uuid(const Port *port);
const char *port_get_id(const Port *port);
PortType port_get_type(const Port *port);
PortEnabledState port_get_enabled_state(const Port *port);
PortEnabledState port_get_requested_state(const Port *port);
PortOperatingStatus port_get_operating_status(const Port *port);

Ports *port_get_slaves(Network *network, const Port *port);

/**
 * Disconnects port - currently acive connection will be disconnected
 *
 * \param port network port which will be disconnected
 * \retval LMI_SUCCESS success
 */
int port_disconnect(Port *port);

/**
 * Request state of network port
 *
 * \param port network port to which the state will be requested
 * \param state state to be set
 * \retval LMI_SUCCESS success
 * \retval LMI_WRONG_PARAMETER wrong/unsupported state
 * \retval LMI_ERROR_PORT_STATE_CHANGE_FAILED unable to set the state
 */
LMIResult port_request_state(Port *port, PortEnabledState state);

const char *port_get_mac(const Port *port);
const char *port_get_permanent_mac(const Port *port);

IPConfig *port_get_ipconfig(const Port *port);

/**
 * Get the speed of the port in MegaBytes per Second.
 *
 * \param port network port which speed will be returned
 * \retval 0 unable to get network port speed
 * \retval >0 speed of the network port
 */
unsigned int port_get_speed(const Port *port);

bool port_compare(const Port *p1, const Port *p2);

const char *port_get_state_reason(const Port *port);

void port_free(Port *p);

LIST_DECL(Port, port)

Port *ports_find_by_uuid(const Ports *ports, const char *uuid);
Port *ports_find_by_id(const Ports *ports, const char *id);

struct PortStat {
    const Port const *port;
    unsigned long rx_bytes;
    unsigned long rx_packets;
    unsigned long rx_errs;
    unsigned long rx_drop;
    unsigned long rx_fifo;
    unsigned long rx_frame;
    unsigned long rx_compressed;
    unsigned long rx_multicast;
    unsigned long tx_bytes;
    unsigned long tx_packets;
    unsigned long tx_errs;
    unsigned long tx_drop;
    unsigned long tx_fifo;
    unsigned long tx_colls;
    unsigned long tx_carrier;
    unsigned long tx_compressed;
};

PortStat *port_stat_new(void);
void port_stat_free(PortStat *);

LIST_DECL(PortStat, port_stat)

#endif
