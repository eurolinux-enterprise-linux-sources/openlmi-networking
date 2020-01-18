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
#include "errors.h"
#include "network.h"

#include <string.h>
#include <net/if.h>
#include <errno.h>

#define PORT_SYS_FLAGS "/sys/class/net/%s/flags"

static int port_read_flags(const char *port_name)
{
    char *flagpath;
    int flags = 0x0;
    if (asprintf(&flagpath, PORT_SYS_FLAGS, port_name) < 0) {
        error("Memory allocation failed");
        return -1;
    }
    FILE *f = fopen(flagpath, "r");
    if (f == NULL) {
        error("Unable to open %s: %s", flagpath, strerror(errno));
        return -1;
    }
    if (fscanf(f, "%i", &flags) != 1) {
        fclose(f);
        error("Unable to get the flags for device %s from file %s",
              port_name, flagpath);
        free(flagpath);
        return -2;
    }
    free(flagpath);
    fclose(f);
    return flags;
}

Port *port_new(void)
{
    Port *p = malloc(sizeof(Port));
    if (p == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    p->id = NULL;
    p->uuid = NULL;
    p->mac = NULL;
    p->permmac = NULL;
    p->priv = NULL;
    p->requestedState = STATE_NO_CHANGE;
    p->ipconfig = ipconfig_new();
    if (p->ipconfig == NULL) {
        free(p);
        return NULL;
    }
    return p;
}

void *port_get_priv(const Port *port)
{
    return port->priv;
}

const char *port_get_id(const Port *port)
{
    return port->id;
}

const char *port_get_uuid(const Port *port)
{
    return port->uuid;
}

PortType port_get_type(const Port *port)
{
    return port->type;
}

PortEnabledState port_get_enabled_state(const Port *port)
{
    if (lmi_testing) {
        if (port->requestedState == STATE_NO_CHANGE)
            return STATE_ENABLED;
        return port->requestedState;
    }

    int flags = port_read_flags(port_get_id(port));
    if (flags < 0) {
        return STATE_ENABLED_BUT_OFFLINE;
    }
    if (flags & IFF_UP) {
        return STATE_ENABLED;
    } else {
        return STATE_DISABLED;
    }
}

PortEnabledState port_get_requested_state(const Port *port)
{
    return port->requestedState;
}

PortOperatingStatus port_get_operating_status(const Port *port)
{
    return port->operatingStatus;
}

Ports *port_get_slaves(Network *network, const Port *port)
{
    return port_priv_get_slaves(network, port);
}

int port_disconnect(Port *port)
{
    return port_priv_disconnect(port);
}

LMIResult port_request_state(Port *port, PortEnabledState state)
{
    if (lmi_testing) {
        port->requestedState = state;
        return LMI_SUCCESS;
    }
    int flags = port_read_flags(port_get_id(port));
    if (flags < 0) {
        // Unable to read current flags, assume 0
        flags = 0;
    }
    switch (state) {
        case STATE_DISABLED:
            flags ^= IFF_UP;
            break;
        case STATE_ENABLED:
            flags |= IFF_UP;
            break;
        default:
            return LMI_WRONG_PARAMETER;
    }
    port->requestedState = state;

    char *flagpath;
    if (asprintf(&flagpath, PORT_SYS_FLAGS, port_get_id(port)) < 0) {
        error("Memory allocation failed");
        return LMI_ERROR_PORT_STATE_CHANGE_FAILED;
    }
    FILE *f = fopen(flagpath, "w");
    if (f == NULL) {
        error("Unable to open %s: %s", flagpath, strerror(errno));
        free(flagpath);
        return LMI_ERROR_PORT_STATE_CHANGE_FAILED;
    }
    fprintf(f, "0x%x", flags);
    fclose(f);
    free(flagpath);
    return LMI_SUCCESS;
}

const char *port_get_mac(const Port *port)
{
    return port->mac;
}

const char *port_get_permanent_mac(const Port *port)
{
    return port->permmac;
}

IPConfig *port_get_ipconfig(const Port *port)
{
    return port->ipconfig;
}

unsigned int port_get_speed(const Port *port)
{
    if (port->type == TYPE_ETHERNET) {
        return port->typespec.ethernet.speed;
    }
    return 0;
}

bool port_compare(const Port *p1, const Port *p2)
{
    if (p1 == NULL || p2 == NULL) {
        return false;
    }
    if (p1->id == NULL || p2->id == NULL) {
        return false;
    }
    return strcmp(p1->id, p2->id) == 0;
}

void port_free(Port *p)
{
    if (p == NULL) {
        return;
    }
    free(p->uuid);
    free(p->id);
    free(p->mac);
    ipconfig_free(p->ipconfig);
    port_priv_free(p->priv);
    free(p);
}

LIST_IMPL(Port, port)

Port *ports_find_by_uuid(const Ports *ports, const char *uuid)
{
    if (uuid == NULL) {
        return NULL;
    }
    Port *port;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        if (port->uuid == NULL) {
            continue;
        }
        if (port->uuid != NULL && strcmp(port->uuid, uuid) == 0) {
            return port;
        }
    }
    return NULL;
}

Port *ports_find_by_id(const Ports *ports, const char *id)
{
    if (id == NULL) {
        return NULL;
    }
    Port *port;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        port = ports_index(ports, i);
        if (port->id == NULL) {
            continue;
        }
        if (strcmp(port->id, id) == 0) {
            return port;
        }
    }
    return NULL;
}

LIST_IMPL(PortStat, port_stat)

PortStat *port_stat_new(void)
{
    PortStat *stat = malloc(sizeof(PortStat));
    if (stat == NULL) {
        error("Memory allocation failed");
        return NULL;
    }
    stat->port = NULL;
    return stat;
}

void port_stat_free(PortStat *stat)
{
    free(stat);
}

const char *port_get_state_reason(const Port *port)
{
    return port_priv_get_state_reason(port);
}
