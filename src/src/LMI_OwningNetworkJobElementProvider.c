#include <konkret/konkret.h>
#include "LMI_OwningNetworkJobElement.h"
#include <LMI_NetworkJob.h>
#include <LMI_IPConfigurationService.h>
#include "network.h"

static const CMPIBroker* _cb;

static void LMI_OwningNetworkJobElementInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_OwningNetworkJobElementCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_OwningNetworkJobElementEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_OwningNetworkJobElementEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    LMI_IPConfigurationServiceRef confserv;
    LMI_IPConfigurationServiceRef_Init(&confserv, _cb, ns);
    LMI_IPConfigurationServiceRef_Set_SystemName(&confserv, lmi_get_system_name_safe(cc));
    LMI_IPConfigurationServiceRef_Set_SystemCreationClassName(&confserv, get_system_creation_class_name());
    LMI_IPConfigurationServiceRef_Set_CreationClassName(&confserv, LMI_IPConfigurationService_ClassName);
    LMI_IPConfigurationServiceRef_Set_Name(&confserv, LMI_IPConfigurationService_ClassName);

    network_lock(network);
    const Jobs *jobs = network_get_jobs(network);
    Job *job;
    char *id;

    for (size_t i = 0; i < jobs_length(jobs); ++i) {
        if (!KOkay(res)) {
            break;
        }
        job = jobs_index(jobs, i);
        LMI_OwningNetworkJobElement w;
        LMI_OwningNetworkJobElement_Init(&w, _cb, ns);

        LMI_NetworkJobRef network_job;
        LMI_NetworkJobRef_Init(&network_job, _cb, ns);
        id = id_to_instanceid_with_index("Job", LMI_NetworkJob_ClassName, job->id);
        if (id == NULL) {
            error("Memory allocation failed");
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
        LMI_NetworkJobRef_Set_InstanceID(&network_job, id);
        free(id);

        LMI_OwningNetworkJobElement_SetObjectPath_OwningElement(&w, LMI_IPConfigurationServiceRef_ToObjectPath(&confserv, &res));
        LMI_OwningNetworkJobElement_Set_OwnedElement(&w, &network_job);
        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class "
                  LMI_OwningNetworkJobElement_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }

    network_unlock(network);
    return res;
}

static CMPIStatus LMI_OwningNetworkJobElementGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_OwningNetworkJobElementCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OwningNetworkJobElementModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OwningNetworkJobElementDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_OwningNetworkJobElementExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_OwningNetworkJobElementAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_OwningNetworkJobElementAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_OwningNetworkJobElementAssociators(
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
        LMI_OwningNetworkJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_OwningNetworkJobElementAssociatorNames(
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
        LMI_OwningNetworkJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_OwningNetworkJobElementReferences(
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
        LMI_OwningNetworkJobElement_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_OwningNetworkJobElementReferenceNames(
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
        LMI_OwningNetworkJobElement_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_OwningNetworkJobElement,
    LMI_OwningNetworkJobElement,
    _cb,
    LMI_OwningNetworkJobElementInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_OwningNetworkJobElement,
    LMI_OwningNetworkJobElement,
    _cb,
    LMI_OwningNetworkJobElementAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_OwningNetworkJobElement",
    "LMI_OwningNetworkJobElement",
    "instance association")
