/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
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

#include <cmpimacs.h>
#include "globals.h"

CMPIStatus CreateIndication(
    const CMPIBroker *broker,
    const CMPIContext *ctx,
    const char *ns,
    const char *indication_class,
    CMPIInstance *source_instance,
    CMPIInstance *previous_instance)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    // We can't use konkretcmpi here, because it doesn't know embedded object/instance
    CMPIObjectPath *op = CMNewObjectPath(broker, ns, indication_class, &rc);
    if (rc.rc != CMPI_RC_OK) {
        error("Unable to create indication object path");
        return rc;
    }
    CMPIInstance *ind_inst = CMNewInstance(broker, op, &rc);
    if (rc.rc != CMPI_RC_OK) {
        error("Unable to create indication instance");
        return rc;
    }

    // Set source instance
    rc = CMSetProperty(ind_inst, "SourceInstance", &source_instance, CMPI_instance);
    if (rc.rc != CMPI_RC_OK) {
        error("Unable to set SourceInstance property");
        return rc;
    }
    // Set previous instance if exist
    if (previous_instance != NULL) {
        rc = CMSetProperty(ind_inst, "PreviousInstance", &previous_instance, CMPI_instance);
        if (rc.rc != CMPI_RC_OK) {
            error("Unable to set PreviousInstance property");
            return rc;
        }
    }
    rc = CBDeliverIndication(broker, ctx, ns, ind_inst);
    if (rc.rc != CMPI_RC_OK) {
        error("Unable to deliver indication");
        return rc;
    }
    debug("Indication %s created", indication_class);
    return rc;
}
