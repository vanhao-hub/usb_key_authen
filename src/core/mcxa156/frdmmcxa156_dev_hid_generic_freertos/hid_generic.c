/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
/*${standard_header_anchor}*/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_hid.h"

#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"

#include "hid_generic.h"

#include "fsl_device_registers.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "board.h"

// HAL includes
#include "../../hal/interface/usb_hid_hal.h"
#include "../../hal/nxp/usb_hid_hal_nxp.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

#if ((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
#include "usb_phy.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_InitHardware(void);

// External function declarations
#if USB_DEVICE_CONFIG_USE_TASK
extern void USB_DeviceTaskFn(void *deviceHandle);
#endif

// NOTE: USB callback functions have been moved to HAL layer (usb_hid_hal_nxp.c)
// Only application initialization remains here

/*******************************************************************************
 * Variables
 ******************************************************************************/

// Application-level task handles (since USB BSP variables moved to HAL)
static TaskHandle_t g_app_task_handle = NULL;
static TaskHandle_t g_usb_device_task_handle = NULL;

// NOTE: USB BSP code has been moved to HAL layer (usb_hid_hal_nxp.c)
// Only application-level variables remain here

/*******************************************************************************
 * Code - Application Entry Point Only
 ******************************************************************************/

// NOTE: USB BSP code has been moved to HAL layer (usb_hid_hal_nxp.c)
// Only application-level variables remain here

/*******************************************************************************
 * Code - Application Entry Point Only
 ******************************************************************************/

#if defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0)
void USB_DeviceTask(void *handle)
{
    while (1U)
    {
        USB_DeviceTaskFn(handle);
    }
}
#endif

void APP_task(void *handle)
{
    // handle parameter is not used in our HAL implementation
    (void)handle;  // Suppress unused parameter warning
    
    // Initialize HAL layer (which will call USB_DeviceApplicationInit internally)
    const usb_hid_hal_t* hal = usb_hid_hal_nxp_get_instance();
    
    if (hal->base.init() != HAL_SUCCESS) {
        usb_echo("HAL initialization failed\r\n");
        return;
    }
    
    usb_echo("USB HAL initialized successfully\r\n");

#if USB_DEVICE_CONFIG_USE_TASK
    // Note: USB task creation is now handled by HAL layer
    // Access HAL's device handle for USB task creation
    void* deviceHandlePtr = hal->get_device_handle();
    if (deviceHandlePtr)
    {
        usb_device_handle deviceHandle = (usb_device_handle)deviceHandlePtr;
        if (xTaskCreate(USB_DeviceTask,                         /* pointer to the task */
                        "usb device task",                      /* task name for kernel awareness debugging */
                        5000L / sizeof(portSTACK_TYPE),         /* task stack size */
                        deviceHandle,                           /* optional task startup argument */
                        5U,                                     /* initial priority */
                        &g_usb_device_task_handle               /* optional task handle to create */
                        ) != pdPASS)
        {
            usb_echo("usb device task create failed!\r\n");
            return;
        }
    }
#endif

    while (1U)
    {
        // Application main loop
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#if defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__)
int main(void)
#else
void main(void)
#endif
{
    BOARD_InitHardware();

    // Create application task - parameter not needed since HAL manages USB context
    if (xTaskCreate(APP_task,                                    /* pointer to the task */
                    "app task",                                  /* task name for kernel awareness debugging */
                    5000L / sizeof(portSTACK_TYPE),              /* task stack size */
                    NULL,                                        /* optional task startup argument - not needed */
                    4U,                                          /* initial priority */
                    &g_app_task_handle                           /* optional task handle to create */
                    ) != pdPASS)
    {
        usb_echo("app task create failed!\r\n");
#if (defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__))
        return 1U;
#else
        return;
#endif
    }

    vTaskStartScheduler();

#if (defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__))
    return 1U;
#endif
}
