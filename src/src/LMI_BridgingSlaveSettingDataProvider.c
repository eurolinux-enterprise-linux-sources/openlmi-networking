#include <konkret/konkret.h>
#include "LMI_BridgingSlaveSettingData.h"
#include "network.h"
#include "ipassignmentsettingdata.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BridgingSlaveSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BridgingSlaveSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingSlaveSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BridgingSlaveSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_BridgingSlaveSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_BridgingSlaveSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BridgingSlaveSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BridgingSlaveSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_BridgingSlaveSettingDataRef ref;
    if (!KOkay(LMI_BridgingSlaveSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_BridgingSlaveSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    LMI_BridgingSlaveSettingData w;
    LMI_BridgingSlaveSettingData_InitFromInstance(&w, _cb, ci);

    Network *network = mi->hdl;
    char *id = id_from_instanceid(w.InstanceID.chars, LMI_BridgingSlaveSettingData_ClassName);

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

    connection_set_master_connection(connection, connection_get_master_connection(old_connection), SETTING_TYPE_BRIDGE);

    Setting *setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_BRIDGE_SLAVE);
    if (setting == NULL) {
        setting = setting_new(SETTING_TYPE_BRIDGE_SLAVE);
        if (setting == NULL || connection_add_setting(connection, setting) != LMI_SUCCESS) {
            connection_free(connection);
            network_unlock(network);
            KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
        }
    }
    BridgeSlaveSetting *bridge = setting_get_bridge_slave_setting(setting);
    if (w.Priority.exists && !w.Priority.null) {
        bridge->priority = w.Priority.value;
    }
    if (w.PathCost.exists && !w.PathCost.null) {
        bridge->path_cost = w.PathCost.value;
    }
    if (w.HairpinMode.exists && !w.HairpinMode.null) {
        bridge->hairpin_mode = w.HairpinMode.value;
    }

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingSlaveSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    Network *network = mi->hdl;

    LMI_BridgingSlaveSettingData w;
    if (LMI_BridgingSlaveSettingData_InitFromObjectPath(&w, _cb, cop).rc != 0) {
        warn("Unable to convert object path to " LMI_BridgingSlaveSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    return IPAssignmentSettingDataDeleteInstance(network, w.InstanceID.chars);
}

static CMPIStatus LMI_BridgingSlaveSettingDataExecQuery(
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
    LMI_BridgingSlaveSettingData,
    LMI_BridgingSlaveSettingData,
    _cb,
    LMI_BridgingSlaveSettingDataInitialize(&mi, ctx))

static void LMI_BridgingSlaveSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BridgingSlaveSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BridgingSlaveSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_BridgingSlaveSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_BridgingSlaveSettingData,
    LMI_BridgingSlaveSettingData,
    _cb,
    LMI_BridgingSlaveSettingDataMethodInitialize(&mi, ctx))

KUint32 LMI_BridgingSlaveSettingData_LMI_AddStaticIPRoute(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BridgingSlaveSettingDataRef* self,
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
    "LMI_BridgingSlaveSettingData",
    "LMI_BridgingSlaveSettingData",
    "instance method")
