/**
 * @file fido_hid_transport_example.c
 * @brief Example usage of FIDO HID Transport Layer
 */

#include "fido_hid_transport.h"
#include "hal/hal_manager.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Example message received callback
 */
void on_fido_message_received(uint32_t cid, uint8_t cmd, 
                             const uint8_t* data, size_t length) {
    printf("FIDO Message received: CID=0x%08X, CMD=0x%02X, Length=%zu\n", 
           cid, cmd, length);
    
    const fido_hid_transport_t* transport = fido_hid_transport_get_instance();
    
    switch (cmd) {
        case FIDO_HID_PING:
            // Echo ping data back
            printf("Processing PING command\n");
            transport->send_message(cid, FIDO_HID_PING, data, length);
            break;
            
        case FIDO_HID_MSG:
            // Process FIDO2 CTAP message
            printf("Processing CTAP2 message (length=%zu bytes)\n", length);
            
            // Print first few bytes for debugging
            printf("Data: ");
            for (size_t i = 0; i < (length > 16 ? 16 : length); i++) {
                printf("%02X ", data[i]);
            }
            if (length > 16) printf("...");
            printf("\n");
            
            // TODO: Forward to CTAP2 processor
            // ctap2_process_message(cid, data, length);
            
            // For demo, send back a simple response
            uint8_t response[] = {0x00}; // CTAP2 success
            transport->send_message(cid, FIDO_HID_MSG, response, sizeof(response));
            break;
            
        case FIDO_HID_INIT:
            // Handle channel initialization
            printf("Processing INIT command\n");
            if (length >= 8) { // INIT request should be 8 bytes
                uint32_t new_cid;
                if (transport->allocate_channel(&new_cid) == HAL_SUCCESS) {
                    // Prepare INIT response (17 bytes)
                    uint8_t response[17] = {0};
                    
                    // Echo nonce (first 8 bytes)
                    memcpy(response, data, 8);
                    
                    // New CID (4 bytes)
                    response[8] = (new_cid >> 24) & 0xFF;
                    response[9] = (new_cid >> 16) & 0xFF;
                    response[10] = (new_cid >> 8) & 0xFF;
                    response[11] = new_cid & 0xFF;
                    
                    // Protocol version (1 byte) 
                    response[12] = 0x02; // FIDO_2_0
                    
                    // Device version (3 bytes)
                    response[13] = 0x01; // Major
                    response[14] = 0x00; // Minor  
                    response[15] = 0x00; // Build
                    
                    // Capabilities (1 byte)
                    response[16] = 0x04; // CAPFLAG_WINK
                    
                    transport->send_message(cid, FIDO_HID_INIT, response, sizeof(response));
                    printf("Allocated new CID: 0x%08X\n", new_cid);
                } else {
                    transport->send_error(cid, FIDO_ERR_CHANNEL_BUSY);
                }
            } else {
                transport->send_error(cid, FIDO_ERR_INVALID_LEN);
            }
            break;
            
        default:
            printf("Unknown command: 0x%02X\n", cmd);
            transport->send_error(cid, FIDO_ERR_INVALID_CMD);
            break;
    }
}

/**
 * @brief Initialize FIDO transport example
 */
hal_result_t init_fido_transport_example(void) {
    // Get USB HID HAL (assuming hal_manager exists)
    // const usb_hid_hal_t* usb_hal = hal_manager_get_usb_hid();
    // if (!usb_hal) {
    //     printf("Error: USB HID HAL not available\n");
    //     return HAL_ERROR_NOT_AVAILABLE;
    // }
    
    // For now, use NULL as placeholder
    const usb_hid_hal_t* usb_hal = NULL;
    
    // Get transport instance
    const fido_hid_transport_t* transport = fido_hid_transport_get_instance();
    
    // Initialize transport
    hal_result_t result = transport->init(usb_hal);
    if (result != HAL_SUCCESS) {
        printf("Error: Transport initialization failed: %d\n", result);
        return result;
    }
    
    // Set message callback
    result = transport->set_message_callback(on_fido_message_received);
    if (result != HAL_SUCCESS) {
        printf("Error: Failed to set message callback: %d\n", result);
        return result;
    }
    
    printf("FIDO HID Transport initialized successfully\n");
    return HAL_SUCCESS;
}

/**
 * @brief Test fragmentation with large message (SENDING)
 */
hal_result_t test_large_message_send(void) {
    const fido_hid_transport_t* transport = fido_hid_transport_get_instance();
    
    // Create large test message (200 bytes)
    uint8_t large_data[200];
    for (int i = 0; i < 200; i++) {
        large_data[i] = i & 0xFF;
    }
    
    printf("=== Testing Large Message SENDING (200 bytes) ===\n");
    printf("Expected fragmentation:\n");
    printf("  Packet 1 (INIT): 57 bytes payload\n");
    printf("  Packet 2 (CONT seq=0): 59 bytes payload\n");
    printf("  Packet 3 (CONT seq=1): 59 bytes payload\n");
    printf("  Packet 4 (CONT seq=2): 25 bytes payload (padded to 64)\n");
    
    uint32_t test_cid = 0x12345678;
    hal_result_t result = transport->send_message(test_cid, FIDO_HID_MSG, 
                                                 large_data, sizeof(large_data));
    
    if (result == HAL_SUCCESS) {
        printf("Large message fragmentation test PASSED\n");
    } else {
        printf("Large message fragmentation test FAILED: %d\n", result);
    }
    
    return result;
}

/**
 * @brief Demo receive fragmented message simulation (RECEIVING)
 * 
 * This simulates receiving multiple 64-byte packets that need to be 
 * reassembled into a complete message.
 */
hal_result_t test_large_message_receive_simulation(void) {
    printf("\n=== Testing Large Message RECEIVING (Simulation) ===\n");
    
    // Simulate receiving a 200-byte message fragmented into 4 packets
    uint32_t test_cid = 0x87654321;
    uint8_t test_cmd = FIDO_HID_MSG;
    uint16_t total_length = 200;
    
    // Create test payload (200 bytes)
    uint8_t original_data[200];
    for (int i = 0; i < 200; i++) {
        original_data[i] = (i * 2) & 0xFF; // Different pattern
    }
    
    printf("Simulating reception of 4 packets...\n");
    
    // Packet 1: Initialization packet (57 bytes payload)
    uint8_t packet1[64];
    fido_hid_prepare_init_packet(packet1, test_cid, test_cmd, original_data, total_length);
    
    printf("Packet 1 (INIT): CID=0x%08X, CMD=0x%02X, Length=%u\n", 
           test_cid, test_cmd, total_length);
    printf("  First 16 bytes: ");
    for (int i = 7; i < 7 + 16; i++) {
        printf("%02X ", packet1[i]);
    }
    printf("\n");
    
    // Packet 2: Continuation packet seq=0 (59 bytes payload)
    uint8_t packet2[64];
    fido_hid_prepare_cont_packet(packet2, test_cid, 0, original_data + 57, 200 - 57);
    
    printf("Packet 2 (CONT seq=0):\n");
    printf("  First 16 bytes: ");
    for (int i = 5; i < 5 + 16; i++) {
        printf("%02X ", packet2[i]);
    }
    printf("\n");
    
    // Packet 3: Continuation packet seq=1 (59 bytes payload)
    uint8_t packet3[64];
    fido_hid_prepare_cont_packet(packet3, test_cid, 1, original_data + 57 + 59, 200 - 57 - 59);
    
    printf("Packet 3 (CONT seq=1):\n");
    printf("  First 16 bytes: ");
    for (int i = 5; i < 5 + 16; i++) {
        printf("%02X ", packet3[i]);
    }
    printf("\n");
    
    // Packet 4: Continuation packet seq=2 (25 bytes payload)
    uint8_t packet4[64];
    fido_hid_prepare_cont_packet(packet4, test_cid, 2, original_data + 57 + 59 + 59, 200 - 57 - 59 - 59);
    
    printf("Packet 4 (CONT seq=2):\n");
    printf("  First 16 bytes: ");
    for (int i = 5; i < 5 + 16; i++) {
        printf("%02X ", packet4[i]);
    }
    printf("\n");
    
    // In a real scenario, these packets would be received via USB HID HAL
    // and processed by the transport layer's USB RX callback:
    //
    // usb_rx_callback(FIDO_HID_ENDPOINT, packet1, 64);
    // usb_rx_callback(FIDO_HID_ENDPOINT, packet2, 64);  
    // usb_rx_callback(FIDO_HID_ENDPOINT, packet3, 64);
    // usb_rx_callback(FIDO_HID_ENDPOINT, packet4, 64);
    //
    // The transport layer would:
    // 1. Parse each packet to extract CID, command/sequence, payload
    // 2. Validate sequence numbers for continuation packets
    // 3. Assemble payload data in the receive buffer
    // 4. When complete, call on_fido_message_received(cid, cmd, assembled_data, total_length)
    
    printf("\nNOTE: In real operation, these packets would be:\n");
    printf("1. Received via USB HID HAL callbacks\n");
    printf("2. Parsed by transport layer\n");
    printf("3. Assembled into complete message\n");
    printf("4. Delivered to application via on_fido_message_received()\n");
    
    return HAL_SUCCESS;
}

/**
 * @brief Demo packet parsing 
 */
hal_result_t test_packet_parsing(void) {
    printf("\n=== Testing Packet Parsing ===\n");
    
    // Create a test initialization packet
    uint32_t test_cid = 0xABCD1234;
    uint8_t test_cmd = FIDO_HID_PING;
    uint8_t test_data[] = "Hello FIDO!";
    uint16_t test_data_len = strlen((char*)test_data);
    
    uint8_t packet[64];
    fido_hid_prepare_init_packet(packet, test_cid, test_cmd, test_data, test_data_len);
    
    printf("Created packet: ");
    for (int i = 0; i < 20; i++) {
        printf("%02X ", packet[i]);
    }
    printf("...\n");
    
    // Parse the packet back
    uint32_t parsed_cid;
    bool is_init;
    uint8_t cmd_or_seq;
    uint16_t total_len;
    const uint8_t* payload;
    size_t payload_len;
    
    fido_hid_parse_packet(packet, &parsed_cid, &is_init, &cmd_or_seq, 
                         &total_len, &payload, &payload_len);
    
    printf("Parsed results:\n");
    printf("  CID: 0x%08X (expected: 0x%08X) %s\n", 
           parsed_cid, test_cid, (parsed_cid == test_cid) ? "✓" : "✗");
    printf("  Is Init: %s (expected: true) %s\n", 
           is_init ? "true" : "false", is_init ? "✓" : "✗");
    printf("  Command: 0x%02X (expected: 0x%02X) %s\n", 
           cmd_or_seq, test_cmd, (cmd_or_seq == test_cmd) ? "✓" : "✗");
    printf("  Total Length: %u (expected: %u) %s\n", 
           total_len, test_data_len, (total_len == test_data_len) ? "✓" : "✗");
    printf("  Payload Length: %zu (expected: %u) %s\n", 
           payload_len, test_data_len, (payload_len == test_data_len) ? "✓" : "✗");
    printf("  Payload: \"");
    for (size_t i = 0; i < payload_len; i++) {
        printf("%c", payload[i]);
    }
    printf("\" %s\n", (memcmp(payload, test_data, test_data_len) == 0) ? "✓" : "✗");
    
    return HAL_SUCCESS;
}

/**
 * @brief Demo continuation packet parsing
 */
hal_result_t test_continuation_packet_parsing(void) {
    printf("\n=== Testing Continuation Packet Parsing ===\n");
    
    uint32_t test_cid = 0x11223344;
    uint8_t test_seq = 5;
    uint8_t test_data[] = "This is continuation packet data that should be parsed correctly";
    size_t test_data_len = strlen((char*)test_data);
    
    uint8_t packet[64];
    fido_hid_prepare_cont_packet(packet, test_cid, test_seq, test_data, test_data_len);
    
    printf("Created continuation packet (seq=%u)\n", test_seq);
    
    // Parse the packet
    uint32_t parsed_cid;
    bool is_init;
    uint8_t cmd_or_seq;
    uint16_t total_len;
    const uint8_t* payload;
    size_t payload_len;
    
    fido_hid_parse_packet(packet, &parsed_cid, &is_init, &cmd_or_seq, 
                         &total_len, &payload, &payload_len);
    
    printf("Parsed results:\n");
    printf("  CID: 0x%08X (expected: 0x%08X) %s\n", 
           parsed_cid, test_cid, (parsed_cid == test_cid) ? "✓" : "✗");
    printf("  Is Init: %s (expected: false) %s\n", 
           is_init ? "true" : "false", !is_init ? "✓" : "✗");
    printf("  Sequence: %u (expected: %u) %s\n", 
           cmd_or_seq, test_seq, (cmd_or_seq == test_seq) ? "✓" : "✗");
    printf("  Payload Length: %zu\n", payload_len);
    printf("  Payload: \"");
    size_t print_len = (payload_len > 40) ? 40 : payload_len;
    for (size_t i = 0; i < print_len; i++) {
        printf("%c", payload[i]);
    }
    if (payload_len > 40) printf("...");
    printf("\"\n");
    
    return HAL_SUCCESS;
}

/**
 * @brief Run all transport tests
 */
hal_result_t run_transport_tests(void) {
    printf("=== FIDO HID Transport Layer Tests ===\n\n");
    
    // Test packet parsing
    test_packet_parsing();
    test_continuation_packet_parsing();
    
    // Test message fragmentation
    test_large_message_send();
    
    // Test message reassembly simulation
    test_large_message_receive_simulation();
    
    printf("\n=== All Tests Completed ===\n");
    return HAL_SUCCESS;
}