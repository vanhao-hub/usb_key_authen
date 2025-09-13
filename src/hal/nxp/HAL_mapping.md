# HAL USB HID Configuration Mapping

## Overview
This document provides a comprehensive mapping between the platform-independent `usb_hid_descriptor_t` structure and the NXP USB stack configuration points in `usb_device_descriptor.c`. This mapping enables dynamic configuration of USB descriptors through the HAL layer.

## Platform Descriptor Structure
```c
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
```

## Configuration Mapping Table

| Platform Field | NXP Target Structure | Byte Position | Current Value | Data Format | Configuration Method |
|----------------|---------------------|---------------|---------------|-------------|---------------------|
| **Device Identity (g_UsbDeviceDescriptor)** |
| `vendor_id` | `g_UsbDeviceDescriptor[14-15]` | Bytes 14-15 | `0xC9 0x1F` | Little-endian uint16 | Direct byte modification |
| `product_id` | `g_UsbDeviceDescriptor[16-17]` | Bytes 16-17 | `0xA2 0x00` | Little-endian uint16 | Direct byte modification |
| `device_version` | `g_UsbDeviceDescriptor[18-19]` | Bytes 18-19 | `0x01 0x01` | Little-endian BCD | Direct byte modification |
| **String Descriptors** |
| `manufacturer_string` | `g_UsbDeviceString1[]` | Entire array | UTF-16LE: "NXP SEMICONDUCTORS" | Length prefix + UTF-16LE | Complete array replacement |
| `product_string` | `g_UsbDeviceString2[]` | Entire array | UTF-16LE: "HID GENERIC DEVICE" | Length prefix + UTF-16LE | Complete array replacement |
| `serial_string` | `g_UsbDeviceString3[]` | Entire array | UTF-16LE: "000012345678900000" | Length prefix + UTF-16LE | Complete array replacement |
| **HID Report Descriptor** |
| `report_descriptor` | `g_UsbDeviceHidGenericReportDescriptor[]` | Entire array | Vendor HID (25 bytes) | Binary HID descriptor | Complete array replacement |
| `report_descriptor_size` | Referenced by multiple locations | Multiple | `sizeof(g_UsbDeviceHidGenericReportDescriptor)` | Size calculation | Auto-calculated after replacement |
| **Configuration Descriptor Packet Sizes** |
| IN endpoint packet size | `g_UsbDeviceConfigurationDescriptor[50-51]` | Bytes 50-51 | `0x08 0x00` (8 bytes) | Little-endian uint16 | Direct byte modification |
| OUT endpoint packet size | `g_UsbDeviceConfigurationDescriptor[57-58]` | Bytes 57-58 | `0x08 0x00` (8 bytes) | Little-endian uint16 | Direct byte modification |
| **Endpoint Structure Arrays** |
| IN endpoint max packet | `g_UsbDeviceHidGenericEndpoints[0].maxPacketSize` | Struct member | `8` | uint16 value | Direct struct member |
| OUT endpoint max packet | `g_UsbDeviceHidGenericEndpoints[1].maxPacketSize` | Struct member | `8` | uint16 value | Direct struct member |

## Buffer Size Configuration Points

| Configuration Point | Target Structure | Byte/Member Position | Current Value | FIDO2 Requirement | Modification Method |
|---------------------|------------------|---------------------|---------------|-------------------|-------------------|
| **Header Macro Constants (Compile-time)** |
| `FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| `FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| `HS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| `HS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| `USB_HID_GENERIC_IN_BUFFER_LENGTH` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| `USB_HID_GENERIC_OUT_BUFFER_LENGTH` | Macro definition | Header file | `8U` | `64U` | Static macro change |
| **Runtime Structure Members** |
| IN endpoint packet size | `g_UsbDeviceHidGenericEndpoints[0].maxPacketSize` | Struct member | `8` | `64` | Direct assignment |
| OUT endpoint packet size | `g_UsbDeviceHidGenericEndpoints[1].maxPacketSize` | Struct member | `8` | `64` | Direct assignment |
| **Configuration Descriptor Binary Data** |
| IN endpoint descriptor | `g_UsbDeviceConfigurationDescriptor[50-51]` | Bytes 50-51 | `0x08 0x00` | `0x40 0x00` | Direct byte write |
| OUT endpoint descriptor | `g_UsbDeviceConfigurationDescriptor[57-58]` | Bytes 57-58 | `0x08 0x00` | `0x40 0x00` | Direct byte write |

## Detailed Mapping Analysis

### 1. Device Descriptor Structure (g_UsbDeviceDescriptor)
```c
// USB Device Descriptor Layout (18 bytes total)
g_UsbDeviceDescriptor[0]     = 0x12              // bLength 
g_UsbDeviceDescriptor[1]     = 0x01              // bDescriptorType (DEVICE)
g_UsbDeviceDescriptor[2-3]   = USB_DEVICE_SPECIFIC_BCD_VERSION  // bcdUSB
g_UsbDeviceDescriptor[4]     = USB_DEVICE_CLASS   // bDeviceClass
g_UsbDeviceDescriptor[5]     = USB_DEVICE_SUBCLASS // bDeviceSubClass  
g_UsbDeviceDescriptor[6]     = USB_DEVICE_PROTOCOL // bDeviceProtocol
g_UsbDeviceDescriptor[7]     = USB_CONTROL_MAX_PACKET_SIZE // bMaxPacketSize0
g_UsbDeviceDescriptor[8-9]   = USB_DEVICE_VID     // idVendor (little-endian)
g_UsbDeviceDescriptor[10-11] = USB_DEVICE_PID     // idProduct (little-endian) 
g_UsbDeviceDescriptor[12-13] = USB_DEVICE_DEMO_BCD_VERSION // bcdDevice
g_UsbDeviceDescriptor[14]    = 0x01               // iManufacturer (string index)
g_UsbDeviceDescriptor[15]    = 0x02               // iProduct (string index)
g_UsbDeviceDescriptor[16]    = 0x00/0x03          // iSerialNumber (conditional)
g_UsbDeviceDescriptor[17]    = USB_DEVICE_CONFIGURATION_COUNT // bNumConfigurations

// HAL Configuration Targets:
// vendor_id -> Bytes 8-9 (little-endian)
// product_id -> Bytes 10-11 (little-endian)  
// device_version -> Bytes 12-13 (little-endian BCD)
```

### 2. String Descriptor Arrays
```c
// g_UsbDeviceString1[] (Manufacturer) - Current: "NXP SEMICONDUCTORS"
g_UsbDeviceString1[0] = 2U + 2U * 18U           // Total length (38 bytes)
g_UsbDeviceString1[1] = USB_DESCRIPTOR_TYPE_STRING // Descriptor type (0x03)
g_UsbDeviceString1[2-37] = UTF-16LE characters   // "NXP SEMICONDUCTORS" in UTF-16LE

// g_UsbDeviceString2[] (Product) - Current: "HID GENERIC DEVICE"  
g_UsbDeviceString2[0] = 2U + 2U * 18U           // Total length (38 bytes)
g_UsbDeviceString2[1] = USB_DESCRIPTOR_TYPE_STRING // Descriptor type (0x03)
g_UsbDeviceString2[2-37] = UTF-16LE characters   // "HID GENERIC DEVICE" in UTF-16LE

// g_UsbDeviceString3[] (Serial) - Current: "000012345678900000"
g_UsbDeviceString3[0] = 16 * 2 + 2              // Total length (34 bytes)
g_UsbDeviceString3[1] = USB_DESCRIPTOR_TYPE_STRING // Descriptor type (0x03)
g_UsbDeviceString3[2-33] = UTF-16LE characters   // Serial number in UTF-16LE

// HAL Configuration: Complete array replacement with proper UTF-16LE encoding
```

### 3. HID Report Descriptor (g_UsbDeviceHidGenericReportDescriptor)
```c
// Current NXP HID Report Descriptor (25 bytes) - Vendor specific with 8-byte reports
static const uint8_t current_descriptor[25] = {
    0x05, 0x81,  // Usage Page (Vendor defined)
    0x09, 0x82,  // Usage (Vendor defined) 
    0xA1, 0x01,  // Collection (Application)
    0x09, 0x83,  // Usage (Vendor defined)
    0x09, 0x84,  // Usage (Vendor defined)
    0x15, 0x80,  // Logical Minimum (-128)
    0x25, 0x7F,  // Logical Maximum (127) 
    0x75, 0x08,  // Report Size (8 bits)
    0x95, 0x08,  // Report Count (8) -> **8-byte reports**
    0x81, 0x02,  // Input(Data, Variable, Absolute)
    0x09, 0x84,  // Usage (Vendor defined)
    0x15, 0x80,  // Logical Minimum (-128)
    0x25, 0x7F,  // Logical Maximum (127)
    0x75, 0x08,  // Report Size (8 bits) 
    0x95, 0x08,  // Report Count (8) -> **8-byte reports**
    0x91, 0x02,  // Output(Data, Variable, Absolute)
    0xC0         // End collection
};

// FIDO2 Required HID Report Descriptor (from fido_hid_transport.c)
static const uint8_t fido_hid_report_descriptor[] = {
    0x06, 0xD0, 0xF1,    // Usage Page (FIDO Alliance)
    0x09, 0x01,          // Usage (U2F HID Authenticator Device)
    0xA1, 0x01,          // Collection (Application)
    0x09, 0x20,          // Usage (Input Report Data)
    0x15, 0x00,          // Logical Minimum (0)
    0x26, 0xFF, 0x00,    // Logical Maximum (255)
    0x75, 0x08,          // Report Size (8 bits)
    0x95, 0x40,          // Report Count (64) -> **64-byte reports**
    0x81, 0x02,          // Input (Data, Variable, Absolute)
    0x09, 0x21,          // Usage (Output Report Data)
    0x15, 0x00,          // Logical Minimum (0)
    0x26, 0xFF, 0x00,    // Logical Maximum (255)
    0x75, 0x08,          // Report Size (8 bits)
    0x95, 0x40,          // Report Count (64) -> **64-byte reports**
    0x91, 0x02,          // Output (Data, Variable, Absolute)
    0xC0                 // End Collection
};

// HAL Configuration: Complete replacement of g_UsbDeviceHidGenericReportDescriptor[]
```

### 4. Configuration Descriptor Packet Size Positions
```c
// g_UsbDeviceConfigurationDescriptor[] - Binary descriptor data
// Total length: USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL

// Configuration Header (9 bytes) + Interface (9 bytes) + HID (9 bytes) = 27 bytes
// Then Endpoint Descriptors start:

// IN Endpoint Descriptor (7 bytes starting at offset ~41):
// Offset 41: USB_DESCRIPTOR_LENGTH_ENDPOINT (7)
// Offset 42: USB_DESCRIPTOR_TYPE_ENDPOINT (5) 
// Offset 43: USB_HID_GENERIC_ENDPOINT_IN | USB_IN (0x81)
// Offset 44: USB_ENDPOINT_INTERRUPT (3)
// Offset 45-46: wMaxPacketSize (little-endian) -> **TARGET for packet size**
// Offset 47: bInterval

// OUT Endpoint Descriptor (7 bytes starting at offset ~48):
// Offset 48: USB_DESCRIPTOR_LENGTH_ENDPOINT (7)
// Offset 49: USB_DESCRIPTOR_TYPE_ENDPOINT (5)
// Offset 50: USB_HID_GENERIC_ENDPOINT_OUT | USB_OUT (0x02) 
// Offset 51: USB_ENDPOINT_INTERRUPT (3)
// Offset 52-53: wMaxPacketSize (little-endian) -> **TARGET for packet size**
// Offset 54: bInterval

// HAL Configuration: Direct byte modification at specific offsets
```

### 5. Endpoint Structure Array (g_UsbDeviceHidGenericEndpoints)
```c
// Runtime endpoint configuration structures
usb_device_endpoint_struct_t g_UsbDeviceHidGenericEndpoints[2] = {
    // [0] IN endpoint
    {
        .endpointAddress = USB_HID_GENERIC_ENDPOINT_IN | USB_IN,
        .transferType = USB_ENDPOINT_INTERRUPT,
        .maxPacketSize = FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE,  // **TARGET**
        .interval = FS_HID_GENERIC_INTERRUPT_IN_INTERVAL
    },
    // [1] OUT endpoint  
    {
        .endpointAddress = USB_HID_GENERIC_ENDPOINT_OUT | USB_OUT,
        .transferType = USB_ENDPOINT_INTERRUPT, 
        .maxPacketSize = FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE, // **TARGET**
        .interval = FS_HID_GENERIC_INTERRUPT_OUT_INTERVAL
    }
};

// HAL Configuration: Direct struct member assignment
```

## Implementation Strategy

### Phase 1: Static Configuration (Recommended)
Modify header files and recompile:
```c
// usb_device_descriptor.h
#define USB_DEVICE_VID 0x1234  // From descriptor->vendor_id
#define USB_DEVICE_PID 0x5678  // From descriptor->product_id
#define FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE  (64U)
#define FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE (64U)
#define USB_HID_GENERIC_IN_BUFFER_LENGTH  (64U)
#define USB_HID_GENERIC_OUT_BUFFER_LENGTH (64U)
```

### Phase 2: Dynamic Configuration (Advanced)
Runtime modification of descriptor arrays:
```c
// Modify g_UsbDeviceDescriptor at runtime
g_UsbDeviceDescriptor[14] = USB_SHORT_GET_LOW(descriptor->vendor_id);
g_UsbDeviceDescriptor[15] = USB_SHORT_GET_HIGH(descriptor->vendor_id);

// Replace string descriptors
// Replace report descriptor
// Update configuration descriptor packet sizes
```

## Configuration Functions

## Configuration Functions

### Required HAL Implementation
```c
hal_result_t usb_hid_hal_nxp_configure(const usb_hid_descriptor_t* descriptor) {
    if (!descriptor) return HAL_ERROR_INVALID_PARAM;
    
    // 1. Update Device Descriptor (g_UsbDeviceDescriptor)
    g_UsbDeviceDescriptor[8]  = (descriptor->vendor_id) & 0xFF;        // idVendor low
    g_UsbDeviceDescriptor[9]  = (descriptor->vendor_id >> 8) & 0xFF;   // idVendor high
    g_UsbDeviceDescriptor[10] = (descriptor->product_id) & 0xFF;       // idProduct low  
    g_UsbDeviceDescriptor[11] = (descriptor->product_id >> 8) & 0xFF;  // idProduct high
    g_UsbDeviceDescriptor[12] = (descriptor->device_version) & 0xFF;   // bcdDevice low
    g_UsbDeviceDescriptor[13] = (descriptor->device_version >> 8) & 0xFF; // bcdDevice high
    
    // 2. Replace String Descriptors
    replace_string_descriptor(1, descriptor->manufacturer_string);  // g_UsbDeviceString1
    replace_string_descriptor(2, descriptor->product_string);       // g_UsbDeviceString2  
    replace_string_descriptor(3, descriptor->serial_string);        // g_UsbDeviceString3
    
    // 3. Replace HID Report Descriptor
    replace_hid_report_descriptor(descriptor->report_descriptor, 
                                 descriptor->report_descriptor_size);
    
    // 4. Update Configuration Descriptor packet sizes (if 64-byte packets)
    if (requires_64_byte_packets(descriptor)) {
        update_configuration_descriptor_packet_sizes(64);
        update_endpoint_structures_packet_sizes(64);
    }
    
    // 5. Update string descriptor length arrays
    update_string_length_arrays();
    
    return HAL_SUCCESS;
}

// Helper function implementations
static void replace_string_descriptor(uint8_t index, const char* str) {
    // Convert ASCII to UTF-16LE format with length prefix
    // Update g_UsbDeviceString1[], g_UsbDeviceString2[], or g_UsbDeviceString3[]
}

static void replace_hid_report_descriptor(const uint8_t* desc, size_t size) {
    // Replace g_UsbDeviceHidGenericReportDescriptor[] completely
    // Update size references
}

static void update_configuration_descriptor_packet_sizes(uint16_t packet_size) {
    // Update bytes 45-46 for IN endpoint (little-endian)
    g_UsbDeviceConfigurationDescriptor[45] = packet_size & 0xFF;
    g_UsbDeviceConfigurationDescriptor[46] = (packet_size >> 8) & 0xFF;
    
    // Update bytes 52-53 for OUT endpoint (little-endian)  
    g_UsbDeviceConfigurationDescriptor[52] = packet_size & 0xFF;
    g_UsbDeviceConfigurationDescriptor[53] = (packet_size >> 8) & 0xFF;
}

static void update_endpoint_structures_packet_sizes(uint16_t packet_size) {
    // Update runtime endpoint structures
    g_UsbDeviceHidGenericEndpoints[0].maxPacketSize = packet_size; // IN endpoint
    g_UsbDeviceHidGenericEndpoints[1].maxPacketSize = packet_size; // OUT endpoint
}
```

### Buffer Size Validation
```c
static bool requires_64_byte_packets(const usb_hid_descriptor_t* descriptor) {
    // Check if HID report descriptor contains 64-byte report specifications
    // Parse descriptor for Report Count (0x95) with value 0x40 (64)
    
    const uint8_t* desc = descriptor->report_descriptor;
    size_t size = descriptor->report_descriptor_size;
    
    for (size_t i = 0; i < size - 1; i++) {
        if (desc[i] == 0x95 && desc[i+1] == 0x40) {  // Report Count (64)
            return true;
        }
    }
    return false;
}

static hal_result_t validate_buffer_compatibility(const usb_hid_descriptor_t* descriptor) {
    if (requires_64_byte_packets(descriptor)) {
        // Ensure all packet size constants support 64 bytes
        // This may require compile-time macro changes for full compatibility
        return HAL_SUCCESS; // Will be handled by runtime updates
    }
    return HAL_SUCCESS;
}
```

## Critical Configuration Points

### 🔴 **CRITICAL: Buffer Size Mismatch**
The current NXP configuration uses 8-byte packets while FIDO2 requires 64-byte packets. This must be resolved through:

1. **Static approach**: Change macros and recompile
2. **Dynamic approach**: Runtime modification of descriptors and structures

### 🟡 **Important: String Descriptor Format**
NXP uses UTF-16LE format with length prefixes. String replacement must maintain this format.

### 🟡 **Important: Descriptor Consistency**
All packet size references must be updated consistently across:
- Header macros
- Descriptor arrays  
- Runtime structures
- Endpoint configurations

## Validation Checklist

### Direct Byte Modifications Required:
- [ ] `g_UsbDeviceDescriptor[8-9]`: Vendor ID (little-endian)
- [ ] `g_UsbDeviceDescriptor[10-11]`: Product ID (little-endian)  
- [ ] `g_UsbDeviceDescriptor[12-13]`: Device version (little-endian BCD)
- [ ] `g_UsbDeviceString1[]`: Complete array replacement with UTF-16LE manufacturer string
- [ ] `g_UsbDeviceString2[]`: Complete array replacement with UTF-16LE product string
- [ ] `g_UsbDeviceString3[]`: Complete array replacement with UTF-16LE serial string
- [ ] `g_UsbDeviceHidGenericReportDescriptor[]`: Complete replacement with FIDO2 descriptor
- [ ] `g_UsbDeviceConfigurationDescriptor[45-46]`: IN endpoint packet size (little-endian)
- [ ] `g_UsbDeviceConfigurationDescriptor[52-53]`: OUT endpoint packet size (little-endian)
- [ ] `g_UsbDeviceHidGenericEndpoints[0].maxPacketSize`: IN endpoint struct member
- [ ] `g_UsbDeviceHidGenericEndpoints[1].maxPacketSize`: OUT endpoint struct member
- [ ] `g_UsbDeviceStringDescriptorLength[]`: Update length array after string changes
- [ ] Update HID descriptor length reference after report descriptor change

### Macro Updates (Static Configuration):
- [ ] `USB_DEVICE_VID` macro (if using static approach)
- [ ] `USB_DEVICE_PID` macro (if using static approach)  
- [ ] `USB_DEVICE_DEMO_BCD_VERSION` macro (if using static approach)
- [ ] `FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE` from 8U to 64U
- [ ] `FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE` from 8U to 64U
- [ ] `HS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE` from 8U to 64U  
- [ ] `HS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE` from 8U to 64U
- [ ] `USB_HID_GENERIC_IN_BUFFER_LENGTH` from 8U to 64U
- [ ] `USB_HID_GENERIC_OUT_BUFFER_LENGTH` from 8U to 64U

## References
- FIDO Alliance CTAP HID Transport Protocol
- USB HID Specification v1.11
- NXP USB Stack Documentation
- USB Device Framework (USB 2.0 Specification Chapter 9)
