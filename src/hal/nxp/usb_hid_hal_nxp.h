/**
 * @file usb_hid_hal_nxp.h
 * @brief NXP USB HID HAL Implementation Header
 * @author USB Key Authentication Team
 * @date 2025-09-11
 * @version 1.0
 * 
 * This file provides the header for NXP USB HID HAL implementation.
 */

#ifndef USB_HID_HAL_NXP_H
#define USB_HID_HAL_NXP_H

#include "../interface/usb_hid_hal.h"
#include "../interface/hal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/**
 * @brief NXP USB HID HAL version information
 */
#define USB_HID_HAL_NXP_VERSION_MAJOR   1
#define USB_HID_HAL_NXP_VERSION_MINOR   0
#define USB_HID_HAL_NXP_VERSION_PATCH   0

/*******************************************************************************
 * API
 ******************************************************************************/

/**
 * @brief Get NXP USB HID HAL instance
 * 
 * Returns a pointer to the USB HID HAL interface implemented using
 * NXP USB stack. This instance can be used with FIDO transport layer.
 * 
 * @return Pointer to USB HID HAL interface
 * 
 * @note The returned instance is a singleton
 * @note Call init() before using other HAL functions
 * 
 * @code
 * const usb_hid_hal_t* hal = usb_hid_hal_nxp_get_instance();
 * hal_result_t result = hal->base.init();
 * @endcode
 */
const usb_hid_hal_t* usb_hid_hal_nxp_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif // USB_HID_HAL_NXP_H
