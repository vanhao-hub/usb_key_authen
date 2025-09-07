#ifndef USB_HID_HAL_H
#define USB_HID_HAL_H

/**
 * @file usb_hid_hal.h
 * @brief USB HID Hardware Abstraction Layer Interface
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file defines the USB HID HAL interface for communication with
 * host systems. It provides a platform-independent API for USB HID
 * operations used by FIDO2/WebAuthn authentication.
 */

#include "hal_common.h"

/** @brief USB HID packet size in bytes */
#define USB_HID_PACKET_SIZE         64

/** @brief USB Vendor ID (placeholder - replace with actual) */
#define USB_HID_VENDOR_ID           0x1234

/** @brief USB Product ID (placeholder - replace with actual) */
#define USB_HID_PRODUCT_ID          0x5678

/** @brief Maximum number of USB HID endpoints */
#define USB_HID_MAX_ENDPOINTS       4

/**
 * @brief USB HID endpoint direction types
 * 
 * Defines the direction of data flow for USB HID endpoints.
 */
typedef enum {
    USB_HID_ENDPOINT_IN = 0x80,     /**< Input endpoint (device to host) */
    USB_HID_ENDPOINT_OUT = 0x00     /**< Output endpoint (host to device) */
} usb_hid_endpoint_direction_t;

/**
 * @brief USB HID descriptor structure
 * 
 * Contains USB device descriptor information and HID-specific
 * configuration data.
 */
typedef struct {
    uint16_t vendor_id;                 /**< USB Vendor ID */
    uint16_t product_id;                /**< USB Product ID */
    uint16_t device_version;            /**< Device version number */
    const char* manufacturer_string;    /**< Manufacturer string */
    const char* product_string;         /**< Product description string */
    const char* serial_string;          /**< Serial number string */
    const uint8_t* report_descriptor;   /**< HID report descriptor */
    size_t report_descriptor_size;      /**< Size of report descriptor */
} usb_hid_descriptor_t;

/**
 * @brief USB HID receive callback function type
 * 
 * Called when data is received from the host.
 * 
 * @param endpoint Endpoint number that received data
 * @param data Pointer to received data buffer
 * @param length Number of bytes received
 * 
 * @note Callback is called from interrupt context on some platforms
 * @warning Keep processing time minimal in callback
 */
typedef void (*usb_hid_rx_callback_t)(uint8_t endpoint, const uint8_t* data, size_t length);

/**
 * @brief USB HID transmit complete callback function type
 * 
 * Called when a transmission operation completes.
 * 
 * @param endpoint Endpoint number that completed transmission
 * @param result Result of the transmission operation
 * 
 * @note Callback is called from interrupt context on some platforms
 */
typedef void (*usb_hid_tx_complete_callback_t)(uint8_t endpoint, hal_result_t result);

/**
 * @brief USB HID event callback function type
 * 
 * Called when USB events occur (connect, disconnect, suspend, etc.).
 * 
 * @param event USB event that occurred (USB_HID_EVENT_*)
 * 
 * @see USB_HID_EVENT_CONNECT, USB_HID_EVENT_DISCONNECT
 */
typedef void (*usb_hid_event_callback_t)(uint32_t event);

/** @brief USB device connected event */
#define USB_HID_EVENT_CONNECT       0x01

/** @brief USB device disconnected event */
#define USB_HID_EVENT_DISCONNECT    0x02

/** @brief USB device suspended event */
#define USB_HID_EVENT_SUSPEND       0x04

/** @brief USB device resumed event */
#define USB_HID_EVENT_RESUME        0x08

/** @brief USB device reset event */
#define USB_HID_EVENT_RESET         0x10

/**
 * @brief USB HID HAL interface structure
 * 
 * This structure defines the complete USB HID HAL API including
 * configuration, data transfer, and control operations.
 */
typedef struct {
    hal_base_t base;    /**< Base HAL interface */
    
    /**
     * @brief Configure USB HID device
     * 
     * Sets up the USB HID device with the provided descriptor.
     * Must be called after init() and before other operations.
     * 
     * @param descriptor Pointer to USB HID descriptor structure
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Configuration successful
     * @retval HAL_ERROR_INVALID_PARAM Invalid descriptor provided
     * @retval HAL_ERROR_NOT_INITIALIZED Module not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE USB hardware configuration failed
     * 
     * @note Descriptor data must remain valid during operation
     * @see usb_hid_descriptor_t
     */
    hal_result_t (*configure)(const usb_hid_descriptor_t* descriptor);
    
    /**
     * @brief Set USB HID callback functions
     * 
     * Registers callback functions for USB HID events and data transfer.
     * 
     * @param rx_cb Receive data callback (can be NULL)
     * @param tx_cb Transmit complete callback (can be NULL)
     * @param event_cb USB event callback (can be NULL)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Callbacks registered successfully
     * @retval HAL_ERROR_NOT_INITIALIZED Module not initialized
     * 
     * @note Callbacks are called from interrupt context on some platforms
     * @warning Keep callback processing time minimal
     */
    hal_result_t (*set_callbacks)(usb_hid_rx_callback_t rx_cb, 
                                 usb_hid_tx_complete_callback_t tx_cb,
                                 usb_hid_event_callback_t event_cb);
    
    /**
     * @brief Send HID report to host
     * 
     * Transmits data to the host via the specified endpoint.
     * Operation may be asynchronous depending on implementation.
     * 
     * @param endpoint Endpoint number to send data on
     * @param data Pointer to data buffer to send
     * @param length Number of bytes to send (max USB_HID_PACKET_SIZE)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data queued for transmission
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Module not initialized
     * @retval HAL_ERROR_BUSY Endpoint busy with previous transmission
     * @retval HAL_ERROR_HARDWARE_FAILURE USB hardware error
     * 
     * @note Data buffer must remain valid until tx_complete callback
     * @note Length must not exceed USB_HID_PACKET_SIZE
     * @see usb_hid_tx_complete_callback_t
     */
    hal_result_t (*send_report)(uint8_t endpoint, const uint8_t* data, size_t length);
    
    /**
     * @brief Receive HID report from host
     * 
     * Retrieves data received from the host on the specified endpoint.
     * This is typically used for polling-based implementations.
     * 
     * @param endpoint Endpoint number to receive data from
     * @param buffer Pointer to buffer to store received data
     * @param length Pointer to buffer size (input) and received length (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data received successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Module not initialized
     * @retval HAL_ERROR_TIMEOUT No data available
     * 
     * @note For interrupt-driven implementations, use rx_callback instead
     * @note *length is updated with actual received bytes
     */
    hal_result_t (*receive_report)(uint8_t endpoint, uint8_t* buffer, size_t* length);
    
    /**
     * @brief Set HID feature report
     * 
     * Sends a feature report to the host. Feature reports are used
     * for device configuration and control.
     * 
     * @param data Pointer to feature report data
     * @param length Size of feature report data
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Feature report sent successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Feature reports not supported
     * 
     * @note Feature reports are optional in HID specification
     */
    hal_result_t (*set_feature_report)(const uint8_t* data, size_t length);
    
    /**
     * @brief Get HID feature report
     * 
     * Retrieves a feature report from the device.
     * 
     * @param buffer Pointer to buffer for feature report data
     * @param length Pointer to buffer size (input) and report length (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Feature report retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Feature reports not supported
     * 
     * @note *length is updated with actual report size
     */
    hal_result_t (*get_feature_report)(uint8_t* buffer, size_t* length);
    
    /**
     * @brief Check if USB device is connected
     * 
     * @return true if device is connected to host, false otherwise
     * 
     * @note This checks physical connection, not configuration state
     */
    bool (*is_connected)(void);
    
    /**
     * @brief Get USB HID status
     * 
     * Retrieves current status flags of the USB HID device.
     * 
     * @param status Pointer to store status flags (USB_HID_STATUS_*)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Status retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid status pointer
     * 
     * @see USB_HID_STATUS_CONNECTED, USB_HID_STATUS_CONFIGURED
     */
    hal_result_t (*get_status)(uint32_t* status);
    
    /**
     * @brief Suspend USB device
     * 
     * Puts the USB device into suspend mode for power saving.
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Device suspended successfully
     * @retval HAL_ERROR_NOT_SUPPORTED Suspend not supported
     * 
     * @note Device may wake up on USB activity
     */
    hal_result_t (*suspend)(void);
    
    /**
     * @brief Resume USB device from suspend
     * 
     * Wakes up the USB device from suspend mode.
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Device resumed successfully
     * @retval HAL_ERROR_INVALID_STATE Device not suspended
     * 
     * @see suspend()
     */
    hal_result_t (*resume)(void);
    
} usb_hid_hal_t;

/** @brief USB device is connected to host */
#define USB_HID_STATUS_CONNECTED    0x01

/** @brief USB device is configured by host */
#define USB_HID_STATUS_CONFIGURED   0x02

/** @brief USB device is suspended */
#define USB_HID_STATUS_SUSPENDED    0x04

/** @brief Transmit operation in progress */
#define USB_HID_STATUS_TX_BUSY      0x08

/** @brief Receive data available */
#define USB_HID_STATUS_RX_READY     0x10

#endif // USB_HID_HAL_H