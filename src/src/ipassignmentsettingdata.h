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

#ifndef IPASSIGNMENTSETTINGDATA_H
#define IPASSIGNMENTSETTINGDATA_H

#include <konkret.h>
#include "network.h"
#include "ipconfig.h"
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_ExtendedStaticIPAssignmentSettingData.h"
#include "LMI_DHCPSettingData.h"

CMPIStatus connection_to_IPAssignmentSettingData(
    const Connection *connection,
    LMI_IPAssignmentSettingData *w);

CMPIStatus setting_to_ExtendedStaticIPAssignmentSettingData(
    const Setting *setting,
    LMI_ExtendedStaticIPAssignmentSettingData *w);

CMPIStatus setting_to_DHCPSettingData(
    const Setting *setting,
    LMI_DHCPSettingData *w);

CMPIStatus setting_to_IPAssignmentSettingData(
    const Setting *setting,
    LMI_IPAssignmentSettingData *w);

CMPIStatus route_to_IPRouteSettingData(
    const Route *route,
    const char *setting_id,
    size_t route_nr,
    LMI_IPRouteSettingData *w);

typedef enum {
    LMI_IPAssignmentSettingData_Type,
    LMI_ExtendedStaticIPAssignmentSettingData_Type,
    LMI_DHCPSettingData_Type,
    LMI_IPRouteSettingData_Type,
    LMI_BridgingMasterSettingData_Type,
    LMI_BridgingSlaveSettingData_Type,
    LMI_BondingMasterSettingData_Type,
    LMI_BondingSlaveSettingData_Type
} IPAssignmentSettingDataTypes;

CMPIStatus IPAssignmentSettingDataEnumInstances(
    IPAssignmentSettingDataTypes type,
    Network *network,
    const CMPIResult* cr,
    const CMPIBroker *cb,
    const char *ns);

CMPIStatus IPAssignmentSettingDataDeleteInstance(
    Network *network,
    const char *id);

#endif
