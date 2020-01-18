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

#ifndef ACTIVE_CONNECTION_PRIVATE_H
#define ACTIVE_CONNECTION_PRIVATE_H

typedef enum {
    ACTIVE_CONNECTION_STATE_UNKNOWN=0,
    ACTIVE_CONNECTION_STATE_ACTIVATING,
    ACTIVE_CONNECTION_STATE_ACTIVATED,
    ACTIVE_CONNECTION_STATE_DEACTIVATING,
    ACTIVE_CONNECTION_STATE_DEACTIVATED
} ActiveConnectionStatus;

struct ActiveConnection
{
    /** Backend specific unique identification */
    char *uuid;
    Connection *connection;
    Ports *ports;
    Network *network;
    ActiveConnectionStatus status;
    /** Pointer to backend specific private data */
    void *priv;
};

void active_connection_priv_free(void *priv);

#endif
