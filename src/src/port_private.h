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

#ifndef PORT_PRIVATE_H
#define PORT_PRIVATE_H

#include "port.h"
#include "ipconfig.h"

struct Port {
    /** Backend specific unique identification */
    char *uuid;
    /** Pointer to backend specific private data */
    void *priv;
    char *id; // DeviceID
    PortType type;
    PortEnabledState requestedState;
    PortOperatingStatus operatingStatus;
    char *mac, *permmac;
    IPConfig *ipconfig;
    bool carrier;
    union {
        struct {
            Ports *slaves;
        } bond;
        struct {
            Ports *slaves;
        } bridge;
        struct {
            uint32_t capabilities;
        } bluetooth;
        struct {
            char *perm_hw_address;
            uint32_t speed;
        } ethernet;
        struct {
            uint32_t caps;
            uint32_t current_caps;
        } modem;
        struct {
            Port *companion;
            uint32_t active_channel;
        } olpc_mesh;
        struct {
            int vlan_id;
        } vlan;
        struct {
            char *perm_hw_address;
            int mode;
            uint32_t rate;
            void *active_ap; // TODO: define struct for access point
            int wireless_caps;
            void *aps; // TODO: define struct for list of access points
            bool wireless_enabled;
        } wifi;
        struct {
            void *active_nsp; // TODO: define it
            void *nsps; // TODO: define it
            unsigned int center_freq;
            int rssi;
            int cinr;
            int tx_power;
            char *bsid;
        } wimax;
    } typespec;
};

void port_priv_free(void *priv);

int port_priv_disconnect(Port *port);

Ports *port_priv_get_slaves(Network *network, const Port *port);

const char *port_priv_get_state_reason(const Port *port);

#endif
