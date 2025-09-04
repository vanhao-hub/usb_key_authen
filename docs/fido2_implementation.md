
# FIDO2/WebAuthn Implementation Guide

## Tổng Quan Kiến Trúc FIDO2/WebAuthn (theo mô hình mới)

FIDO2/WebAuthn được triển khai theo kiến trúc phân tầng hiện đại:
- **Application Layer (APP Layer):** Xử lý logic nghiệp vụ xác thực (FIDO2/WebAuthn, OTP, PIV, ...).
- **Platform Layer:** Cung cấp các thành phần nền tảng như Crypto, Storage, Communication (gồm Protocol Layer: CTAP2, ISO7816, ... và Transport Layer: FIDO HID, CCID, ...).
- **Hardware Abstraction Layer (HAL):** API thống nhất cho phần cứng (USB HID HAL, Crypto HAL, Storage HAL).


## 1. CTAP2 Protocol Implementation (Platform Layer - Protocol)

### 1.1. CTAP2 Commands (Protocol Layer)

#### Mandatory Commands:
```c
#define CTAP2_AUTHENTICATOR_MAKE_CREDENTIAL     0x01
#define CTAP2_AUTHENTICATOR_GET_ASSERTION       0x02  
#define CTAP2_AUTHENTICATOR_GET_INFO            0x04
#define CTAP2_AUTHENTICATOR_CLIENT_PIN          0x06
#define CTAP2_AUTHENTICATOR_RESET               0x07
#define CTAP2_AUTHENTICATOR_GET_NEXT_ASSERTION  0x08

// Optional for enhanced functionality
#define CTAP2_AUTHENTICATOR_CREDENTIAL_MGMT     0x0A
#define CTAP2_AUTHENTICATOR_SELECTION          0x0B
#define CTAP2_AUTHENTICATOR_LARGE_BLOBS         0x0C
#define CTAP2_AUTHENTICATOR_CONFIG             0x0D
```

#### CBOR Data Format:
```c
// CTAP2 sử dụng CBOR (Concise Binary Object Representation)
// Cần implement CBOR encoder/decoder

typedef struct {
    uint8_t* buffer;
    size_t length;
    size_t capacity;
} cbor_buffer_t;

int cbor_encode_map_start(cbor_buffer_t* buf, size_t items);
int cbor_encode_string(cbor_buffer_t* buf, const char* str);
int cbor_encode_bytes(cbor_buffer_t* buf, uint8_t* data, size_t len);
int cbor_encode_uint(cbor_buffer_t* buf, uint64_t value);
```

### 1.2. MakeCredential Implementation

```c
// Input parameters (từ browser qua CBOR)
typedef struct {
    uint8_t client_data_hash[32];           // SHA-256 của client data
    rp_entity_t rp;                         // Relying Party info
    user_entity_t user;                     // User info  
    pub_key_cred_params_t* cred_params;     // Supported algorithms
    size_t cred_params_count;
    uint8_t** exclude_list;                 // Credentials to exclude
    size_t exclude_list_count;
    extensions_t* extensions;               // Optional extensions
    uint8_t options;                        // rk, uv flags
    uint8_t* pin_auth;                      // PIN authentication
    uint8_t pin_protocol;                   // PIN protocol version
} ctap2_make_credential_req_t;

// Output response
typedef struct {
    uint8_t fmt[32];                        // Attestation format ("packed")
    uint8_t* auth_data;                     // Authenticator data
    size_t auth_data_len;
    uint8_t* att_stmt;                      // Attestation statement (CBOR)
    size_t att_stmt_len;
} ctap2_make_credential_resp_t;

// Implementation steps:
int ctap2_make_credential(ctap2_make_credential_req_t* req, ctap2_make_credential_resp_t* resp) {
    // 1. Validate input parameters
    if (!validate_make_credential_params(req)) {
        return CTAP2_ERR_INVALID_PARAMETER;
    }
    
    // 2. Check if credential exists in exclude list
    if (check_exclude_list(req->exclude_list, req->exclude_list_count)) {
        return CTAP2_ERR_CREDENTIAL_EXCLUDED;
    }
    
    // 3. Verify user presence (button press)
    if (!verify_user_presence()) {
        return CTAP2_ERR_USER_ACTION_TIMEOUT;
    }
    
    // 4. Verify PIN if required
    if (req->options & CTAP2_OPTION_UV) {
        if (!verify_pin_auth(req->pin_auth, req->pin_protocol)) {
            return CTAP2_ERR_PIN_AUTH_INVALID;
        }
    }
    
    // 5. Generate new key pair
    crypto_key_t public_key, private_key;
    if (generate_credential_key_pair(req->rp.id, req->user.id, &public_key, &private_key) != 0) {
        return CTAP2_ERR_KEY_STORE_FULL;
    }
    
    // 6. Create credential ID
    uint8_t credential_id[64];
    size_t cred_id_len;
    if (create_credential_id(&private_key, req->rp.id, credential_id, &cred_id_len) != 0) {
        return CTAP2_ERR_OTHER;
    }
    
    // 7. Build authenticator data
    uint8_t auth_data[256];
    size_t auth_data_len;
    if (build_authenticator_data(req->rp.id, &public_key, credential_id, cred_id_len, 
                                auth_data, &auth_data_len) != 0) {
        return CTAP2_ERR_OTHER;
    }
    
    // 8. Create attestation
    uint8_t att_stmt[256];  
    size_t att_stmt_len;
    if (create_attestation(auth_data, auth_data_len, req->client_data_hash, 
                          att_stmt, &att_stmt_len) != 0) {
        return CTAP2_ERR_OTHER;
    }
    
    // 9. Store credential if resident key requested
    if (req->options & CTAP2_OPTION_RK) {
        if (store_resident_credential(&private_key, req->rp.id, req->user.id, 
                                     credential_id, cred_id_len) != 0) {
            return CTAP2_ERR_KEY_STORE_FULL;
        }
    }
    
    // 10. Build response
    strcpy(resp->fmt, "packed");
    resp->auth_data = malloc(auth_data_len);
    memcpy(resp->auth_data, auth_data, auth_data_len);
    resp->auth_data_len = auth_data_len;
    resp->att_stmt = malloc(att_stmt_len);
    memcpy(resp->att_stmt, att_stmt, att_stmt_len);
    resp->att_stmt_len = att_stmt_len;
    
    return CTAP2_ERR_SUCCESS;
}
```

### 1.3. GetAssertion Implementation

```c
typedef struct {
    uint8_t rp_id[256];                     // Relying Party ID
    uint8_t client_data_hash[32];           // SHA-256 của client data
    uint8_t** allow_list;                   // Allowed credentials
    size_t allow_list_count;
    extensions_t* extensions;               // Optional extensions
    uint8_t options;                        // up, uv flags
    uint8_t* pin_auth;                      // PIN authentication
    uint8_t pin_protocol;                   // PIN protocol version
} ctap2_get_assertion_req_t;

typedef struct {
    uint8_t* credential_id;                 // Credential identifier
    size_t credential_id_len;
    uint8_t* auth_data;                     // Authenticator data
    size_t auth_data_len;
    uint8_t* signature;                     // Assertion signature
    size_t signature_len;
    user_entity_t* user;                    // User info (for resident keys)
    uint8_t number_of_credentials;          // Total credentials found
} ctap2_get_assertion_resp_t;

int ctap2_get_assertion(ctap2_get_assertion_req_t* req, ctap2_get_assertion_resp_t* resp) {
    // 1. Validate parameters
    if (!validate_get_assertion_params(req)) {
        return CTAP2_ERR_INVALID_PARAMETER;
    }
    
    // 2. Find matching credentials
    credential_t* matching_creds[MAX_CREDENTIALS];
    size_t cred_count;
    
    if (req->allow_list_count > 0) {
        // Use allow list
        cred_count = find_credentials_by_allow_list(req->rp_id, req->allow_list, 
                                                   req->allow_list_count, matching_creds);
    } else {
        // Search resident keys
        cred_count = find_resident_credentials(req->rp_id, matching_creds);
    }
    
    if (cred_count == 0) {
        return CTAP2_ERR_NO_CREDENTIALS;
    }
    
    // 3. Verify user presence
    if (!verify_user_presence()) {
        return CTAP2_ERR_USER_ACTION_TIMEOUT;
    }
    
    // 4. Verify PIN if required
    if (req->options & CTAP2_OPTION_UV) {
        if (!verify_pin_auth(req->pin_auth, req->pin_protocol)) {
            return CTAP2_ERR_PIN_AUTH_INVALID;
        }
    }
    
    // 5. Use first matching credential for response
    credential_t* cred = matching_creds[0];
    
    // 6. Build authenticator data
    uint8_t auth_data[256];
    size_t auth_data_len;
    if (build_assertion_auth_data(req->rp_id, auth_data, &auth_data_len) != 0) {
        return CTAP2_ERR_OTHER;
    }
    
    // 7. Create signature
    uint8_t signature[64];
    size_t sig_len;
    uint8_t data_to_sign[512];
    size_t data_len = 0;
    
    memcpy(data_to_sign + data_len, auth_data, auth_data_len);
    data_len += auth_data_len;
    memcpy(data_to_sign + data_len, req->client_data_hash, 32);
    data_len += 32;
    
    if (sign_data(cred->private_key, data_to_sign, data_len, signature, &sig_len) != 0) {
        return CTAP2_ERR_OTHER;
    }
    
    // 8. Build response
    resp->credential_id = malloc(cred->credential_id_len);
    memcpy(resp->credential_id, cred->credential_id, cred->credential_id_len);
    resp->credential_id_len = cred->credential_id_len;
    
    resp->auth_data = malloc(auth_data_len);
    memcpy(resp->auth_data, auth_data, auth_data_len);
    resp->auth_data_len = auth_data_len;
    
    resp->signature = malloc(sig_len);
    memcpy(resp->signature, signature, sig_len);
    resp->signature_len = sig_len;
    
    resp->number_of_credentials = cred_count;
    
    return CTAP2_ERR_SUCCESS;
}
```

## 2. Authenticator Data Format

### 2.1. Authenticator Data Structure
```c
typedef struct __attribute__((packed)) {
    uint8_t rp_id_hash[32];                 // SHA-256(rp_id)
    uint8_t flags;                          // UP, UV, AT, ED flags
    uint32_t sign_count;                    // Signature counter (big-endian)
    // Optional: Attested credential data (for MakeCredential)
    // Optional: Extensions (CBOR)
} authenticator_data_t;

// Flags
#define AUTHDATA_FLAG_UP    0x01    // User Present
#define AUTHDATA_FLAG_UV    0x04    // User Verified  
#define AUTHDATA_FLAG_AT    0x40    // Attested credential data included
#define AUTHDATA_FLAG_ED    0x80    // Extension data included
```

### 2.2. Attested Credential Data (cho MakeCredential)
```c
typedef struct __attribute__((packed)) {
    uint8_t aaguid[16];                     // Authenticator AAGUID
    uint16_t credential_id_length;          // Big-endian
    // uint8_t credential_id[credential_id_length];
    // CBOR encoded credential public key
} attested_credential_data_t;
```

## 3. Cryptographic Requirements

### 3.1. Key Generation
```c
// ES256 (ECDSA with P-256 and SHA-256)
int generate_es256_key_pair(crypto_key_t* public_key, crypto_key_t* private_key) {
    // Generate P-256 key pair
    // Public key format: COSE Key (CBOR)
    // Private key: store securely, never expose
}

// COSE Key format for public key
typedef struct {
    int kty;        // Key type (2 = EC2)
    int alg;        // Algorithm (-7 = ES256)
    int crv;        // Curve (1 = P-256)
    uint8_t x[32];  // X coordinate
    uint8_t y[32];  // Y coordinate
} cose_ec2_key_t;
```

### 3.2. Signature Creation
```c
int create_attestation_signature(uint8_t* auth_data, size_t auth_data_len,
                                uint8_t* client_data_hash,
                                crypto_key_t* attestation_key,
                                uint8_t* signature, size_t* sig_len) {
    // Concatenate authData || clientDataHash
    uint8_t data_to_sign[256 + 32];
    memcpy(data_to_sign, auth_data, auth_data_len);
    memcpy(data_to_sign + auth_data_len, client_data_hash, 32);
    
    // Sign with ECDSA P-256 SHA-256
    return ecdsa_p256_sign(attestation_key, data_to_sign, 
                          auth_data_len + 32, signature, sig_len);
}
```

## 4. Storage Requirements

### 4.1. Persistent Data
```c
// Attestation key (device-wide)
typedef struct {
    crypto_key_t private_key;
    uint8_t aaguid[16];
    uint32_t sign_count;
} attestation_data_t;

// Resident credentials
typedef struct {
    uint8_t credential_id[64];
    size_t credential_id_len;
    crypto_key_t private_key;
    uint8_t rp_id[256];
    uint8_t user_id[64];
    size_t user_id_len;
    char user_name[64];
    char user_display_name[64];
    uint32_t creation_time;
} resident_credential_t;

// PIN data
typedef struct {
    uint8_t pin_hash[32];
    uint8_t retries_left;
    bool pin_set;
    uint32_t consecutive_failures;
} pin_data_t;
```

### 4.2. Storage Layout
```c
// Flash layout
#define STORAGE_ATTESTATION_OFFSET      0x0000
#define STORAGE_PIN_OFFSET             0x0100  
#define STORAGE_RESIDENT_CREDS_OFFSET  0x0200
#define STORAGE_COUNTERS_OFFSET        0x8000

#define MAX_RESIDENT_CREDENTIALS       25
#define RESIDENT_CRED_SIZE            sizeof(resident_credential_t)
```


## 5. USB HID Transport (Platform Layer - Transport)

### 5.1. FIDO HID Protocol (Transport Layer)
```c
// HID Report Descriptor for FIDO HID (chuẩn giao tiếp vật lý)
static const uint8_t fido_hid_report_descriptor[] = {
    0x06, 0xD0, 0xF1,    // Usage Page (FIDO Alliance)
    0x09, 0x01,          // Usage (U2F HID Authenticator Device)
    0xA1, 0x01,          // Collection (Application)
    0x09, 0x20,          //   Usage (Input Report Data)
    0x15, 0x00,          //   Logical Minimum (0)
    0x26, 0xFF, 0x00,    //   Logical Maximum (255)
    0x75, 0x08,          //   Report Size (8)
    0x95, 0x40,          //   Report Count (64)
    0x81, 0x02,          //   Input (Data,Var,Abs)
    0x09, 0x21,          //   Usage (Output Report Data)
    0x15, 0x00,          //   Logical Minimum (0)
    0x26, 0xFF, 0x00,    //   Logical Maximum (255)
    0x75, 0x08,          //   Report Size (8)
    0x95, 0x40,          //   Report Count (64)
    0x91, 0x02,          //   Output (Data,Var,Abs)
    0xC0                 // End Collection
};

#define FIDO_HID_PACKET_SIZE    64
#define FIDO_HID_INIT_CMD       0x86
#define FIDO_HID_MSG_CMD        0x83
```

### 5.2. Packet Handling (Transport Layer)
```c
typedef struct {
    uint32_t channel_id;
    uint8_t command;
    uint8_t bcnt_h;
    uint8_t bcnt_l;
    uint8_t data[FIDO_HID_PACKET_SIZE - 7];
} fido_hid_init_packet_t;

typedef struct {
    uint32_t channel_id;
    uint8_t sequence;
    uint8_t data[FIDO_HID_PACKET_SIZE - 5];
} fido_hid_cont_packet_t;

int process_fido_hid_packet(uint8_t* packet, size_t packet_len);
int send_fido_response(uint32_t channel_id, uint8_t* response, size_t resp_len);
```

## 6. Testing & Compliance

### 6.1. FIDO2 Conformance Tests
- FIDO Alliance cung cấp conformance test suite
- Cần pass các test cases cho Level 1 certification
- Test với các browser: Chrome, Firefox, Edge

### 6.2. WebAuthn Testing
```javascript
// Browser-side test
navigator.credentials.create({
    publicKey: {
        challenge: new Uint8Array(32),
        rp: { name: "Test RP", id: "localhost" },
        user: {
            id: new Uint8Array(16),
            name: "testuser",
            displayName: "Test User"
        },
        pubKeyCredParams: [{ alg: -7, type: "public-key" }],
        authenticatorSelection: {
            authenticatorAttachment: "cross-platform",
            userVerification: "preferred"
        }
    }
});
```

## Kết Luận


## Kết luận

Triển khai FIDO2/WebAuthn theo kiến trúc phân tầng:
1. **Application Layer:** Xử lý logic xác thực (MakeCredential, GetAssertion, ...)
2. **Platform Layer:**
    - **Protocol Layer:** CTAP2, ISO7816, Yubico OTP, ...
    - **Transport Layer:** FIDO HID, CCID, BLE, ...
    - **Crypto/Storage:** Sinh key, ký số, lưu trữ an toàn
3. **HAL:** API phần cứng (USB HID HAL, Crypto HAL, Storage HAL)

Prototype nên tập trung vào core functionality:
- MakeCredential & GetAssertion
- Basic attestation
- User presence detection
- FIDO HID communication
