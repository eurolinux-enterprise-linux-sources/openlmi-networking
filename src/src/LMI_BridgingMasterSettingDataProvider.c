#include <konkret/konkret.h>
#include "LMI_BridgingMasterSettingData.h"
#include "network.h"
#include "ipassignmentsettingdata.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BridgingMasterSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BridgingMasterSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingMasterSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BridgingMasterSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_BridgingMasterSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_BridgingMasterSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BridgingMasterSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BridgingMasterSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_BridgingMasterSettingDataRef ref;
    if (!KOkay(LMI_BridgingMasterSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_BridgingMasterSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    LMI_BridgingMasterSettingData w;
    LMI_BridgingMasterSettingData_InitFromInstance(&w, _cb, ci);

    Network *network = mi->hdl;
    char *id = id_from_instanceid(w.InstanceID.chars, LMI_BridgingMasterSettingData_ClassName);

    network_lock(network);
    const Connections *connections = network_get_connections(network);
    Connection *old_connection = connections_find_by_id(connections, id);
    free(id);
    if (old_connection == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "No such connection");
    }

    Connection *connection = connection_clone(old_connection);

    if (w.Caption.exists && !w.Caption.null) {
        connection_set_name(connection, w.Caption.chars);
    }
    Setting *setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_BRIDGE);
    if (setting == NULL) {
        network_unlock(network);
        connection_free(connection);
        KReturn2(_cb, ERR_FAILED, "Invalid type of the SettingData");
    }
    BridgeSetting *bridge = setting_get_bridge_setting(setting);
    if (w.InterfaceName.exists && !w.InterfaceName.null) {
        free(bridge->interface_name);
        if ((bridge->interface_name = strdup(w.InterfaceName.chars)) == NULL) {
            connection_free(connection);
            network_unlock(network);
            KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
        }
    }
    if (w.STP.exists && !w.STP.null) {
        bridge->stp = w.STP.value;
    }
    if (w.Priority.exists && !w.Priority.null) {
        bridge->priority = w.Priority.value;
    }
    if (w.ForwardDelay.exists && !w.ForwardDelay.null) {
        bridge->forward_delay = w.ForwardDelay.value;
    }
    if (w.HelloTime.exists && !w.HelloTime.null) {
        bridge->hello_time = w.HelloTime.value;
    }
    if (w.MaxAge.exists && !w.MaxAge.null) {
        bridge->max_age = w.MaxAge.value;
    }
    if (w.AgeingTime.exists && !w.AgeingTime.null) {
        bridge->ageing_time = w.AgeingTime.value;
    }

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingMasterSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    Network *network = mi->hdl;

    LMI_BridgingMasterSettingData w;
    if (LMI_BridgingMasterSettingData_InitFromObjectPath(&w, _cb, cop).rc != 0) {
        warn("Unable to convert object path to " LMI_BridgingMasterSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    return IPAssignmentSettingDataDeleteInstance(network, w.InstanceID.chars);
}

static CMPIStatus LMI_BridgingMasterSettingDataExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

CMInstanceMIStub(
    LMI_BridgingMasterSettingData,
    LMI_BridgingMasterSettingData,
    _cb,
    LMI_BridgingMasterSettingDataInitialize(&mi, ctx))

static void LMI_BridgingMasterSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BridgingMasterSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingMasterSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_BridgingMasterSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_BridgingMasterSettingData,
    LMI_BridgingMasterSettingData,
    _cb,
    LMI_BridgingMasterSettingDataMethodInitialize(&mi, ctx))

KUint32 LMI_BridgingMasterSettingData_LMI_AddStaticIPRoute(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BridgingMasterSettingDataRef* self,
    const KUint16* AddressType,
    const KString* DestinationAddress,
    const KString* DestinationMask,
    const KUint8* PrefixLength,
    KRef* Route,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_BridgingMasterSettingData",
    "LMI_BridgingMasterSettingData",
    "instance method")
