# Framework Architecture Design

## Tổng Quan Thiết Kế

Framework được thiết kế với nguyên tắc **layered architecture** và **dependency injection** để đảm bảo:
- **Modularity**: Dễ dàng thêm/bớt chức năng
- **Portability**: Không phụ thuộc MCU cụ thể  
- **Maintainability**: Code dễ đọc, dễ debug
- **Scalability**: Dễ mở rộng từ prototype lên production

## 1. Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   FIDO2     │  │     PIV     │  │        OTP          │  │
│  │  WebAuthn   │  │   Module    │  │      Module         │  │
│  │   Module    │  │             │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                    PROTOCOL LAYER                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   CTAP2     │  │  ISO 7816   │  │    Yubico OTP       │  │
│  │  Protocol   │  │  Protocol   │  │     Protocol        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                   TRANSPORT LAYER                           │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                   USB Interface                         ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐ ││
│  │  │  USB HID    │  │  USB CCID   │  │   USB CDC       │ ││
│  │  │             │  │             │  │                 │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────────┘ ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                 HARDWARE ABSTRACTION LAYER                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   USB HAL   │  │ Crypto HAL  │  │     Storage HAL     │  │
│  │             │  │             │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Timer HAL  │  │  GPIO HAL   │  │      RNG HAL        │  │
│  │             │  │             │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                      HARDWARE LAYER                         │
│            MCU Specific Implementation                       │
└─────────────────────────────────────────────────────────────┘
```

## 2. Core Components

### 2.1. Hardware Abstraction Layer (HAL)
```c
// Core interface definitions
typedef struct {
    int (*init)(void);
    int (*deinit)(void);
    int (*reset)(void);
} hal_base_t;

// USB HAL
typedef struct {
    hal_base_t base;
    int (*send_report)(uint8_t endpoint, uint8_t* data, size_t len);
    int (*receive_report)(uint8_t endpoint, uint8_t* buffer, size_t* len);
    int (*set_descriptor)(usb_descriptor_t* desc);
} usb_hal_t;

// Crypto HAL  
typedef struct {
    hal_base_t base;
    int (*generate_key_pair)(crypto_key_type_t type, crypto_key_t* public_key, crypto_key_t* private_key);
    int (*sign)(crypto_key_t* private_key, uint8_t* data, size_t data_len, uint8_t* signature, size_t* sig_len);
    int (*verify)(crypto_key_t* public_key, uint8_t* data, size_t data_len, uint8_t* signature, size_t sig_len);
    int (*random_bytes)(uint8_t* buffer, size_t len);
    int (*hash)(crypto_hash_type_t type, uint8_t* data, size_t len, uint8_t* hash);
} crypto_hal_t;

// Storage HAL
typedef struct {
    hal_base_t base;
    int (*read)(uint32_t address, uint8_t* buffer, size_t len);
    int (*write)(uint32_t address, uint8_t* data, size_t len);
    int (*erase)(uint32_t address, size_t len);
    int (*get_size)(void);
} storage_hal_t;
```

### 2.2. Protocol Layer
```c
// Generic protocol interface
typedef struct {
    int (*init)(void);
    int (*process_command)(uint8_t* cmd, size_t cmd_len, uint8_t* response, size_t* resp_len);
    int (*get_info)(protocol_info_t* info);
} protocol_interface_t;

// CTAP2 Protocol
typedef struct {
    protocol_interface_t base;
    int (*make_credential)(ctap2_make_credential_req_t* req, ctap2_make_credential_resp_t* resp);
    int (*get_assertion)(ctap2_get_assertion_req_t* req, ctap2_get_assertion_resp_t* resp);
    int (*get_info)(ctap2_info_t* info);
    int (*client_pin)(ctap2_client_pin_req_t* req, ctap2_client_pin_resp_t* resp);
} ctap2_protocol_t;
```

### 2.3. Application Layer
```c
// FIDO2 Application
typedef struct {
    int (*authenticator_make_credential)(webauthn_make_credential_req_t* req, webauthn_credential_t* cred);
    int (*authenticator_get_assertion)(webauthn_get_assertion_req_t* req, webauthn_assertion_t* assertion);
    int (*authenticator_get_info)(webauthn_authenticator_info_t* info);
    int (*set_pin)(uint8_t* pin, size_t pin_len);
    int (*verify_pin)(uint8_t* pin, size_t pin_len);
} fido2_app_t;
```

## 3. Configuration & Plugin System

### 3.1. Feature Configuration
```c
// config.h - Compile-time feature selection
#define FEATURE_FIDO2_ENABLED       1
#define FEATURE_PIV_ENABLED         0  // Disable for prototype
#define FEATURE_OTP_ENABLED         0  // Disable for prototype
#define FEATURE_OPENPGP_ENABLED     0  // Disable for prototype

#define FEATURE_USER_VERIFICATION   1  // PIN support
#define FEATURE_RESIDENT_KEYS       1  // Discoverable credentials
#define FEATURE_CREDENTIAL_MGMT     1  // Credential management

#define MAX_RESIDENT_KEYS          25
#define MAX_CREDENTIAL_SIZE        1024
```

### 3.2. Runtime Plugin Registration
```c
// Plugin registration system
typedef struct {
    char name[32];
    protocol_interface_t* protocol;
    void* context;
} protocol_plugin_t;

int register_protocol(protocol_plugin_t* plugin);
int unregister_protocol(const char* name);
protocol_interface_t* get_protocol(const char* name);
```

## 4. Threading & State Management

### 4.1. Event-Driven Architecture
```c
// Event system for handling USB/user interactions
typedef enum {
    EVENT_USB_DATA_RECEIVED,
    EVENT_USER_PRESENCE,
    EVENT_TIMER_EXPIRED,
    EVENT_CREDENTIAL_REQUEST,
} event_type_t;

typedef struct {
    event_type_t type;
    void* data;
    size_t data_len;
    void (*callback)(void* context);
    void* context;
} event_t;

int event_queue_push(event_t* event);
int event_queue_process(void);
```

### 4.2. State Machine
```c
// Global authenticator state
typedef enum {
    STATE_IDLE,
    STATE_PROCESSING_COMMAND,
    STATE_WAITING_USER_PRESENCE,
    STATE_WAITING_PIN_ENTRY,
    STATE_ERROR
} authenticator_state_t;

typedef struct {
    authenticator_state_t current_state;
    uint32_t timeout_ms;
    void (*timeout_callback)(void);
} state_machine_t;
```

## 5. Security Architecture

### 5.1. Key Management
```c
// Secure key storage
typedef struct {
    uint32_t key_id;
    crypto_key_type_t type;
    uint8_t usage_flags;
    uint8_t key_data[64];  // Encrypted/wrapped
    uint32_t creation_time;
} stored_key_t;

// Key derivation
int derive_attestation_key(uint8_t* master_secret, uint8_t* key_out);
int derive_credential_key(uint8_t* rp_id, uint8_t* user_id, uint8_t* key_out);
```

### 5.2. Access Control
```c
// PIN/User verification
typedef struct {
    uint8_t pin_hash[32];
    uint8_t pin_retries;
    bool pin_set;
    uint32_t consecutive_pin_failures;
} pin_state_t;

int verify_user_presence(void);
int verify_user_verification(uint8_t* pin, size_t pin_len);
```

## 6. Build System & Portability

### 6.1. Directory Structure
```
src/
├── hal/                 # Hardware Abstraction Layer
│   ├── interface/       # HAL interface definitions
│   ├── stm32/          # STM32-specific implementation
│   ├── esp32/          # ESP32-specific implementation
│   └── mock/           # Mock implementation for testing
├── protocol/           # Protocol implementations
│   ├── ctap2/
│   ├── iso7816/
│   └── yubico_otp/
├── app/               # Application layer
│   ├── fido2/
│   ├── piv/
│   └── otp/
├── transport/         # Transport layer (USB HID, CCID, etc.)
├── crypto/           # Cryptographic operations
├── storage/          # Persistent storage management
├── core/            # Core framework (events, state machine)
└── config/          # Configuration files
```

### 6.2. CMake Build System
```cmake
# Configurable build targets
option(TARGET_MCU "Target MCU family" "STM32F4")
option(ENABLE_FIDO2 "Enable FIDO2 support" ON)
option(ENABLE_PIV "Enable PIV support" OFF)
option(ENABLE_DEBUG "Enable debug features" OFF)

# Platform-specific HAL selection
if(TARGET_MCU STREQUAL "STM32F4")
    add_subdirectory(src/hal/stm32)
elseif(TARGET_MCU STREQUAL "ESP32")
    add_subdirectory(src/hal/esp32)
endif()
```

## 7. Testing Strategy

### 7.1. Unit Testing
- Mock HAL implementations
- Protocol compliance tests
- Cryptographic operation tests
- State machine testing

### 7.2. Integration Testing
- End-to-end FIDO2 flows
- USB communication testing
- Multi-protocol interaction

### 7.3. Compliance Testing
- FIDO2 conformance tests
- WebAuthn compatibility testing
- USB certification tests

## Kết Luận

Framework này được thiết kế để:
1. **Prototype nhanh**: Bắt đầu với FIDO2 only, dễ dàng disable các features khác
2. **Mở rộng dễ dàng**: Thêm protocols mới thông qua plugin system
3. **Portable**: HAL layer cho phép chuyển đổi MCU
4. **Production-ready**: Architecture đã chuẩn bị cho security và performance requirements

Bước tiếp theo: Implement core framework và FIDO2 basic functionality.
