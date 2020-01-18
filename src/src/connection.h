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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "globals.h"

/**
 * Create new \p Connection
 *
 * \param network pointer to Network
 * \param id unique identificator
 * \param name human-readable name
 * \return \p Connection
 */
Connection *connection_new(Network *network, const char *id, const char *name);

/**
 * Clone connection
 *
 * \param connection Connection to be cloned
 * \return \p Connection cloned connection
 */
Connection *connection_clone(Connection *c);

/**
 * Update the \p Connection
 *
 * \param connection connection to update
 * \param new_connection how updated connection should look like
 * @return LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult connection_update(const Connection *connection, Connection *new_connection);

/**
 * Gets unique identificator of the connection
 *
 * \param connection
 * \return uuid of the connection
 */
const char *connection_get_uuid(const Connection *connection);

/**
 * Gets unique identificator of the connection.
 *
 * \param connection
 * \return id of the connection
 **/
const char *connection_get_id(const Connection *connection);

/**
 * Gets human-readable name of the connection
 *
 * @param connection
 * @return name of the connection
 **/
const char *connection_get_name(const Connection *connection);

/**
 * Sets the human-readable connection name
 * @param connection
 * @param name new name of the connection
 * @return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult connection_set_name(Connection *connection, const char *name);

/**
 * Check if the connection happens automatically.
 *
 * @param connection
 * @retval true if the connection happens automatically
 * @retval false manually-activate connection
 **/
bool connection_get_autoconnect(const Connection *connection);

/**
 * Set whether the connection should be activated automatically.
 *
 * @param connection
 * @param autoconnect true if connection will be autoconnected
 * @return \p LMIResult with LMI_SUCCESS or error code on failure
 **/
LMIResult connection_set_autoconnect(const Connection *connection, bool autoconnect);

/**
 * List of settings that belongs to the connection.
 *
 * \param connection
 * \return \p Settings (don't free).
 */
const Settings *connection_get_settings(const Connection *connection);

/**
 * Gets the port to which the connection belongs.
 *
 * \param connection
 * \return \p Port (don't free).
 */
const Port *connection_get_port(const Connection *connection);

/**
 * Gets type of the connection
 *
 * \param connection
 * \return \p ConnectionType
 */
ConnectionType connection_get_type(const Connection *connection);

/**
 * Sets type of the connection
 *
 * \param connection
 * \param type
 * @return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult connection_set_type(Connection *connection, ConnectionType type);

/**
 * Sets master connection for given connection
 *
 * \param connection
 * \param master Master for given connection
 * \param type Type of the slavery - SETTING_TYPE_BRIDGE or SETTING_TYPE_BOND
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult connection_set_master_connection(Connection *connection, const Connection *master, SettingType type);

/**
 * Gets master connection for given slave connection
 *
 * \param connection slave connection
 * \return \p Connection master connection or NULL if the slave doesn't
 *     have master connection
 */
Connection *connection_get_master_connection(const Connection *connection);

/**
 * Add new setting to the list of the settings.
 *
 * Setting will be freed when connection is freed, do not free it.
 *
 * \param connection \p Connection
 * \param setting \p Setting
 * \return \p LMIResult with LMI_SUCCESS or error code on failure
 */
LMIResult connection_add_setting(Connection *connection, Setting *setting);

/**
 * Make connection activable only on specific port.
 *
 * \param connection \p Connection
 * \param port \p Port
 * \retval 0 on success
 */
LMIResult connection_set_port(Connection *connection, Port *port);

/**
 * Checks if two connections are the same.
 *
 * Checking is based on comparing of IDs.
 *
 * \param c1 \p Connection
 * \param c2 \p Connection
 * \retval true if both connections are the same
 * \retval false otherwise
 */
bool connection_compare(const Connection *c1, const Connection *c2);

/**
 * Delete connection from the system
 *
 * \param connection \p Connection
 * \retval LMI_SUCCESS deletion successfull
 * \retval LMI_ERROR_CONNECTION_DELETE_FAILED deletion failed
 */
LMIResult connection_delete(const Connection *connection);

/**
 * Frees connection with all the settings.
 * Doesn't free the port.
 *
 * \param connection \p Connection
 */
void connection_free(Connection *connection);

LIST_DECL(Connection, connection)

/**
 * Find connection by given uuid
 *
 * \param connections list of \p Connection
 * \param uuid UUID to find
 * \return \p Connection
 */
Connection *connections_find_by_uuid(const Connections *connections, const char *uuid);

/**
 * Find connection by given id
 *
 * \param connections list of \p Connection
 * \param uuid id to find
 * \return \p Connection
 */
Connection *connections_find_by_id(const Connections *connections, const char *id);


#endif
