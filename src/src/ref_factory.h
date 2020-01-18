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

#ifndef REF_FACTORY_H
#define REF_FACTORY_H

#include <cmpidt.h>
#include "setting.h"

/**
 * Reference Factory creates CMPIObjectPath that points to instance of CMPI object.
 *
 * Use this to create CMPI Associations.
 */

#define CIMReferenceOPDecl(classname) \
CMPIObjectPath *classname##RefOP(const char *name, const char *creationClassName, const CMPIBroker *cb, const CMPIContext *cc, const char *ns);

CIMReferenceOPDecl(CIM_IPAssignmentSettingData)
CIMReferenceOPDecl(CIM_IPVersionSettingData)
CIMReferenceOPDecl(CIM_IPNetworkConnection)
CIMReferenceOPDecl(LMI_IPNetworkConnection)
CIMReferenceOPDecl(CIM_ProtocolEndpoint)
CIMReferenceOPDecl(CIM_Service)
CIMReferenceOPDecl(CIM_LogicalDevice)
CIMReferenceOPDecl(CIM_NetworkPort)
CIMReferenceOPDecl(CIM_ServiceAccessPoint)
CIMReferenceOPDecl(LMI_LAGPort8023ad)
CIMReferenceOPDecl(LMI_LinkAggregator8023ad)
CIMReferenceOPDecl(LMI_LANEndpoint)
CIMReferenceOPDecl(CIM_LANEndpoint)

CMPIObjectPath *CIM_IPConfigurationServiceRefOP(const CMPIBroker *cb, const CMPIContext *cc, const char *ns);

CMPIObjectPath *settingToLMI_IPAssignmentSettingDataSubclassOP(const Setting *setting, const CMPIBroker *cb, const CMPIContext *cc, const char *ns);

#endif
