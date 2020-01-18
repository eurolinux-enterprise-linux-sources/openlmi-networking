#include <konkret/konkret.h>
#include "LMI_BondingMasterSettingData.h"
#include "network.h"
#include "ipassignmentsettingdata.h"
#include "connection.h"
#include "setting.h"

static const CMPIBroker* _cb = NULL;

static void LMI_BondingMasterSettingDataInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BondingMasterSettingDataCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingMasterSettingDataEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_BondingMasterSettingDataEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    return IPAssignmentSettingDataEnumInstances(
            LMI_BondingMasterSettingData_Type,
            network,
            cr, _cb, ns);
}

static CMPIStatus LMI_BondingMasterSettingDataGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_BondingMasterSettingDataCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_BondingMasterSettingDataModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    LMI_BondingMasterSettingDataRef ref;
    if (!KOkay(LMI_BondingMasterSettingDataRef_InitFromObjectPath(&ref, _cb, cop))) {
        warn("Unable to convert object path to " LMI_BondingMasterSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    LMI_BondingMasterSettingData w;
    LMI_BondingMasterSettingData_InitFromInstance(&w, _cb, ci);

    Network *network = mi->hdl;
    char *id = id_from_instanceid(w.InstanceID.chars, LMI_BondingMasterSettingData_ClassName);

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
    Setting *setting = settings_find_by_type(connection_get_settings(connection), SETTING_TYPE_BOND);
    if (setting == NULL) {
        connection_free(connection);
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "Invalid type of the SettingData");
    }
    BondSetting *bond = setting_get_bond_setting(setting);
    if (w.Mode.exists && !w.Mode.null) {
        if (w.Mode.value > 6) {
            connection_free(connection);
            network_unlock(network);
            KReturn2(_cb, ERR_INVALID_PARAMETER, "Invalid value of Mode parameter");
        }
        bond->mode = w.Mode.value;
    }
    if (w.MIIMon.exists && !w.MIIMon.null) {
        bond->miimon = w.MIIMon.value;
    }
    if (w.DownDelay.exists && !w.DownDelay.null) {
        bond->downdelay = w.DownDelay.value;
    }
    if (w.UpDelay.exists && !w.UpDelay.null) {
        bond->updelay = w.UpDelay.value;
    }
    if (w.ARPInterval.exists && !w.ARPInterval.null) {
        bond->arp_interval = w.ARPInterval.value;
    }
    if (w.ARPIPTarget.exists && !w.ARPIPTarget.null) {
        char *addr;
        bond->arp_ip_target = ip_addresses_new(w.ARPIPTarget.count);
        for (CMPIUint32 i = 0; i < w.ARPIPTarget.count; ++i) {
            if ((addr = strdup(KStringA_Get(&w.ARPIPTarget, i))) == NULL) {
                connection_free(connection);
                network_unlock(network);
                KReturn2(_cb, ERR_FAILED, "Memory allocation failed");
            }
            ip_addresses_add(bond->arp_ip_target, addr);
        }
    }

    int rc = connection_update(old_connection, connection);
    connection_free(connection);
    network_unlock(network);
    if (rc != 0) {
        CMReturn(CMPI_RC_ERR_FAILED);
    }
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingMasterSettingDataDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    Network *network = mi->hdl;

    LMI_BondingMasterSettingData w;
    if (LMI_BondingMasterSettingData_InitFromObjectPath(&w, _cb, cop).rc != 0) {
        warn("Unable to convert object path to " LMI_BondingMasterSettingData_ClassName);
        KReturn(ERR_INVALID_PARAMETER);
    }

    return IPAssignmentSettingDataDeleteInstance(network, w.InstanceID.chars);
}

static CMPIStatus LMI_BondingMasterSettingDataExecQuery(
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
    LMI_BondingMasterSettingData,
    LMI_BondingMasterSettingData,
    _cb,
    LMI_BondingMasterSettingDataInitialize(&mi, ctx))

static void LMI_BondingMasterSettingDataMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_BondingMasterSettingDataMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_BondingMasterSettingDataInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_BondingMasterSettingData_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_BondingMasterSettingData,
    LMI_BondingMasterSettingData,
    _cb,
    LMI_BondingMasterSettingDataMethodInitialize(&mi, ctx))

KUint32 LMI_BondingMasterSettingData_LMI_AddStaticIPRoute(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_BondingMasterSettingDataRef* self,
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
    "LMI_BondingMasterSettingData",
    "LMI_BondingMasterSettingData",
    "instance method")
