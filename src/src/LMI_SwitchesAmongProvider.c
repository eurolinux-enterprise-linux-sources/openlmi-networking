#include <konkret/konkret.h>
#include "LMI_SwitchesAmong.h"
#include <LMI_SwitchService.h>
#include <LMI_SwitchPort.h>
#include "network.h"
#include "port.h"

static const CMPIBroker* _cb;

static void LMI_SwitchesAmongInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_SwitchesAmongCleanup( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SwitchesAmongEnumInstanceNames( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_SwitchesAmongEnumInstances( 
    CMPIInstanceMI* mi,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    LMI_SwitchesAmong w;
    LMI_SwitchesAmong_Init(&w, _cb, ns);

    LMI_SwitchServiceRef dependent;
    LMI_SwitchServiceRef_Init(&dependent, _cb, ns);
    LMI_SwitchServiceRef_Set_CreationClassName(&dependent, LMI_SwitchService_ClassName);
    LMI_SwitchServiceRef_Set_SystemCreationClassName(&dependent, get_system_creation_class_name());
    LMI_SwitchServiceRef_Set_SystemName(&dependent, get_system_name());

    LMI_SwitchPortRef antecedent;
    LMI_SwitchPortRef_Init(&antecedent, _cb, ns);
    LMI_SwitchPortRef_Set_CreationClassName(&antecedent, LMI_SwitchPort_ClassName);
    LMI_SwitchPortRef_Set_SystemCreationClassName(&antecedent, get_system_creation_class_name());
    LMI_SwitchPortRef_Set_SystemName(&antecedent, get_system_name());

    Port *port, *slave;
    network_lock(network);
    const Ports *ports = network_get_ports(network);
    Ports *slaves;
    size_t j;
    for (size_t i = 0; i < ports_length(ports); ++i) {
        if (!KOkay(res)) {
            break;
        }
        port = ports_index(ports, i);
        if (port_get_type(port) != TYPE_BRIDGE) {
            continue;
        }

        LMI_SwitchServiceRef_Set_Name(&dependent, port_get_id(port));

        slaves = port_get_slaves(network, port);
        for (j = 0; j < ports_length(slaves); ++j) {
            slave = ports_index(slaves, j);

            LMI_SwitchPortRef_Set_Name(&antecedent, port_get_id(slave));

            LMI_SwitchesAmong_Set_Antecedent(&w, &antecedent);
            LMI_SwitchesAmong_Set_Dependent(&w, &dependent);

            if (!ReturnInstance(cr, w)) {
                error("Unable to return instance of class " LMI_SwitchPort_ClassName);
                CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                break;
            }
        }
        ports_free(slaves, false);
    }
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_SwitchesAmongGetInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc,
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char** properties) 
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_SwitchesAmongCreateInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const CMPIInstance* ci) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SwitchesAmongModifyInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop,
    const CMPIInstance* ci, 
    const char**properties) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SwitchesAmongDeleteInstance( 
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_SwitchesAmongExecQuery(
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop, 
    const char* lang, 
    const char* query) 
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_SwitchesAmongAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_SwitchesAmongAssociationCleanup( 
    CMPIAssociationMI* mi,
    const CMPIContext* cc, 
    CMPIBoolean term) 
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_SwitchesAmongAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    return KDefaultAssociators(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SwitchesAmong_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_SwitchesAmongAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return KDefaultAssociatorNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SwitchesAmong_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_SwitchesAmongReferences(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return KDefaultReferences(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SwitchesAmong_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_SwitchesAmongReferenceNames(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* assocClass,
    const char* role)
{
    return KDefaultReferenceNames(
        _cb,
        mi,
        cc,
        cr,
        cop,
        LMI_SwitchesAmong_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub( 
    LMI_SwitchesAmong,
    LMI_SwitchesAmong,
    _cb,
    LMI_SwitchesAmongInitialize(&mi, ctx))

CMAssociationMIStub( 
    LMI_SwitchesAmong,
    LMI_SwitchesAmong,
    _cb,
    LMI_SwitchesAmongAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_SwitchesAmong",
    "LMI_SwitchesAmong",
    "instance association")
