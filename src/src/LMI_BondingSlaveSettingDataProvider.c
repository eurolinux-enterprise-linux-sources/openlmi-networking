#include <konkret/konkret.h>
#include "LMI_BondingSlaveSettingData.h"
#include "network.h"
#include "ipassignmentsettingdata.h"
#include "connection.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BondingSlaveSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BondingSlaveSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingSlaveSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BondingSlaveSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_BondingSlaveSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_BondingSlaveSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BondingSlaveSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BondingSlaveSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_BondingSlaveSettingDataRef ref;
    if (!KOkay(LMI_BondingSlaveSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_BondingSlaveSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    LMI_BondingSlaveSettingData w;
    LMI_BondingSlaveSettingData_InitFromInstance(&w, _cb, ci);

    Network *network = mi->hdl;
    char *id = id_from_instanceid(w.InstanceID.chars, LMI_BondingSlaveSettingData_ClassName);

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

    connection_set_master_connection(connection, connection_get_master_connection(old_connection), SETTING_TYPE_BOND);

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingSlaveSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BondingSlaveSettingDataExecQuery(
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
    LMI_BondingSlaveSettingData,
    LMI_BondingSlaveSettingData,
    _cb,
    LMI_BondingSlaveSettingDataInitialize(&mi, ctx))

static void LMI_BondingSlaveSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BondingSlaveSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingSlaveSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_BondingSlaveSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_BondingSlaveSettingData,
    LMI_BondingSlaveSettingData,
    _cb,
    LMI_BondingSlaveSettingDataMethodInitialize(&mi, ctx))

KUint32 LMI_BondingSlaveSettingData_LMI_AddStaticIPRoute(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BondingSlaveSettingDataRef* self,
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
    "LMI_BondingSlaveSettingData",
    "LMI_BondingSlaveSettingData",
    "instance method")
