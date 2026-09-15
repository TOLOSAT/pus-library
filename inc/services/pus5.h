/**
 * @file    pus5.h
 * @author  Merlin Kooshmanian
 * @brief   Header file for PUS 5 functions (Event Reporting)
 *
 * @copyright Copyright (c) TOLOSAT 2026
 */

/**
 * @defgroup middlewares Middlewares
 * @{
 * @defgroup pus PUS Library
 * @{
 * @defgroup pus5 PUS Service 5
 * @brief PUS service 5 (Event Reporting) implementation
 * @{
 */

#ifndef PUS5_H
#define PUS5_H

/******************************* Include Files *******************************/

#include "kernel_types.h"
#include "pus_types.h"

/***************************** Macros Definitions ****************************/

#define EVENT_ID_SIZE     4u                                /**< Event ID size */
#define EVENT_DATA_SIZE   6u                                /**< Event data size */
#define EVENT_REPORT_SIZE (EVENT_ID_SIZE + EVENT_DATA_SIZE) /**< Event report size */

/***************************** Types Definitions *****************************/

/**
 * @struct  eventReport_t
 * @brief   Struct type for an event report
 */
typedef struct
{
    uint32_t EID;                  /**< @brief Event ID */
    uint8_t data[EVENT_DATA_SIZE]; /**< @brief Event data */
} ATTR_BYTE_ALIGNED eventReport_t;
ASSERT_SIZE(eventReport_t, EVENT_REPORT_SIZE)

/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

/**
 * @fn          BuildS5SS1234(pusTM_t *tm, severityLevel_t severity, eventReport_t *report)
 * @brief       Function that send S5SS1, S5SS2, S5SS3 or S5SS4 TM (event report)
 * @param[out]  tm TM that will be sent
 * @param[in]   severity Severity of the event
 * @param[in]   report Event report
 * @retval      #RET_INVALID_PARAM if a pointer is NULL or severity is not 1,2,3 or 4
 * @retval      #RET_ERROR if cannot build TM
 * @retval      #RET_SUCCESSFUL else
 */
extern returnCode_t BuildS5SS1234(pusTM_t *tm, severityLevel_t severity, eventReport_t *report);

#endif /* PUS5_H */

/**
 * @}
 * @}
 * @}
 */