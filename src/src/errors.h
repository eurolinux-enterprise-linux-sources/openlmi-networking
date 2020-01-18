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

#ifndef ERRORS_H
#define ERRORS_H

typedef enum LMIResult {
    LMI_SUCCESS = 0,

    // Global errors
    LMI_ERROR_UNKNOWN = 1,
    LMI_ERROR_TIMEOUT,
    LMI_WRONG_PARAMETER,
    LMI_ERROR_MEMORY, // Memory allocation failed
    LMI_ERROR_BACKEND, // Error in communication with backend
    LMI_ERROR_NOT_IMPLEMENTED, // Requested feature is not implemented

    // Connection errors
    LMI_ERROR_CONNECTION_UNKNOWN = 20,
    LMI_ERROR_CONNECTION_ACTIVATING,
    LMI_ERROR_CONNECTION_INVALID,
    LMI_ERROR_CONNECTION_DELETE_FAILED,
    LMI_ERROR_CONNECTION_UPDATE_FAILED,

    // Port errors
    LMI_ERROR_PORT_UNKNOWN = 40,
    LMI_ERROR_PORT_NO_DEFAULT_CONNECTION,
    LMI_ERROR_PORT_STATE_CHANGE_FAILED,

    // others
    LMI_JOB_STARTED = 4096
} LMIResult;

#endif
