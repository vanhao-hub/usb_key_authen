#ifndef FIDO_HID_TRANSPORT_H
#define FIDO_HID_TRANSPORT_H

/**
 * @file fido_hid_transport.h
 * @brief FIDO HID Transport Layer Interface
 * @author USB Key Authentication Team
 * @date 2025-09-08
 * @version 1.0
 * 
 * This file implements FIDO HID transport protocol on top of USB HID HAL.
 * Handles message fragmentation, channel management, and FIDO HID protocol compliance.
 */

#include "hal/interface/usb_hid_hal.h"
#include "hal/interface/hal_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief FIDO HID packet size (matches USB_HID_PACKET_SIZE) */
#define FIDO_HID_PACKET_SIZE        64

/** @brief FIDO HID payload sizes */
#define FIDO_HID_INIT_PAYLOAD_SIZE  57  // 64 - 4(CID) - 3(CMD+BCNT)
#define FIDO_HID_CONT_PAYLOAD_SIZE  59  // 64 - 4(CID) - 1(SEQ)

/** @brief Maximum FIDO message size per specification */
#define FIDO_MAX_MESSAGE_SIZE       7609

/** @brief USB endpoint for FIDO HID communication */
#define FIDO_HID_ENDPOINT           1

/** @brief FIDO HID command codes */
#define FIDO_HID_MSG                0x03    /**< CTAP2 message */
#define FIDO_HID_INIT               0x06    /**< Channel initialization */
#define FIDO_HID_PING               0x01    /**< Ping/echo */
#define FIDO_HID_ERROR              0x3F    /**< Error response */

/** @brief FIDO HID error codes */
#define FIDO_ERR_INVALID_CMD        0x01    /**< Invalid command */
#define FIDO_ERR_INVALID_PAR        0x02    /**< Invalid parameter */
#define FIDO_ERR_INVALID_LEN        0x03    /**< Invalid length */
#define FIDO_ERR_INVALID_SEQ        0x04    /**< Invalid sequence */
#define FIDO_ERR_MSG_TIMEOUT        0x05    /**< Message timeout */
#define FIDO_ERR_CHANNEL_BUSY       0x06    /**< Channel busy */
#define FIDO_ERR_INVALID_CID        0x0B    /**< Invalid channel ID */
#define FIDO_ERR_OTHER              0x7F    /**< Other error */

/** @brief FIDO channel identifiers */
#define FIDO_BROADCAST_CID          0xFFFFFFFF  /**< Broadcast channel */
#define FIDO_RESERVED_CID_START     0x00000000  /**< Reserved range start */
#define FIDO_RESERVED_CID_END       0x0000FFFF  /**< Reserved range end */

/** @brief Transport configuration */
#define FIDO_RECEIVE_TIMEOUT_MS     3000    /**< Message receive timeout */
#define FIDO_CHANNEL_TIMEOUT_MS     30000   /**< Channel inactivity timeout */
#define FIDO_MAX_CHANNELS           4       /**< Maximum concurrent channels */

/**
 * @brief FIDO transport state enumeration
 */
typedef enum {
    FIDO_TRANSPORT_IDLE,        /**< No active operations */
    FIDO_TRANSPORT_RECEIVING,   /**< Assembling fragmented message */
    FIDO_TRANSPORT_SENDING,     /**< Transmitting message */
    FIDO_TRANSPORT_ERROR        /**< Error state */
} fido_transport_state_t;

/**
 * @brief FIDO message received callback function type
 * 
 * Called when a complete FIDO message has been received and assembled.
 * 
 * @param cid Channel identifier
 * @param cmd FIDO command code
 * @param data Pointer to complete message data
 * @param length Length of message data
 * 
 * @note Callback is called from transport context
 * @warning Keep processing time minimal
 */
typedef void (*fido_message_callback_t)(uint32_t cid, uint8_t cmd, 
                                        const uint8_t* data, size_t length);

/**
 * @brief FIDO HID transport interface structure
 */
typedef struct {
    /**
     * @brief Initialize FIDO HID transport
     * 
     * @param usb_hal Pointer to USB HID HAL interface
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*init)(const usb_hid_hal_t* usb_hal);
    
    /**
     * @brief Deinitialize transport
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*deinit)(void);
    
    /**
     * @brief Send FIDO message
     * 
     * Automatically fragments large messages into multiple packets.
     * 
     * @param cid Channel identifier
     * @param cmd FIDO command code
     * @param data Pointer to message data
     * @param length Length of message data
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*send_message)(uint32_t cid, uint8_t cmd, 
                                const uint8_t* data, size_t length);
    
    /**
     * @brief Send FIDO error response
     * 
     * @param cid Channel identifier
     * @param error_code FIDO error code
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*send_error)(uint32_t cid, uint8_t error_code);
    
    /**
     * @brief Set message received callback
     * 
     * @param callback Callback function (NULL to disable)
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*set_message_callback)(fido_message_callback_t callback);
    
    /**
     * @brief Allocate new channel identifier
     * 
     * @param new_cid Pointer to store new channel ID
     * @return HAL_SUCCESS on success, error code otherwise
     */
    hal_result_t (*allocate_channel)(uint32_t* new_cid);
    
    /**
     * @brief Get transport state
     * 
     * @return Current transport state
     */
    fido_transport_state_t (*get_state)(void);
    
    /**
     * @brief Check if channel is active
     * 
     * @param cid Channel identifier to check
     * @return true if channel is active, false otherwise
     */
    bool (*is_channel_active)(uint32_t cid);
    
} fido_hid_transport_t;

/**
 * @brief Get FIDO HID transport instance
 * 
 * @return Pointer to transport interface (singleton)
 */
const fido_hid_transport_t* fido_hid_transport_get_instance(void);

/**
 * @brief FIDO HID packet helper functions
 */

/**
 * @brief Prepare FIDO initialization packet
 * 
 * @param packet Output 64-byte packet buffer
 * @param cid Channel identifier
 * @param cmd FIDO command
 * @param data Payload data
 * @param data_len Payload length
 */
void fido_hid_prepare_init_packet(uint8_t packet[FIDO_HID_PACKET_SIZE],
                                 uint32_t cid, uint8_t cmd,
                                 const uint8_t* data, uint16_t data_len);

/**
 * @brief Prepare FIDO continuation packet
 * 
 * @param packet Output 64-byte packet buffer
 * @param cid Channel identifier
 * @param seq Sequence number
 * @param data Payload data
 * @param data_len Payload length
 */
void fido_hid_prepare_cont_packet(uint8_t packet[FIDO_HID_PACKET_SIZE],
                                 uint32_t cid, uint8_t seq,
                                 const uint8_t* data, size_t data_len);

/**
 * @brief Parse FIDO HID packet
 * 
 * @param packet Input 64-byte packet
 * @param cid Output channel identifier
 * @param is_init Output: true if initialization packet
 * @param cmd_or_seq Output: command (init) or sequence (cont)
 * @param total_len Output: total message length (init packets only)
 * @param payload Output: pointer to payload in packet
 * @param payload_len Output: payload length in this packet
 */
void fido_hid_parse_packet(const uint8_t packet[FIDO_HID_PACKET_SIZE],
                          uint32_t* cid, bool* is_init, uint8_t* cmd_or_seq,
                          uint16_t* total_len, const uint8_t** payload,
                          size_t* payload_len);

#endif // FIDO_HID_TRANSPORT_H