#include <konkret/konkret.h>
#include "LMI_AffectedNetworkJobElement.h"
#include <LMI_IPAssignmentSettingData.h>
#include <LMI_IPNetworkConnection.h>
#include "network.h"
#include "activeconnection.h"
#include "connection.h"
#include "port.h"

static const CMPIBroker* _cb;

static void LMI_AffectedNetworkJobElementInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_AffectedNetworkJobElementCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AffectedNetworkJobElementEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_AffectedNetworkJobElementEnumInstances(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    CMPIStatus res = { CMPI_RC_OK, NULL };
    Network *network = mi->hdl;
    const char *ns = KNameSpace(cop);

    network_lock(network);
    const Jobs *jobs = network_get_jobs(network);
    Job *job;
    char *id;
    size_t j;

    LMI_AffectedNetworkJobElement w;
    LMI_AffectedNetworkJobElement_Init(&w, _cb, ns);

    for (size_t i = 0; i < jobs_length(jobs); ++i) {
        if (!KOkay(res)) {
            break;
        }
        job = jobs_index(jobs, i);

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
        LMI_AffectedNetworkJobElement_SetObjectPath_AffectingElement(&w, LMI_NetworkJobRef_ToObjectPath(&network_job, &res));
        if (!KOkay(res)) {
            error("Can't create ObjectPath from " LMI_NetworkJob_ClassName);
            break;
        }

        JobAffectedElement *affected_element;
        CMPIObjectPath *op;
        for (j = 0; j < job_affected_elements_length(job->affected_elements); ++j) {
            affected_element = job_affected_elements_index(job->affected_elements, j);
            op = NULL;

            switch (affected_element->type) {
                case JOB_AFFECTED_CONNECTION_ID: {
                    // Affected IPAssignmentSettingData
                    id = id_to_instanceid(affected_element->id, LMI_IPAssignmentSettingData_ClassName);
                    LMI_IPAssignmentSettingDataRef settingData;
                    LMI_IPAssignmentSettingDataRef_Init(&settingData, _cb, ns);
                    LMI_IPAssignmentSettingDataRef_Set_InstanceID(&settingData, id);
                    free(id);
                    op = LMI_IPAssignmentSettingDataRef_ToObjectPath(&settingData, &res);
                    if (!KOkay(res)) {
                        error("Can't create ObjectPath from " LMI_IPAssignmentSettingData_ClassName);
                        op = NULL;
                    }
                    break;
                }
                case JOB_AFFECTED_PORT_ID: {
                    // Affected IPNetworkConnections
                    LMI_IPNetworkConnectionRef networkConnection;
                    LMI_IPNetworkConnectionRef_Init(&networkConnection, _cb, ns);
                    LMI_IPNetworkConnectionRef_Set_SystemName(&networkConnection, get_system_name());
                    LMI_IPNetworkConnectionRef_Set_CreationClassName(&networkConnection, LMI_IPNetworkConnection_ClassName);
                    LMI_IPNetworkConnectionRef_Set_SystemCreationClassName(&networkConnection, get_system_creation_class_name());
                    LMI_IPNetworkConnectionRef_Set_Name(&networkConnection, affected_element->id);
                    op = LMI_IPNetworkConnectionRef_ToObjectPath(&networkConnection, &res);
                    if (!KOkay(res)) {
                        error("Can't create ObjectPath from " LMI_IPNetworkConnection_ClassName);
                        op = NULL;
                    }
                    break;
                }
                default:
                    break;
            }
            if (op != NULL) {
                LMI_AffectedNetworkJobElement_SetObjectPath_AffectedElement(&w, op);

                if (!ReturnInstance(cr, w)) {
                    error("Unable to return instance of class "
                          LMI_AffectedNetworkJobElement_ClassName);
                    CMSetStatus(&res, CMPI_RC_ERR_FAILED);
                    break;
                }
            }
        }
    }

    network_unlock(network);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AffectedNetworkJobElementGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_AffectedNetworkJobElementCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedNetworkJobElementModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char**properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedNetworkJobElementDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_AffectedNetworkJobElementExecQuery(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* lang,
    const char* query)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static void LMI_AffectedNetworkJobElementAssociationInitialize(
    CMPIAssociationMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}


static CMPIStatus LMI_AffectedNetworkJobElementAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    mi->hdl = NULL;
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_AffectedNetworkJobElementAssociators(
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
        LMI_AffectedNetworkJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole,
        properties);
}

static CMPIStatus LMI_AffectedNetworkJobElementAssociatorNames(
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
        LMI_AffectedNetworkJobElement_ClassName,
        assocClass,
        resultClass,
        role,
        resultRole);
}

static CMPIStatus LMI_AffectedNetworkJobElementReferences(
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
        LMI_AffectedNetworkJobElement_ClassName,
        assocClass,
        role,
        properties);
}

static CMPIStatus LMI_AffectedNetworkJobElementReferenceNames(
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
        LMI_AffectedNetworkJobElement_ClassName,
        assocClass,
        role);
}

CMInstanceMIStub(
    LMI_AffectedNetworkJobElement,
    LMI_AffectedNetworkJobElement,
    _cb,
    LMI_AffectedNetworkJobElementInitialize(&mi, ctx))

CMAssociationMIStub(
    LMI_AffectedNetworkJobElement,
    LMI_AffectedNetworkJobElement,
    _cb,
    LMI_AffectedNetworkJobElementAssociationInitialize(&mi, ctx))

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_AffectedNetworkJobElement",
    "LMI_AffectedNetworkJobElement",
    "instance association")
