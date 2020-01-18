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

#ifndef CONNECTION_PRIVATE_H
#define CONNECTION_PRIVATE_H

#include "globals.h"

struct Connection {
    Network *network;
    /** Backend specific unique identification */
    char *uuid;
    /** Pointer to backend specific private data */
    void *priv;
    char *id;
    char *name;
    ConnectionType type;
    bool autoconnect;
    Settings *settings;
    Port *port;
    char *master_id;
    char *slave_type;
};

LMIResult connection_priv_update(const Connection *connection, Connection *new_connection);

LMIResult connection_priv_delete(const Connection *connection);

void *connection_priv_new(void);
void connection_priv_free(void *priv);

#endif
