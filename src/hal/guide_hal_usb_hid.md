# USB HID HAL Implementation Guide

## 📊 **Phân tích USB HID HAL Interface**

### 🎯 **USB HID HAL APIs cần implement (12 APIs):**

| **Nhóm** | **API** | **Mô tả** |
|----------|---------|-----------|
| **Cấu hình** | `configure()` | Cấu hình USB HID descriptor |
| | `set_callbacks()` | Đăng ký callback functions |
| **Truyền dữ liệu** | `send_report()` | Gửi HID report tới host |
| | `receive_report()` | Nhận HID report từ host |
| **Feature Reports** | `set_feature_report()` | Gửi feature report |
| | `get_feature_report()` | Nhận feature report |
| **Trạng thái** | `is_connected()` | Kiểm tra kết nối USB |
| | `get_status()` | Lấy trạng thái thiết bị |
| **Power Management** | `suspend()` | Suspend thiết bị |
| | `resume()` | Resume thiết bị |
| **Base HAL** | `init()` | Khởi tạo HAL |
| | `deinit()` | Hủy HAL |

### 🔗 **NXP USB APIs cần mapping:**

Từ `hid_generic.c`, các NXP APIs chính:

| **NXP API** | **Chức năng** | **Map tới HAL API** |
|-------------|---------------|---------------------|
| `USB_DeviceClassInit()` | Khởi tạo USB stack | → `init()` |
| `USB_DeviceRun()` | Bắt đầu USB device | → `init()` |
| `USB_DeviceHidSend()` | Gửi HID data | → `send_report()` |
| `USB_DeviceHidRecv()` | Nhận HID data | → `receive_report()` |
| `USB_DeviceHidGenericCallback()` | HID class callback | → Internal mapping |
| `USB_DeviceCallback()` | Device events | → `event_callback` |

### 🎪 **Events cần mapping:**

| **NXP Event** | **HAL Event** |
|---------------|---------------|
| `kUSB_DeviceHidEventSendResponse` | → `tx_complete_callback` |
| `kUSB_DeviceHidEventRecvResponse` | → `rx_callback` |
| `kUSB_DeviceEventBusReset` | → `USB_HID_EVENT_RESET` |
| `kUSB_DeviceEventDetach` | → `USB_HID_EVENT_DISCONNECT` |
| `kUSB_DeviceEventSetConfiguration` | → `USB_HID_EVENT_CONNECT` |

### 🏗️ **Cấu trúc NXP USB Stack:**

#### **Endpoints Configuration:**
```c
usb_device_endpoint_struct_t g_UsbDeviceHidGenericEndpoints[USB_HID_GENERIC_ENDPOINT_COUNT] = {
    /* HID generic interrupt IN pipe */
    {
        USB_HID_GENERIC_ENDPOINT_IN | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        FS_HID_GENERIC_INTERRUPT_IN_PACKET_SIZE,
        FS_HID_GENERIC_INTERRUPT_IN_INTERVAL,
    },
    /* HID generic interrupt OUT pipe */
    {
        USB_HID_GENERIC_ENDPOINT_OUT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        FS_HID_GENERIC_INTERRUPT_OUT_PACKET_SIZE,
        FS_HID_GENERIC_INTERRUPT_OUT_INTERVAL,
    }
};
```

#### **HID Report Descriptor:**
```c
uint8_t g_UsbDeviceHidGenericReportDescriptor[] = {
    0x05U, 0x81U, /* Usage Page (Vendor defined)*/
    0x09U, 0x82U, /* Usage (Vendor defined) */
    0xA1U, 0x01U, /* Collection (Application) */
    0x09U, 0x83U, /* Usage (Vendor defined) */
    // ... Input/Output reports định nghĩa 8 bytes mỗi chiều
    0xC0U,        /* End collection */
};
```

### 📋 **Kế hoạch implement:**

#### **Phase 1: Tạo USB HID HAL Implementation**
1. **Tạo file**: `src/hal/usb_hid_hal_nxp.c`
2. **Implement struct**: `usb_hid_hal_t` với tất cả function pointers
3. **Wrap NXP APIs** vào HAL APIs

#### **Phase 2: Callback Mapping**
1. **Wrap NXP callbacks** → HAL callbacks
2. **Event translation** từ NXP events → HAL events
3. **State management** giữa 2 layers

#### **Phase 3: Integration**
1. **Connect HAL** với FIDO Transport Layer
2. **Testing** end-to-end communication
3. **Error handling** và recovery

### 🔧 **Implementation Details:**

#### **HAL Context Structure:**
```c
typedef struct {
    usb_hid_hal_t base;                    // Base HAL interface
    usb_device_handle device_handle;       // NXP device handle
    class_handle_t hid_handle;            // NXP HID class handle
    usb_hid_rx_callback_t rx_callback;    // HAL RX callback
    usb_hid_tx_complete_callback_t tx_callback; // HAL TX callback
    usb_hid_event_callback_t event_callback;    // HAL event callback
    bool initialized;                      // Initialization state
    bool connected;                       // Connection state
    uint32_t status_flags;                // Current status
} usb_hid_hal_nxp_context_t;
```

#### **Key Mapping Functions:**
1. **`hal_init()`** → `USB_DeviceApplicationInit()`
2. **`hal_send_report()`** → `USB_DeviceHidSend()`
3. **`hal_receive_report()`** → setup receive buffer
4. **`nxp_callback_wrapper()`** → translate callbacks

### 🎯 **FIDO HID Requirements:**

#### **Packet Format:**
- **Packet Size**: 64 bytes
- **FIDO HID Protocol**: Channel ID + Command/Sequence + Payload
- **Fragmentation**: Large messages split across multiple packets

#### **Channel Management:**
- **Broadcast Channel**: 0xFFFFFFFF
- **Allocated Channels**: Dynamic allocation
- **Timeout Handling**: Message and channel timeouts

### 🚀 **Next Steps:**

1. **Analyze FIDO Transport Requirements** in detail
2. **Create HAL implementation** file
3. **Implement basic APIs** (init, send, receive)
4. **Add callback mapping** and event handling
5. **Test with FIDO Transport Layer**
6. **Integrate with NXP project** build system

### 📝 **Notes:**

- **Report Descriptor** cần được modify cho FIDO HID compliance
- **VID/PID** cần được đăng ký chính thức cho production
- **Endpoint configuration** có thể cần adjust cho FIDO requirements
- **Buffer management** cần optimize cho performance
