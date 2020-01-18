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

#include "ref_factory.h"
#include "globals.h"
#include "setting.h"

#include "CIM_ComputerSystem.h"
#include "LMI_IPNetworkConnection.h"
#include "CIM_LANEndpoint.h"
#include "CIM_ProtocolEndpoint.h"
#include "CIM_LogicalDevice.h"
#include "CIM_NetworkPort.h"
#include "CIM_ServiceAccessPoint.h"
#include "CIM_Service.h"
#include "LMI_ExtendedStaticIPAssignmentSettingData.h"
#include "LMI_DHCPSettingData.h"
#include "LMI_IPConfigurationService.h"
#include "LMI_IPAssignmentSettingData.h"
#include "LMI_LAGPort8023ad.h"
#include "LMI_LANEndpoint.h"
#include "LMI_LinkAggregator8023ad.h"

#define CIMReferenceOPImpl_Name(classname) \
CMPIObjectPath *classname##RefOP(const char *name, const char *creationClassName, const CMPIBroker *cb, const CMPIContext *cc, const char *ns) \
{ \
    classname##Ref ref; \
    classname##Ref_Init(&ref, cb, ns); \
    classname##Ref_Set_CreationClassName(&ref, creationClassName); \
    classname##Ref_Set_Name(&ref, name); \
    classname##Ref_Set_SystemCreationClassName(&ref, get_system_creation_class_name()); \
    classname##Ref_Set_SystemName(&ref, lmi_get_system_name_safe(cc)); \
    CMPIObjectPath *op = classname##Ref_ToObjectPath(&ref, NULL); \
    op->ft->setClassName(op, creationClassName); \
    return op; \
}

#define CIMReferenceOPImpl_DeviceID(classname) \
CMPIObjectPath *classname##RefOP(const char *name, const char *creationClassName, const CMPIBroker *cb, const CMPIContext *cc, const char *ns) \
{ \
    classname##Ref ref; \
    classname##Ref_Init(&ref, cb, ns); \
    classname##Ref_Set_CreationClassName(&ref, creationClassName); \
    classname##Ref_Set_DeviceID(&ref, name); \
    classname##Ref_Set_SystemCreationClassName(&ref, get_system_creation_class_name()); \
    classname##Ref_Set_SystemName(&ref, lmi_get_system_name_safe(cc)); \
    CMPIObjectPath *op = classname##Ref_ToObjectPath(&ref, NULL); \
    op->ft->setClassName(op, creationClassName); \
    return op; \
}

#define CIMReferenceOPImpl_InstanceID(classname) \
CMPIObjectPath *classname##RefOP(const char *name, const char *creationClassName, const CMPIBroker *cb, const CMPIContext *cc, const char *ns) \
{ \
    classname##Ref ref; \
    classname##Ref_Init(&ref, cb, ns); \
    classname##Ref_Set_InstanceID(&ref, name); \
    CMPIObjectPath *op = classname##Ref_ToObjectPath(&ref, NULL); \
    op->ft->setClassName(op, creationClassName); \
    return op; \
}

CIMReferenceOPImpl_Name(CIM_ServiceAccessPoint)
CIMReferenceOPImpl_Name(CIM_IPNetworkConnection)
CIMReferenceOPImpl_Name(LMI_IPNetworkConnection)
CIMReferenceOPImpl_Name(LMI_LAGPort8023ad)
CIMReferenceOPImpl_Name(LMI_LinkAggregator8023ad)
CIMReferenceOPImpl_Name(LMI_LANEndpoint)
CIMReferenceOPImpl_Name(CIM_LANEndpoint)
CIMReferenceOPImpl_Name(CIM_ProtocolEndpoint)
CIMReferenceOPImpl_Name(CIM_Service)
CIMReferenceOPImpl_DeviceID(CIM_NetworkPort)
CIMReferenceOPImpl_DeviceID(CIM_LogicalDevice)
CIMReferenceOPImpl_InstanceID(CIM_IPVersionSettingData)
CIMReferenceOPImpl_InstanceID(CIM_IPAssignmentSettingData)

CMPIObjectPath *CIM_IPConfigurationServiceRefOP(const CMPIBroker *cb, const CMPIContext *cc, const char *ns)
{
    LMI_IPConfigurationServiceRef ipConfigurationServiceRef;
    LMI_IPConfigurationServiceRef_Init(&ipConfigurationServiceRef, cb, ns);
    LMI_IPConfigurationServiceRef_Set_CreationClassName(&ipConfigurationServiceRef, LMI_IPConfigurationService_ClassName);
    LMI_IPConfigurationServiceRef_Set_Name(&ipConfigurationServiceRef, LMI_IPConfigurationService_ClassName);
    LMI_IPConfigurationServiceRef_Set_SystemCreationClassName(&ipConfigurationServiceRef, get_system_creation_class_name());
    LMI_IPConfigurationServiceRef_Set_SystemName(&ipConfigurationServiceRef, lmi_get_system_name_safe(cc));

    CMPIStatus rc;
    return LMI_IPConfigurationServiceRef_ToObjectPath(&ipConfigurationServiceRef, &rc);
}

CMPIObjectPath *settingToLMI_IPAssignmentSettingDataSubclassOP(const Setting *setting, const CMPIBroker *cb, const CMPIContext *cc, const char *ns)
{
    LMI_IPAssignmentSettingDataRef ipAssignmentSettingDataRef;
    LMI_IPAssignmentSettingDataRef_Init(&ipAssignmentSettingDataRef, cb, ns);
    const char *className = NULL;
    if (setting_get_type(setting) == SETTING_TYPE_IPv4 || setting_get_type(setting) == SETTING_TYPE_IPv6) {
        switch (setting_get_method(setting)) {
            case SETTING_METHOD_STATIC:
                className = LMI_ExtendedStaticIPAssignmentSettingData_ClassName;
                break;
            case SETTING_METHOD_LINK_LOCAL:
                className = LMI_ExtendedStaticIPAssignmentSettingData_ClassName;
                break;
            case SETTING_METHOD_DHCP:
            case SETTING_METHOD_DHCPv6:
                className = LMI_DHCPSettingData_ClassName;
                break;
            case SETTING_METHOD_STATELESS:
                className = LMI_IPAssignmentSettingData_ClassName;
                break;
            default:
                warn("Unknown setting (%s) method: %d", setting_get_id(setting), setting_get_method(setting));
                return NULL;
                break;
        }
    } else {
        className = LMI_DHCPSettingData_ClassName;
    }

    CMPIStatus rc;
    char *instanceid = id_to_instanceid(setting_get_id(setting), className);
    LMI_IPAssignmentSettingDataRef_Set_InstanceID(&ipAssignmentSettingDataRef, instanceid);
    free(instanceid);
    CMPIObjectPath *ipAssignmentSettingDataOP = LMI_IPAssignmentSettingDataRef_ToObjectPath(&ipAssignmentSettingDataRef, &rc);
    ipAssignmentSettingDataOP->ft->setClassName(ipAssignmentSettingDataOP, className);
    return ipAssignmentSettingDataOP;
}
