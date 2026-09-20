/**
 * @file    pus5.c
 * @author  Merlin Kooshmanian
 * @brief   Source file for PUS 5 functions (Event reporting)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/******************************* Include Files *******************************/

#include "kernel.h"
#include "tm_management.h"
#include "services/pus5.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @copydoc BuildS5SS1234
 */
returnCode_t BuildS5SS1234(pusTM_t *tm, severityLevel_t severity, eventReport_t *report)
{
    returnCode_t return_value = RET_SUCCESSFUL;

    // Check parameter(s)
    if ((tm != NULL) && (report != NULL) && (severity <= SEVERITY_HIGH))
    {
        // Build TM
        return_value = BuildTM(tm, 5u, severity, (pusData_t *)report, EVENT_REPORT_SIZE);
    }
    else
    {
        return_value = RET_INVALID_PARAM;
    }

    return return_value;
}
