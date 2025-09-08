/**
 * @file fido_hid_transport.c
 * @brief FIDO HID Transport Layer Implementation
 */

#include "fido_hid_transport.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Channel information structure
 */
typedef struct {
    uint32_t cid;               /**< Channel identifier */
    uint32_t last_activity;     /**< Last activity timestamp */
    bool active;                /**< Channel is active */
} fido_channel_t;

/**
 * @brief Receive buffer for message assembly
 */
typedef struct {
    uint32_t cid;                           /**< Channel ID */
    uint8_t cmd;                            /**< Command */
    uint16_t total_length;                  /**< Total message length */
    uint16_t received_length;               /**< Bytes received so far */
    uint8_t expected_seq;                   /**< Expected sequence number */
    uint32_t timeout_start;                 /**< Receive start timestamp */
    uint8_t buffer[FIDO_MAX_MESSAGE_SIZE];  /**< Message buffer */
    bool active;                            /**< Buffer in use */
} fido_receive_buffer_t;

/**
 * @brief FIDO transport context
 */
typedef struct {
    const usb_hid_hal_t* usb_hal;           /**< USB HID HAL interface */
    fido_message_callback_t msg_callback;   /**< Message callback */
    fido_transport_state_t state;           /**< Current state */
    fido_receive_buffer_t rx_buffer;        /**< Receive buffer */
    fido_channel_t channels[FIDO_MAX_CHANNELS]; /**< Channel tracking */
    uint32_t next_cid;                      /**< Next CID to allocate */
    bool initialized;                       /**< Initialization flag */
} fido_transport_context_t;

/** @brief Global transport context */
static fido_transport_context_t g_transport_ctx = {0};

/**
 * @brief Internal function prototypes
 */
static void usb_rx_callback(uint8_t endpoint, const uint8_t* data, size_t length);
static void usb_tx_complete_callback(uint8_t endpoint, hal_result_t result);
static void usb_event_callback(uint32_t event);
static hal_result_t process_fido_packet(const uint8_t packet[FIDO_HID_PACKET_SIZE]);
static hal_result_t send_error_response(uint32_t cid, uint8_t error_code);
static void reset_receive_buffer(void);
static uint32_t get_timestamp_ms(void);
static fido_channel_t* find_channel(uint32_t cid);
static fido_channel_t* allocate_channel_slot(void);
static void update_channel_activity(uint32_t cid);

/**
 * @brief Initialize FIDO HID transport
 */
static hal_result_t fido_transport_init(const usb_hid_hal_t* usb_hal) {
    if (!usb_hal) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (g_transport_ctx.initialized) {
        return HAL_ERROR_ALREADY_INITIALIZED;
    }
    
    // Initialize context
    memset(&g_transport_ctx, 0, sizeof(g_transport_ctx));
    g_transport_ctx.usb_hal = usb_hal;
    g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
    g_transport_ctx.next_cid = 0x00010000; // Start after reserved range
    
    // Set USB callbacks
    hal_result_t result = usb_hal->set_callbacks(usb_rx_callback, 
                                                usb_tx_complete_callback,
                                                usb_event_callback);
    if (result != HAL_SUCCESS) {
        return result;
    }
    
    g_transport_ctx.initialized = true;
    return HAL_SUCCESS;
}

/**
 * @brief Deinitialize transport
 */
static hal_result_t fido_transport_deinit(void) {
    if (!g_transport_ctx.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    // Clear callbacks
    if (g_transport_ctx.usb_hal) {
        g_transport_ctx.usb_hal->set_callbacks(NULL, NULL, NULL);
    }
    
    // Reset context
    memset(&g_transport_ctx, 0, sizeof(g_transport_ctx));
    
    return HAL_SUCCESS;
}

/**
 * @brief Send FIDO message
 */
static hal_result_t fido_transport_send_message(uint32_t cid, uint8_t cmd, 
                                               const uint8_t* data, size_t length) {
    if (!g_transport_ctx.initialized || !g_transport_ctx.usb_hal) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (length > FIDO_MAX_MESSAGE_SIZE) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (g_transport_ctx.state == FIDO_TRANSPORT_SENDING) {
        return HAL_ERROR_BUSY;
    }
    
    g_transport_ctx.state = FIDO_TRANSPORT_SENDING;
    
    uint8_t packet[FIDO_HID_PACKET_SIZE];
    hal_result_t result;
    
    // Send initialization packet
    fido_hid_prepare_init_packet(packet, cid, cmd, data, (uint16_t)length);
    result = g_transport_ctx.usb_hal->send_report(FIDO_HID_ENDPOINT, packet, FIDO_HID_PACKET_SIZE);
    if (result != HAL_SUCCESS) {
        g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
        return result;
    }
    
    // Send continuation packets if needed
    size_t sent = FIDO_HID_INIT_PAYLOAD_SIZE;
    uint8_t seq = 0;
    
    while (sent < length) {
        size_t remaining = length - sent;
        
        fido_hid_prepare_cont_packet(packet, cid, seq++, 
                                   data + sent, remaining);
        
        result = g_transport_ctx.usb_hal->send_report(FIDO_HID_ENDPOINT, packet, FIDO_HID_PACKET_SIZE);
        if (result != HAL_SUCCESS) {
            g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
            return result;
        }
        
        sent += (remaining > FIDO_HID_CONT_PAYLOAD_SIZE) ? 
                FIDO_HID_CONT_PAYLOAD_SIZE : remaining;
    }
    
    // Update channel activity
    update_channel_activity(cid);
    
    g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
    return HAL_SUCCESS;
}

/**
 * @brief Send FIDO error response
 */
static hal_result_t fido_transport_send_error(uint32_t cid, uint8_t error_code) {
    return fido_transport_send_message(cid, FIDO_HID_ERROR, &error_code, 1);
}

/**
 * @brief Set message callback
 */
static hal_result_t fido_transport_set_message_callback(fido_message_callback_t callback) {
    if (!g_transport_ctx.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    g_transport_ctx.msg_callback = callback;
    return HAL_SUCCESS;
}

/**
 * @brief Allocate new channel
 */
static hal_result_t fido_transport_allocate_channel(uint32_t* new_cid) {
    if (!g_transport_ctx.initialized || !new_cid) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    // Find available channel slot
    fido_channel_t* channel = allocate_channel_slot();
    if (!channel) {
        return HAL_ERROR_RESOURCE_BUSY;
    }
    
    // Allocate new CID
    *new_cid = g_transport_ctx.next_cid++;
    
    // Avoid reserved range and wrap around
    if (g_transport_ctx.next_cid == 0) {
        g_transport_ctx.next_cid = 0x00010000;
    }
    
    // Initialize channel
    channel->cid = *new_cid;
    channel->last_activity = get_timestamp_ms();
    channel->active = true;
    
    return HAL_SUCCESS;
}

/**
 * @brief Get transport state
 */
static fido_transport_state_t fido_transport_get_state(void) {
    return g_transport_ctx.state;
}

/**
 * @brief Check if channel is active
 */
static bool fido_transport_is_channel_active(uint32_t cid) {
    if (cid == FIDO_BROADCAST_CID) {
        return true; // Broadcast channel always active
    }
    
    fido_channel_t* channel = find_channel(cid);
    return (channel && channel->active);
}

/**
 * @brief USB receive callback
 */
static void usb_rx_callback(uint8_t endpoint, const uint8_t* data, size_t length) {
    if (endpoint != FIDO_HID_ENDPOINT || length != FIDO_HID_PACKET_SIZE) {
        return;
    }
    
    // Convert to fixed-size packet and process
    uint8_t packet[FIDO_HID_PACKET_SIZE];
    memcpy(packet, data, FIDO_HID_PACKET_SIZE);
    
    process_fido_packet(packet);
}

/**
 * @brief USB transmit complete callback
 */
static void usb_tx_complete_callback(uint8_t endpoint, hal_result_t result) {
    // Handle transmission completion if needed
    (void)endpoint;
    (void)result;
}

/**
 * @brief USB event callback
 */
static void usb_event_callback(uint32_t event) {
    if (event & USB_HID_EVENT_DISCONNECT) {
        reset_receive_buffer();
        g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
        
        // Reset all channels
        memset(g_transport_ctx.channels, 0, sizeof(g_transport_ctx.channels));
    }
}

/**
 * @brief Process received FIDO packet
 */
static hal_result_t process_fido_packet(const uint8_t packet[FIDO_HID_PACKET_SIZE]) {
    uint32_t cid;
    bool is_init;
    uint8_t cmd_or_seq;
    uint16_t total_len;
    const uint8_t* payload;
    size_t payload_len;
    
    // Parse packet
    fido_hid_parse_packet(packet, &cid, &is_init, &cmd_or_seq, 
                         &total_len, &payload, &payload_len);
    
    if (is_init) {
        // Initialization packet
        uint8_t cmd = cmd_or_seq;
        
        // Special handling for broadcast CID (INIT command)
        if (cid == FIDO_BROADCAST_CID && cmd != FIDO_HID_INIT) {
            send_error_response(cid, FIDO_ERR_INVALID_CID);
            return HAL_ERROR_INVALID_PARAM;
        }
        
        // Validate channel for non-broadcast
        if (cid != FIDO_BROADCAST_CID && !fido_transport_is_channel_active(cid)) {
            send_error_response(cid, FIDO_ERR_INVALID_CID);
            return HAL_ERROR_INVALID_PARAM;
        }
        
        // Reset receive buffer for new message
        reset_receive_buffer();
        
        g_transport_ctx.rx_buffer.cid = cid;
        g_transport_ctx.rx_buffer.cmd = cmd;
        g_transport_ctx.rx_buffer.total_length = total_len;
        g_transport_ctx.rx_buffer.received_length = 0;
        g_transport_ctx.rx_buffer.expected_seq = 0;
        g_transport_ctx.rx_buffer.timeout_start = get_timestamp_ms();
        g_transport_ctx.rx_buffer.active = true;
        
        // Copy data from initialization packet
        size_t copy_len = (total_len > FIDO_HID_INIT_PAYLOAD_SIZE) ? 
                         FIDO_HID_INIT_PAYLOAD_SIZE : total_len;
        memcpy(g_transport_ctx.rx_buffer.buffer, payload, copy_len);
        g_transport_ctx.rx_buffer.received_length = copy_len;
        
        if (total_len <= FIDO_HID_INIT_PAYLOAD_SIZE) {
            // Complete message received
            if (g_transport_ctx.msg_callback) {
                g_transport_ctx.msg_callback(cid, cmd, 
                                           g_transport_ctx.rx_buffer.buffer, 
                                           total_len);
            }
            reset_receive_buffer();
            update_channel_activity(cid);
        } else {
            g_transport_ctx.state = FIDO_TRANSPORT_RECEIVING;
        }
    } else {
        // Continuation packet
        if (!g_transport_ctx.rx_buffer.active) {
            send_error_response(cid, FIDO_ERR_INVALID_SEQ);
            return HAL_ERROR_INVALID_STATE;
        }
        
        if (g_transport_ctx.rx_buffer.cid != cid) {
            send_error_response(cid, FIDO_ERR_INVALID_CID);
            return HAL_ERROR_INVALID_PARAM;
        }
        
        uint8_t seq = cmd_or_seq;
        if (seq != g_transport_ctx.rx_buffer.expected_seq) {
            send_error_response(cid, FIDO_ERR_INVALID_SEQ);
            reset_receive_buffer();
            return HAL_ERROR_INVALID_PARAM;
        }
        
        // Copy continuation data
        size_t remaining = g_transport_ctx.rx_buffer.total_length - 
                          g_transport_ctx.rx_buffer.received_length;
        size_t copy_len = (remaining > FIDO_HID_CONT_PAYLOAD_SIZE) ? 
                         FIDO_HID_CONT_PAYLOAD_SIZE : remaining;
        
        memcpy(g_transport_ctx.rx_buffer.buffer + g_transport_ctx.rx_buffer.received_length,
               payload, copy_len);
        g_transport_ctx.rx_buffer.received_length += copy_len;
        g_transport_ctx.rx_buffer.expected_seq++;
        
        if (g_transport_ctx.rx_buffer.received_length >= g_transport_ctx.rx_buffer.total_length) {
            // Complete message received
            if (g_transport_ctx.msg_callback) {
                g_transport_ctx.msg_callback(g_transport_ctx.rx_buffer.cid, 
                                           g_transport_ctx.rx_buffer.cmd,
                                           g_transport_ctx.rx_buffer.buffer, 
                                           g_transport_ctx.rx_buffer.total_length);
            }
            update_channel_activity(cid);
            reset_receive_buffer();
            g_transport_ctx.state = FIDO_TRANSPORT_IDLE;
        }
    }
    
    return HAL_SUCCESS;
}

/**
 * @brief Send error response
 */
static hal_result_t send_error_response(uint32_t cid, uint8_t error_code) {
    return fido_transport_send_error(cid, error_code);
}

/**
 * @brief Reset receive buffer
 */
static void reset_receive_buffer(void) {
    memset(&g_transport_ctx.rx_buffer, 0, sizeof(g_transport_ctx.rx_buffer));
}

/**
 * @brief Get current timestamp (placeholder implementation)
 */
static uint32_t get_timestamp_ms(void) {
    // TODO: Implement actual timestamp function
    static uint32_t counter = 0;
    return counter++;
}

/**
 * @brief Find channel by CID
 */
static fido_channel_t* find_channel(uint32_t cid) {
    for (int i = 0; i < FIDO_MAX_CHANNELS; i++) {
        if (g_transport_ctx.channels[i].active && 
            g_transport_ctx.channels[i].cid == cid) {
            return &g_transport_ctx.channels[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate channel slot
 */
static fido_channel_t* allocate_channel_slot(void) {
    // Find free slot
    for (int i = 0; i < FIDO_MAX_CHANNELS; i++) {
        if (!g_transport_ctx.channels[i].active) {
            return &g_transport_ctx.channels[i];
        }
    }
    
    // No free slots, try to find expired channel
    uint32_t current_time = get_timestamp_ms();
    for (int i = 0; i < FIDO_MAX_CHANNELS; i++) {
        if (current_time - g_transport_ctx.channels[i].last_activity > FIDO_CHANNEL_TIMEOUT_MS) {
            memset(&g_transport_ctx.channels[i], 0, sizeof(fido_channel_t));
            return &g_transport_ctx.channels[i];
        }
    }
    
    return NULL; // No available slots
}

/**
 * @brief Update channel activity
 */
static void update_channel_activity(uint32_t cid) {
    if (cid == FIDO_BROADCAST_CID) {
        return; // Don't track broadcast channel
    }
    
    fido_channel_t* channel = find_channel(cid);
    if (channel) {
        channel->last_activity = get_timestamp_ms();
    }
}

/**
 * @brief FIDO transport interface instance
 */
static const fido_hid_transport_t g_fido_transport = {
    .init = fido_transport_init,
    .deinit = fido_transport_deinit,
    .send_message = fido_transport_send_message,
    .send_error = fido_transport_send_error,
    .set_message_callback = fido_transport_set_message_callback,
    .allocate_channel = fido_transport_allocate_channel,
    .get_state = fido_transport_get_state,
    .is_channel_active = fido_transport_is_channel_active
};

/**
 * @brief Get FIDO transport instance
 */
const fido_hid_transport_t* fido_hid_transport_get_instance(void) {
    return &g_fido_transport;
}

/**
 * @brief Prepare FIDO initialization packet
 */
void fido_hid_prepare_init_packet(uint8_t packet[FIDO_HID_PACKET_SIZE],
                                 uint32_t cid, uint8_t cmd,
                                 const uint8_t* data, uint16_t data_len) {
    memset(packet, 0, FIDO_HID_PACKET_SIZE);
    
    // CID (big-endian)
    packet[0] = (cid >> 24) & 0xFF;
    packet[1] = (cid >> 16) & 0xFF;
    packet[2] = (cid >> 8) & 0xFF;
    packet[3] = cid & 0xFF;
    
    // Command and byte count
    packet[4] = cmd | 0x80;  // Initialization packet marker
    packet[5] = (data_len >> 8) & 0xFF;
    packet[6] = data_len & 0xFF;
    
    // Data (max 57 bytes in init packet)
    if (data && data_len > 0) {
        size_t copy_len = (data_len > FIDO_HID_INIT_PAYLOAD_SIZE) ? 
                         FIDO_HID_INIT_PAYLOAD_SIZE : data_len;
        memcpy(&packet[7], data, copy_len);
    }
}

/**
 * @brief Prepare FIDO continuation packet
 */
void fido_hid_prepare_cont_packet(uint8_t packet[FIDO_HID_PACKET_SIZE],
                                 uint32_t cid, uint8_t seq,
                                 const uint8_t* data, size_t data_len) {
    memset(packet, 0, FIDO_HID_PACKET_SIZE);
    
    // CID (big-endian)
    packet[0] = (cid >> 24) & 0xFF;
    packet[1] = (cid >> 16) & 0xFF;
    packet[2] = (cid >> 8) & 0xFF;
    packet[3] = cid & 0xFF;
    
    // Sequence number
    packet[4] = seq & 0x7F;  // Continuation packet (no 0x80 bit)
    
    // Data (max 59 bytes in continuation packet)
    if (data && data_len > 0) {
        size_t copy_len = (data_len > FIDO_HID_CONT_PAYLOAD_SIZE) ? 
                         FIDO_HID_CONT_PAYLOAD_SIZE : data_len;
        memcpy(&packet[5], data, copy_len);
    }
}

/**
 * @brief Parse FIDO HID packet
 */
void fido_hid_parse_packet(const uint8_t packet[FIDO_HID_PACKET_SIZE],
                          uint32_t* cid, bool* is_init, uint8_t* cmd_or_seq,
                          uint16_t* total_len, const uint8_t** payload,
                          size_t* payload_len) {
    // Extract CID (big-endian)
    *cid = ((uint32_t)packet[0] << 24) | 
           ((uint32_t)packet[1] << 16) | 
           ((uint32_t)packet[2] << 8) | 
           packet[3];
    
    *is_init = (packet[4] & 0x80) != 0;
    
    if (*is_init) {
        // Initialization packet
        *cmd_or_seq = packet[4] & 0x7F;
        *total_len = ((uint16_t)packet[5] << 8) | packet[6];
        *payload = &packet[7];
        *payload_len = (*total_len > FIDO_HID_INIT_PAYLOAD_SIZE) ? 
                      FIDO_HID_INIT_PAYLOAD_SIZE : *total_len;
    } else {
        // Continuation packet
        *cmd_or_seq = packet[4] & 0x7F;
        *total_len = 0; // Not applicable for continuation
        *payload = &packet[5];
        *payload_len = FIDO_HID_CONT_PAYLOAD_SIZE; // Will be trimmed by caller
    }
}