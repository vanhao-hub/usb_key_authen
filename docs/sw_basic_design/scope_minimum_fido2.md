# Minimum Viable FIDO2 Authenticator - Scope Definition

## 1. Simplified Architecture (FIDO2 Only)

```
┌─────────────────────────────────────────────┐
│           Application Layer                 │
│  ┌─────────────────────────────────────────┐ │
│  │          FIDO2 Only                     │ │
│  │    (MakeCredential, GetAssertion)       │ │
│  └─────────────────────────────────────────┘ │
├─────────────────────────────────────────────┤
│           Platform Layer                    │
│  ┌─────────────┬─────────────────────────┐  │
│  │ Crypto      │    Communication        │  │
│  │ Engine      │                         │  │
│  └─────────────┘  ┌─────────────────────┐  │
│                   │ FIDO HID Transport  │  │  ← CHỈ CÓ CÁI NÀY
│                   │     (64-byte)       │  │
│                   └─────────────────────┘  │
├─────────────────────────────────────────────┤
│               HAL Layer                     │
│  ┌─────────────┬─────────────────────────┐  │
│  │USB HID HAL  │    Crypto HAL           │  │  ← BỎ Storage HAL
│  └─────────────┴─────────────────────────┘  │
└─────────────────────────────────────────────┘
```

## 2. Core Components (Minimal Set)

### 2.1. HAL Layer (Chỉ 2 components)

#### 2.1.1. USB HID HAL
USB HID HAL cung cấp interface thuần túy cho USB HID operations - **protocol agnostic**.

**API Design (Fixed 64-byte):**
```c
typedef struct {
    hal_base_t base;
    
    // Configuration
    hal_result_t (*configure)(const usb_hid_descriptor_t* descriptor);
    hal_result_t (*set_callbacks)(usb_hid_rx_callback_t rx_cb, 
                                 usb_hid_tx_complete_callback_t tx_cb,
                                 usb_hid_event_callback_t event_cb);
    
    // Fixed 64-byte operations
    hal_result_t (*send_report)(uint8_t endpoint, const uint8_t packet[64]);
    hal_result_t (*receive_report)(uint8_t endpoint, uint8_t packet[64]);
    
    // Status & Control
    bool (*is_connected)(void);
    hal_result_t (*get_status)(uint32_t* status);
    hal_result_t (*suspend)(void);
    hal_result_t (*resume)(void);
} usb_hid_hal_t;
```

**Callback Types:**
```c
// Always receives exactly 64 bytes
typedef void (*usb_hid_rx_callback_t)(uint8_t endpoint, const uint8_t packet[64]);

// Transmission complete notification
typedef void (*usb_hid_tx_complete_callback_t)(uint8_t endpoint, hal_result_t result);

// USB events (connect, disconnect, suspend, resume)
typedef void (*usb_hid_event_callback_t)(uint32_t event);
```

**Key Points:**
- ✅ **Protocol agnostic**: Không biết gì về FIDO, chỉ xử lý USB HID
- ✅ **Fixed 64-byte packets**: Luôn gửi/nhận đúng 64 bytes
- ✅ **No padding logic**: Application phải tự chuẩn bị 64-byte packet
- ✅ **Pure HAL**: Chỉ hardware abstraction, không có business logic

#### 2.1.2. Crypto HAL
Crypto HAL cung cấp interface cho basic cryptographic operations.

**API Design:**
```c
typedef struct {
    hal_base_t base;
    
    // Key operations
    hal_result_t (*generate_key_pair)(uint8_t* private_key, uint8_t* public_key);
    hal_result_t (*ecdsa_sign)(const uint8_t* private_key, const uint8_t* hash, uint8_t* signature);
    hal_result_t (*ecdsa_verify)(const uint8_t* public_key, const uint8_t* hash, const uint8_t* signature);
    
    // Hash operations
    hal_result_t (*sha256)(const uint8_t* data, size_t length, uint8_t* hash);
    hal_result_t (*hmac_sha256)(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t* hmac);
    
    // Random number generation
    hal_result_t (*random_bytes)(uint8_t* buffer, size_t length);
} crypto_hal_t;
```

**Crypto Requirements (Phase 1):**
- ✅ **ECDSA P-256 (secp256r1)**: FIDO2 required curve
- ✅ **SHA-256**: Hash function
- ✅ **HMAC-SHA256**: Message authentication
- ✅ **CSPRNG**: Cryptographically secure random numbers
- ❌ ~~Ed25519~~ → Phase 2
- ❌ ~~RSA~~ → Phase 2
- ❌ ~~AES encryption~~ → Phase 2

### 2.2. Platform Layer

#### 2.2.1. FIDO HID Transport
Transport layer xử lý FIDO HID protocol trên USB HID HAL.

**Responsibilities:**
```c
// FIDO HID Protocol Constants
#define FIDO_HID_PACKET_SIZE        64
#define FIDO_HID_INIT_PAYLOAD_SIZE  57  // 64 - 4(CID) - 3(CMD+BCNT)
#define FIDO_HID_CONT_PAYLOAD_SIZE  59  // 64 - 4(CID) - 1(SEQ)

// FIDO Commands
#define FIDO_HID_MSG                0x03
#define FIDO_HID_INIT               0x06
#define FIDO_HID_PING               0x01
#define FIDO_HID_ERROR              0x3F

// Transport API
typedef struct {
    hal_result_t (*init)(const usb_hid_hal_t* usb_hal);
    hal_result_t (*send_message)(uint32_t cid, uint8_t cmd, const uint8_t* data, size_t length);
    hal_result_t (*set_message_callback)(fido_message_callback_t callback);
    hal_result_t (*allocate_channel)(uint32_t* new_cid);
    fido_transport_state_t (*get_state)(void);
} fido_transport_t;
```

**Key Features:**
- ✅ **Message fragmentation**: Chia messages > 57 bytes thành multiple packets
- ✅ **Message reassembly**: Ghép packets thành complete message
- ✅ **CID management**: Channel ID allocation và tracking
- ✅ **Error handling**: FIDO HID error responses
- ✅ **Packet helpers**: Preparation và parsing functions

#### 2.2.2. Basic Crypto Engine
Crypto engine wrapper cho crypto HAL.

**API Design:**
```c
typedef struct {
    hal_result_t (*init)(const crypto_hal_t* crypto_hal);
    hal_result_t (*generate_credential_key)(uint8_t* private_key, uint8_t* public_key);
    hal_result_t (*sign_assertion)(const uint8_t* private_key, const uint8_t* client_data_hash, 
                                  const uint8_t* auth_data, uint8_t* signature);
    hal_result_t (*verify_signature)(const uint8_t* public_key, const uint8_t* data, 
                                    const uint8_t* signature);
} crypto_engine_t;
```

### 2.3. Application Layer - FIDO2 Core

#### 2.3.1. FIDO2 Features (Minimal MVP)
```c
// Phase 1 - Minimum viable features
✅ GetInfo command
✅ MakeCredential (basic, no PIN, no resident key)  
✅ GetAssertion (basic, no PIN, no resident key)
✅ User presence detection (simple button press)

// Phase 2 - Advanced features
❌ PIN/UV support 
❌ Resident keys (discoverable credentials)
❌ Credential management 
❌ Reset command
❌ Multiple transports
❌ Enterprise attestation
```

#### 2.3.2. CTAP2 Commands Implementation
```c
// ctap2_commands.h
typedef struct {
    hal_result_t (*get_info)(ctap2_get_info_response_t* response);
    hal_result_t (*make_credential)(const ctap2_make_credential_request_t* request, 
                                   ctap2_make_credential_response_t* response);
    hal_result_t (*get_assertion)(const ctap2_get_assertion_request_t* request,
                                 ctap2_get_assertion_response_t* response);
} ctap2_commands_t;
```

**GetInfo Response (Phase 1):**
```c
typedef struct {
    const char* versions[2];        // ["FIDO_2_0", "U2F_V2"]
    const char* aaguid[16];         // Authenticator AAGUID
    uint32_t max_msg_size;          // 1024
    uint8_t pin_protocols[1];       // [1] - PIN protocol v1
    uint32_t max_creds_in_list;     // 10
    uint32_t max_cred_id_length;    // 64
    const char* transports[1];      // ["usb"]
    bool client_pin_supported;      // false (Phase 1)
    bool resident_key_supported;    // false (Phase 1)
    bool user_presence_required;    // true
    bool user_verification_supported; // false (Phase 1)
} ctap2_get_info_response_t;
```

#### 2.3.3. State Machine (Simplified)
```c
typedef enum {
    FIDO_STATE_IDLE,
    FIDO_STATE_PROCESSING,  
    FIDO_STATE_WAITING_UP,  // User presence
    FIDO_STATE_ERROR
} fido_state_t;

// State transitions
// Idle → ProcessingCommand (on CTAP2 command)
// ProcessingCommand → WaitingUserPresence (need button press)
// WaitingUserPresence → ProcessingCommand (button pressed)
// Any state → Error (on error)
// Any state → Idle (on completion/reset)
```

## 3. USB HID Report Length Standard

### 3.1. USB HID Report Behavior
Theo chuẩn USB HID:
- **Reports có kích thước cố định** được định nghĩa trong Report Descriptor
- **Không có length field** trong packet
- **Application phải tự pad** nếu data < report size

### 3.2. FIDO2 HID Specification
- **Report size: 64 bytes cố định**
- **No automatic padding** - application responsibility  
- **Length parameter** trong send_report() chỉ để validation

### 3.3. HAL Implementation Guidelines
```c
hal_result_t usb_hid_send_report(uint8_t endpoint, const uint8_t packet[64]) {
    // HAL chỉ gửi exactly 64 bytes
    // Không có padding logic
    // Application đã chuẩn bị sẵn 64-byte packet
    return hardware_send(endpoint, packet, 64);
}
```

## 4. Directory Structure (Simplified)

```
src/
├── app/
│   └── fido2/                    # Chỉ FIDO2
│       ├── fido2_app.h/c        # Main FIDO2 application logic
│       ├── ctap2_commands.h/c   # CTAP2 command implementations
│       └── fido2_state.h/c      # Simple state machine
├── platform/
│   ├── crypto/
│   │   ├── crypto_engine.h/c    # Basic crypto wrapper
│   │   └── ecdsa_p256.h/c       # ECDSA P-256 implementation
│   └── transport/
│       ├── fido_hid_transport.h/c  # FIDO HID transport only
│       └── fido_hid_protocol.h     # FIDO packet format helpers
├── hal/
│   ├── interface/
│   │   ├── usb_hid_hal.h        # USB HID HAL interface
│   │   ├── crypto_hal.h         # Crypto HAL interface
│   │   └── hal_common.h         # Common HAL definitions
│   └── mock/                    # Mock implementations for testing
│       ├── usb_hid_mock.c
│       └── crypto_mock.c
└── config/
    └── fido2_config.h           # Feature configuration
```

## 5. Implementation Plan (Phase 1)

### Step 1: HAL Interfaces
- ✅ Define USB HID HAL với fixed 64-byte API
- ✅ Define Crypto HAL với basic operations
- ✅ Create mock implementations cho testing

### Step 2: FIDO HID Transport  
- ✅ Implement packet fragmentation/reassembly
- ✅ CID management (simple counter)
- ✅ FIDO HID protocol helpers
- ✅ Message callback interface

### Step 3: Basic Crypto Engine
- ✅ ECDSA P-256 wrapper
- ✅ SHA-256, HMAC-SHA256 implementation
- ✅ Key generation và signing

### Step 4: FIDO2 Core
- ✅ GetInfo implementation
- ✅ MakeCredential (basic, no resident keys)
- ✅ GetAssertion (basic, với allowList)
- ✅ Simple credential storage (in-memory)

### Step 5: Integration & Testing
- ✅ End-to-end testing với browser
- ✅ FIDO2 conformance testing (basic subset)
- ✅ Performance optimization

## 6. Configuration (Minimal)

```c
// fido2_config.h
#define FIDO2_MAX_CREDENTIAL_COUNT      10      // In-memory storage
#define FIDO2_USER_PRESENCE_TIMEOUT     30000   // 30 seconds  
#define FIDO2_MAX_MESSAGE_SIZE          1024    // CTAP2 message limit
#define FIDO2_CREDENTIAL_ID_LENGTH      64      // Credential ID size

// Disabled features for Phase 1
#define FIDO2_SUPPORT_PIN               0       // Phase 2
#define FIDO2_SUPPORT_RESIDENT_KEYS     0       // Phase 2
#define FIDO2_SUPPORT_CRED_MGMT         0       // Phase 2
#define FIDO2_SUPPORT_ENTERPRISE_ATTEST 0       // Phase 2

// Transport configuration
#define FIDO_HID_CHANNEL_TIMEOUT        3000    // Channel timeout (ms)
#define FIDO_HID_MAX_CHANNELS           4       // Max concurrent channels
```

## 7. Key Design Principles

### 7.1. Separation of Concerns
- **HAL**: Pure hardware abstraction, protocol agnostic
- **Transport**: Protocol-specific message handling
- **Application**: Business logic và FIDO2 semantics

### 7.2. Simplicity First
- **No advanced features** trong Phase 1
- **Fixed-size packets** để đơn giản hóa
- **In-memory storage** thay vì persistent storage
- **Simple state machine** với minimal states

### 7.3. Testability
- **Mock HAL implementations** cho unit testing
- **Layered architecture** cho integration testing
- **Clear interfaces** giữa các components

### 7.4. Future Extensibility
- **Plugin-ready architecture** cho Phase 2 features
- **Abstract interfaces** cho easy enhancement
- **Configuration-driven** feature enabling

## 8. Testing Strategy

### 8.1. Unit Testing
```c
// Mock-based testing cho mỗi layer
test_usb_hid_hal_mock()         // HAL layer testing
test_fido_transport()           // Transport layer testing  
test_ctap2_commands()           // Application layer testing
```

### 8.2. Integration Testing
```c
// End-to-end testing với browser
test_chrome_webauthn()          // Chrome compatibility
test_firefox_webauthn()         // Firefox compatibility
test_fido2_conformance()        // FIDO Alliance test suite (basic)
```

### 8.3. Performance Testing
```c
// Performance benchmarks
test_makeCredential_latency()   // Response time measurement
test_concurrent_channels()      // Multiple channel handling
test_memory_usage()             // RAM usage profiling
```

**Đây là scope tối thiểu để tạo ra một working FIDO2 authenticator prototype, tập trung vào core functionality và bỏ qua tất cả advanced features. Thiết kế đảm bảo dễ implement, debug và mở