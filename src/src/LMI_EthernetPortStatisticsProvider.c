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

#include <konkret/konkret.h>
#include "LMI_EthernetPortStatistics.h"
#include "network.h"
#include "port.h"
static const CMPIBroker* _cb = NULL;

static void LMI_EthernetPortStatisticsInitialize(
    CMPIInstanceMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_EthernetPortStatisticsCleanup(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EthernetPortStatisticsEnumInstanceNames(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    return KDefaultEnumerateInstanceNames(
        _cb, mi, cc, cr, cop);
}

static CMPIStatus LMI_EthernetPortStatisticsEnumInstances(
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
    LMIResult result = LMI_SUCCESS;
    PortStats *stats = network_get_ports_statistics(network, &result);
    if (stats == NULL) {
        network_unlock(network);
        KReturn2(_cb, ERR_FAILED, "Unable to get port statistics");
    }
    PortStat *stat;
    char *instanceid;
    for (size_t i = 0; i < port_stats_length(stats); ++i) {
        stat = port_stats_index(stats, i);

        LMI_EthernetPortStatistics w;
        LMI_EthernetPortStatistics_Init(&w, _cb, ns);

        instanceid = id_to_instanceid(port_get_id(stat->port), LMI_EthernetPortStatistics_ClassName);
        LMI_EthernetPortStatistics_Set_InstanceID(&w, instanceid);
        free(instanceid);
        LMI_EthernetPortStatistics_Set_ElementName(&w, port_get_id(stat->port));

        LMI_EthernetPortStatistics_Set_BytesReceived(&w, stat->rx_bytes);
        LMI_EthernetPortStatistics_Set_BytesTransmitted(&w, stat->tx_bytes);
        LMI_EthernetPortStatistics_Set_BytesTotal(&w, stat->rx_bytes + stat->tx_bytes);

        LMI_EthernetPortStatistics_Set_PacketsReceived(&w, stat->rx_packets);
        LMI_EthernetPortStatistics_Set_PacketsTransmitted(&w, stat->tx_packets);

        LMI_EthernetPortStatistics_Set_TotalRxErrors(&w, stat->rx_errs);
        LMI_EthernetPortStatistics_Set_TotalTxErrors(&w, stat->tx_errs);
        LMI_EthernetPortStatistics_Set_TotalCollisions(&w, stat->tx_colls);

        if (!ReturnInstance(cr, w)) {
            error("Unable to return instance of class " LMI_EthernetPortStatistics_ClassName);
            CMSetStatus(&res, CMPI_RC_ERR_FAILED);
            break;
        }
    }
    port_stats_free(stats, true);
    network_unlock(network);
    return res;
}

static CMPIStatus LMI_EthernetPortStatisticsGetInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
    return KDefaultGetInstance(
        _cb, mi, cc, cr, cop, properties);
}

static CMPIStatus LMI_EthernetPortStatisticsCreateInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EthernetPortStatisticsModifyInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const CMPIInstance* ci,
    const char** properties)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EthernetPortStatisticsDeleteInstance(
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop)
{
    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}

static CMPIStatus LMI_EthernetPortStatisticsExecQuery(
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
    LMI_EthernetPortStatistics,
    LMI_EthernetPortStatistics,
    _cb,
    LMI_EthernetPortStatisticsInitialize(&mi, ctx))

static void LMI_EthernetPortStatisticsMethodInitialize(
    CMPIMethodMI *mi,
    const CMPIContext *ctx)
{
    mi->hdl = network_ref(_cb, ctx);
}

static CMPIStatus LMI_EthernetPortStatisticsMethodCleanup(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    CMPIBoolean term)
{
    network_unref(mi->hdl);
    CMReturn(CMPI_RC_OK);
}

static CMPIStatus LMI_EthernetPortStatisticsInvokeMethod(
    CMPIMethodMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* meth,
    const CMPIArgs* in,
    CMPIArgs* out)
{
    return LMI_EthernetPortStatistics_DispatchMethod(
        _cb, mi, cc, cr, cop, meth, in, out);
}

CMMethodMIStub(
    LMI_EthernetPortStatistics,
    LMI_EthernetPortStatistics,
    _cb,
    LMI_EthernetPortStatisticsMethodInitialize(&mi, ctx))

KUint32 LMI_EthernetPortStatistics_ResetSelectedStats(
    const CMPIBroker* cb,
    CMPIMethodMI* mi,
    const CMPIContext* context,
    const LMI_EthernetPortStatisticsRef* self,
    const KStringA* SelectedStatistics,
    CMPIStatus* status)
{
    KUint32 result = KUINT32_INIT;

    KSetStatus(status, ERR_NOT_SUPPORTED);
    return result;
}

KONKRET_REGISTRATION(
    "root/cimv2",
    "LMI_EthernetPortStatistics",
    "LMI_EthernetPortStatistics",
    "instance method")
