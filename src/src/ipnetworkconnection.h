/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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

#ifndef IPNETWORKCONNECTION_H
#define IPNETWORKCONNECTION_H

#include <konkret.h>
#include "network.h"
#include "LMI_IPNetworkConnection.h"
#include "LMI_LANEndpoint.h"
#include "LMI_EthernetPort.h"

CMPIStatus port_to_IPNetworkConnection(
    const Port *port,
    LMI_IPNetworkConnection *w);

CMPIStatus port_to_LANEndpoint(
    const Port *port,
    LMI_LANEndpoint *w);

CMPIStatus port_to_EthernetPort(
    const Port *port,
    LMI_EthernetPort *w);

typedef enum {
    LMI_IPNetworkConnection_Type,
    LMI_LANEndpoint_Type,
    LMI_EthernetPort_Type
} IPNetworkConnectionTypes;

CMPIStatus IPNetworkConnectionEnumInstances(
    IPNetworkConnectionTypes type,
    Network *network,
    const CMPIResult* cr,
    const CMPIBroker *cb,
    const char *ns);

#endif
