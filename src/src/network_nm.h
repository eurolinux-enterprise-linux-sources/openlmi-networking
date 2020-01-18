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

#ifndef NETWORK_NM_H
#define NETWORK_NM_H

#include "network.h"

#include <glib.h>
#include <glib-object.h>
#include <NetworkManager/NetworkManager.h>
// Forward port some of constants from NetworkManager
#ifndef NM_DBUS_INTERFACE_DEVICE_BRIDGE
#define NM_DBUS_INTERFACE_DEVICE_BRIDGE NM_DBUS_INTERFACE_DEVICE ".Bridge"
#endif
#define NM_DEVICE_TYPE_BRIDGE 13

#include <dbus/dbus-glib.h>

int DBUS_BUS;
const char *NM_SERVICE_DBUS;

DBusGConnection *network_priv_get_dbus_connection(Network *network);

#endif
