# YubiKey 5 Series - Phân Tích Chức Năng và Thiết Kế

## Tổng Quan YubiKey 5 Series

YubiKey 5 series là một hardware security key hỗ trợ nhiều protocol authentication khác nhau. Đây là phân tích chi tiết:

## 1. Các Chức Năng Chính

### 1.1. FIDO2/WebAuthn (Ưu tiên cho prototype)
- **Mục đích**: Passwordless authentication cho web applications
  - *Passwordless authentication*: Đăng nhập mà không cần nhập password, chỉ cần nhấn nút trên thiết bị
  - *Web applications*: Các website như Google, Facebook, GitHub, etc.

- **Cơ chế**: Public-key cryptography, challenge-response
  - *Public-key cryptography*: Hệ thống mã hóa dùng 2 keys: public key (công khai) và private key (bí mật)
  - *Challenge-response*: Website gửi một "thử thách" (random data), thiết bị "trả lời" bằng chữ ký số

- **Use cases** (Trường hợp sử dụng): 
  - Đăng nhập websites (Google, Facebook, GitHub, etc.)
  - Passwordless login (đăng nhập không cần password)
  - 2FA (Two-Factor Authentication - xác thực 2 lớp) cho web services

- **Technical** (Chi tiết kỹ thuật): 
  - *CTAP2 protocol*: Giao thức giao tiếp giữa browser và thiết bị USB key
  - *USB HID*: Chuẩn giao tiếp USB cho thiết bị input (keyboard, mouse, etc.)
  - *Resident keys*: Lưu trữ thông tin đăng nhập trực tiếp trên thiết bị USB key
  - *User verification*: Xác thực người dùng qua PIN hoặc vân tay

### 1.2. FIDO U2F (Legacy FIDO)
- **Mục đích**: Two-factor authentication (xác thực 2 lớp)
  - *Two-factor authentication*: Bảo mật 2 tầng - cần password + USB key để đăng nhập
  - *Legacy*: Phiên bản cũ của FIDO, vẫn được hỗ trợ để tương thích ngược

- **Cơ chế**: Second factor cho existing password-based login
  - *Second factor*: Yếu tố thứ 2 (sau password) để xác thực
  - *Password-based login*: Đăng nhập truyền thống bằng username/password

- **Use cases**: 2FA cho websites hỗ trợ U2F
- **Technical**: CTAP1 protocol (phiên bản đầu tiên của CTAP)

### 1.3. PIV (Personal Identity Verification)
- **Mục đích**: Enterprise authentication, certificate-based auth
  - *Enterprise authentication*: Xác thực trong môi trường doanh nghiệp/công ty
  - *Certificate-based auth*: Xác thực dựa trên chứng chỉ số (digital certificate)
  - *Digital certificate*: Như "chứng minh thư điện tử" chứa thông tin và chữ ký số

- **Use cases** (Trường hợp sử dụng):
  - Windows login (đăng nhập Windows bằng smart card)
  - Email signing/encryption (ký và mã hóa email) - S/MIME
  - VPN authentication (xác thực kết nối VPN)
  - Code signing (ký chữ ký số cho code/phần mềm)

- **Technical**: ISO 7816-4 smart card standard
  - *ISO 7816-4*: Tiêu chuẩn quốc tế cho smart card
  - *Smart card*: Thẻ thông minh có chip xử lý, như thẻ ATM

### 1.4. OpenPGP
- **Mục đích**: Email encryption, code signing
  - *Email encryption*: Mã hóa email để chỉ người nhận mới đọc được
  - *Code signing*: Ký chữ ký số vào code để chứng minh tác giả và tính toàn vẹn

- **Use cases** (Trường hợp sử dụng):
  - GPG key storage (lưu trữ khóa GPG trên hardware)
  - Git commit signing (ký chữ ký số cho Git commits)
  - Email encryption/signing (mã hóa và ký email)

- **Technical**: OpenPGP Card specification
  - *GPG (GNU Privacy Guard)*: Phần mềm mã hóa mã nguồn mở
  - *Git commit*: Mỗi lần save code lên Git
  - *OpenPGP*: Tiêu chuẩn mở cho mã hóa và chữ ký số

### 1.5. OTP (One-Time Password)
- **Mục đích**: TOTP/HOTP generation
  - *One-Time Password*: Mật khẩu dùng một lần, thay đổi theo thời gian
  - *TOTP (Time-based OTP)*: OTP dựa trên thời gian (như Google Authenticator)
  - *HOTP (HMAC-based OTP)*: OTP dựa trên số đếm

- **Use cases** (Trường hợp sử dụng):
  - Google Authenticator replacement (thay thế app Google Authenticator)
  - Static passwords (mật khẩu tĩnh - không đổi)
  - Challenge-response OTP (OTP theo kiểu hỏi-đáp)

- **Technical** (Chi tiết kỹ thuật): 
  - Yubico OTP (proprietary) - giao thức riêng của Yubico
  - OATH-HOTP/TOTP (RFC standards) - tiêu chuẩn quốc tế
  - *Proprietary*: Độc quyền, chỉ Yubico sử dụng
  - *RFC standards*: Tiêu chuẩn Internet được công nhận rộng rãi

### 1.6. OATH
- **Mục đích**: Standards-based OTP
  - *Standards-based*: Dựa trên tiêu chuẩn quốc tế, tương thích với nhiều hệ thống

- **Use cases**: Compatible với TOTP apps như Google Authenticator
  - *Compatible*: Tương thích - có thể làm việc cùng nhau

- **Technical**: RFC 6238 (TOTP), RFC 4226 (HOTP)
  - *RFC 6238/4226*: Các tài liệu tiêu chuẩn kỹ thuật cho TOTP và HOTP

## 2. Kiến Trúc Hardware/Software Cần Thiết

### 2.1. Hardware Requirements
```
┌─────────────────────────────────────────┐
│              MCU Core                   │
│  ┌─────────────┐  ┌─────────────────┐   │
│  │   CPU       │  │   Crypto Engine │   │
│  │             │  │   - AES         │   │
│  │             │  │   - ECC P-256   │   │
│  │             │  │   - SHA-256     │   │
│  │             │  │   - RNG         │   │
│  └─────────────┘  └─────────────────┘   │
│                                         │
│  ┌─────────────┐  ┌─────────────────┐   │
│  │Flash Memory │  │   Secure Storage│   │
│  │- Firmware   │  │   - Keys        │   │
│  │- Apps       │  │   - Certificates│   │
│  └─────────────┘  └─────────────────┘   │
└─────────────────────────────────────────┘
          │
          │ USB Interface
          ▼
   ┌─────────────┐
   │ Host Device │
   └─────────────┘
```


### 2.2. Software Architecture (Kiến trúc phần mềm)

```
┌─────────────────────────────────────────────┐
│           Application Layer (APP)           │
│ ┌─────────┐ ┌─────────┐ ┌─────────────┐     │
│ │ FIDO2   │ │   PIV   │ │    OTP      │     │
│ │WebAuthn │ │         │ │             │     │
│ └─────────┘ └─────────┘ └─────────────┘     │
├─────────────────────────────────────────────┤
│             Platform Layer                  │
│ ┌─────────────┬─────────────┬────────────┐ │
│ │  Crypto     │ Communication│  Storage   │ │
│ │  Engine     │             │            │ │
│ └─────────────┴─────────────┴────────────┘ │
│         ┌─────────────────────────────┐     │
│         │   Communication Component   │     │
│         │ ┌─────────────┬───────────┐ │     │
│         │ │ Protocol    │ Transport │ │     │
│         │ │ Layer       │ Layer     │ │     │
│         │ │ (CTAP2,     │ (FIDO HID,│ │     │
│         │ │ ISO7816,    │  CCID,    │ │     │
│         │ │ Yubico OTP) │  BLE, NFC)│ │     │
│         │ └─────────────┴───────────┘ │     │
│         └─────────────────────────────┘     │
├─────────────────────────────────────────────┤
│         Hardware Abstraction Layer (HAL)    │
│ ┌─────────────┬─────────────┬────────────┐ │
│ │ USB HID HAL │ Crypto HAL  │ Storage HAL│ │
│ └─────────────┴─────────────┴────────────┘ │
├─────────────────────────────────────────────┤
│                Hardware                    │
│        MCU + Crypto + Storage              │
└─────────────────────────────────────────────┘
```


#### Mô tả chi tiết từng tầng kiến trúc mới:

**🔵 Application Layer (APP Layer)**
- Chứa logic nghiệp vụ cho các chức năng xác thực: FIDO2/WebAuthn, OTP, PIV, v.v.
- Không phụ thuộc vào phần cứng hay giao thức truyền thông.
- Ví dụ: Khi người dùng yêu cầu tạo credential mới, tầng này sẽ xử lý và gọi xuống Platform Layer.

**🟡 Platform Layer**
- Chứa các thành phần nền tảng: Crypto Engine, Communication, Storage.
- **Crypto**: Thực hiện các thuật toán mã hóa, tạo chữ ký số, sinh số ngẫu nhiên.
- **Storage**: Quản lý lưu trữ keys, credentials, settings.
- **Communication**: Chia thành hai lớp nhỏ:
  - **Protocol Layer**: Xử lý các giao thức xác thực (CTAP2 cho FIDO2, ISO7816 cho PIV, Yubico OTP cho OTP).
  - **Transport Layer**: Xử lý phương thức truyền tải vật lý (FIDO HID, CCID, BLE, NFC).
- Ví dụ: Khi APP Layer yêu cầu tạo credential, Platform Layer sẽ dùng Crypto để tạo key, Storage để lưu, Communication để truyền dữ liệu ra ngoài.

**🟠 Hardware Abstraction Layer (HAL)**
- Cung cấp API thống nhất cho phần cứng: USB HID HAL, Crypto HAL, Storage HAL.
- Giúp code không phụ thuộc loại MCU/chip, dễ mở rộng sang phần cứng mới.
- Ví dụ: Nếu đổi từ STM32 sang ESP32, chỉ cần thay đổi HAL mà không ảnh hưởng logic tầng trên.

**🔴 Hardware Layer**
- Phần cứng thực tế của thiết bị: MCU, Crypto engine, Flash/EEPROM.


#### Luồng dữ liệu ví dụ (FIDO2 Login):
```
1. Website → Browser: "Đăng nhập bằng USB key"
2. Browser → USB HID: Gửi CTAP2 MakeCredential command
3. USB HID → CTAP2 Protocol: Parse command bytes thành structured data
4. CTAP2 → FIDO2 App: "Tạo credential cho domain abc.com"
5. FIDO2 App → Crypto HAL: "Tạo key pair mới"
6. Crypto HAL → Hardware: Thực hiện tạo key trên chip
7. Hardware → Crypto HAL: Key pair đã tạo
8. Crypto HAL → FIDO2 App: Trả về public/private key
9. FIDO2 App → Storage HAL: "Lưu private key"
10. Storage HAL → Hardware: Ghi vào Flash memory
11. FIDO2 App → CTAP2: Trả về credential response
12. CTAP2 → USB HID: Encode response thành bytes
13. USB HID → Browser: Gửi kết quả
14. Browser → Website: "Credential đã tạo thành công"
```
```
┌─────────────────────────────────────────┐
│           Application Layer             │
│ ┌─────────┐ ┌─────────┐ ┌─────────────┐ │
│ │ FIDO2   │ │   PIV   │ │    OTP      │ │
│ │WebAuthn │ │         │ │             │ │
│ └─────────┘ └─────────┘ └─────────────┘ │
├─────────────────────────────────────────┤
│          Protocol Layer                 │
│ ┌─────────┐ ┌─────────┐ ┌─────────────┐ │
│ │  CTAP2  │ │ISO 7816 │ │   Yubico    │ │
│ │         │ │         │ │    OTP      │ │
│ └─────────┘ └─────────┘ └─────────────┘ │
├─────────────────────────────────────────┤
│         Transport Layer                 │
│ ┌─────────────────────────────────────┐ │
│ │            USB HID                  │ │
│ │     ┌─────────┐ ┌─────────────┐     │ │
│ │     │FIDO HID │ │  CCID       │     │ │
│ │     └─────────┘ └─────────────┘     │ │
│ └─────────────────────────────────────┘ │
├─────────────────────────────────────────┤
│         Hardware Abstraction           │
│ ┌─────────┐ ┌─────────┐ ┌─────────────┐ │
│ │  USB    │ │ Crypto  │ │   Storage   │ │
│ │  HAL    │ │  HAL    │ │    HAL      │ │
│ └─────────┘ └─────────┘ └─────────────┘ │
├─────────────────────────────────────────┤
│              Hardware                   │
│        MCU + Crypto + Storage           │
└─────────────────────────────────────────┘
```

## 3. Roadmap Phát Triển (Prototype → Production)

### Phase 1: Prototype (FIDO2/WebAuthn Only) - "Sản phẩm thử nghiệm"
**🎯 Mục tiêu**: Tạo MVP (Minimum Viable Product - sản phẩm khả thi tối thiểu) với basic FIDO2 functionality

**📋 Những gì cần làm:**
- ✅ USB HID transport: Làm cho USB key có thể "nói chuyện" với máy tính
  - *USB HID*: Giả lập như một thiết bị input (keyboard, mouse)
  
- ✅ CTAP2 basic commands: Hỗ trợ các lệnh cơ bản của giao thức FIDO2
  - *CTAP2*: Giao thức để browser và USB key giao tiếp
  - *Basic commands*: GetInfo, MakeCredential, GetAssertion (3 lệnh quan trọng nhất)
  
- ✅ Attestation (basic): Chức năng "chứng thực" thiết bị là thật
  - *Attestation*: Như "giấy chứng nhận" thiết bị không phải fake
  
- ✅ User presence detection: Phát hiện người dùng có mặt (nhấn nút)
  - *User presence*: Đảm bảo có người thật đang sử dụng (không phải bot)
  
- ✅ Basic key generation/storage: Tạo và lưu key cơ bản
  - *Key generation*: Tạo cặp khóa public/private cho mỗi website
  - *Storage*: Lưu trữ an toàn trong Flash memory

**🏆 Kết quả mong đợi**: Có thể đăng nhập 1 website đơn giản bằng USB key

### Phase 2: Enhanced FIDO2 - "FIDO2 hoàn thiện"
**🎯 Mục tiêu**: Full FIDO2 compliance (tuân thủ đầy đủ tiêu chuẩn FIDO2)

**📋 Những gì cần làm:**
- User verification (PIN): Hỗ trợ PIN để bảo mật cao hơn
  - *PIN*: Mã số như PIN thẻ ATM, phải nhập đúng mới sử dụng được
  
- Resident keys: Lưu trữ thông tin đăng nhập trên chính USB key
  - *Resident keys*: USB key "nhớ" được thông tin các tài khoản
  - Lợi ích: Không cần nhập username, chỉ cần cắm USB key
  
- Multiple credentials: Hỗ trợ nhiều tài khoản trên cùng website
  - Ví dụ: Gmail cá nhân và Gmail công ty trên cùng 1 USB key
  
- Credential management: Quản lý (thêm/xóa/sửa) các credential
  - *Credential*: Thông tin xác thực cho 1 tài khoản cụ thể
  
- Enhanced attestation: Chứng thực nâng cao, an toàn hơn
  - Chống fake, chống clone USB key

**🏆 Kết quả mong đợi**: USB key hoạt động như YubiKey thật về FIDO2

### Phase 3: Multi-Protocol Support - "Hỗ trợ đa giao thức"
**🎯 Mục tiêu**: Thêm các protocols khác ngoài FIDO2

**📋 Những gì cần làm:**
- FIDO U2F (backward compatibility): Hỗ trợ websites cũ dùng U2F
  - *Backward compatibility*: Tương thích ngược với hệ thống cũ
  - *U2F*: Phiên bản trước của FIDO2, vẫn được nhiều website dùng
  
- Basic OTP: Tạo mã OTP như Google Authenticator
  - *OTP*: One-Time Password - mã dùng 1 lần, đổi liên tục
  - Như 6 số hiện trên app Google Authenticator
  
- USB CCID interface: Giao diện để hoạt động như smart card
  - *CCID*: Chip Card Interface Device
  - *Smart card*: Thẻ thông minh (như thẻ ATM có chip)

**🏆 Kết quả mong đợi**: USB key có thể thay thế được Google Authenticator

### Phase 4: Enterprise Features - "Tính năng doanh nghiệp"
**🎯 Mục tiêu**: Enterprise-grade functionality (tính năng cấp doanh nghiệp)

**📋 Những gì cần làm:**
- PIV support: Hỗ trợ đăng nhập Windows, VPN công ty
  - *PIV*: Personal Identity Verification - chuẩn xác thực doanh nghiệp
  - Có thể đăng nhập Windows bằng USB key thay vì password
  
- OpenPGP support: Ký và mã hóa email, ký code
  - *OpenPGP*: Chuẩn mã hóa email và files
  - *Code signing*: Ký chữ ký số vào phần mềm để chứng minh tác giả
  
- Advanced key management: Quản lý key phức tạp, bảo mật cao
  - Phân quyền, backup, recovery key
  
- Certificate management: Quản lý chứng chỉ số
  - *Certificate*: Giấy chứng nhận điện tử, như chứng minh thư số

**🏆 Kết quả mong đợi**: Có thể dùng trong môi trường công ty lớn

### Phase 5: Production Hardening - "Cứng cáp hóa sản phẩm"
**🎯 Mục tiêu**: Security và reliability (bảo mật và độ tin cậy) cấp sản xuất

**📋 Những gì cần làm:**
- Secure boot: Khởi động an toàn, chống firmware fake
  - *Secure boot*: Chỉ chạy firmware đã được ký xác thực
  - *Firmware*: Phần mềm điều khiển phần cứng (như BIOS)
  
- Tamper resistance: Chống can thiệp vật lý
  - *Tamper resistance*: Chống phá hoại, nếu bị mở máy thì tự xóa dữ liệu
  
- Certification compliance: Tuân thủ các chứng chỉ quốc tế
  - *FIDO2 certification*: Chứng nhận chính thức từ FIDO Alliance
  - *Common Criteria*: Tiêu chuẩn bảo mật quốc tế
  
- Manufacturing support: Hỗ trợ sản xuất hàng loạt
  - Quy trình sản xuất, kiểm tra chất lượng, provisioning
  - *Provisioning*: Cài đặt thông tin nhận dạng cho từng thiết bị

**🏆 Kết quả mong đợi**: Sản phẩm sẵn sàng bán trên thị trường

### 📊 Timeline tổng quan:
- **Phase 1**: 2-3 tháng (prototype cơ bản)
- **Phase 2**: 1-2 tháng (FIDO2 đầy đủ)  
- **Phase 3**: 2-3 tháng (thêm protocols)
- **Phase 4**: 3-4 tháng (enterprise features)
- **Phase 5**: 2-3 tháng (production ready)

**Tổng cộng**: 10-15 tháng từ prototype đến sản phẩm hoàn chỉnh

## 4. Key Design Principles

### 4.1. Modularity
- Mỗi protocol là một module riêng biệt
- Clear interfaces giữa các layers
- Pluggable transport mechanisms

### 4.2. Security-First
- Hardware-backed key storage
- Secure random number generation
- Side-channel attack resistance
- PIN/credential protection

### 4.3. Standards Compliance
- FIDO2 CTAP2 specification
- WebAuthn standard
- USB HID specification
- Cryptographic standards (NIST, etc.)

### 4.4. Upgradeability
- Firmware update mechanism
- Backward compatibility
- Feature flags for new functionality

## 5. Technical Specifications Cần Thiết

### 5.1. Cryptographic Requirements (Yêu cầu mã hóa)

**🔐 Asymmetric Encryption (Mã hóa bất đối xứng):**
- **ECC P-256**: Elliptic Curve Cryptography với curve P-256
  - *Elliptic Curve*: Thuật toán mã hóa dựa trên đường cong ellipse
  - *P-256*: Một loại curve cụ thể, bảo mật tương đương RSA 3072-bit
  - *Lợi ích*: Nhanh hơn RSA, ít tốn memory hơn
  
- **Ed25519**: Thuật toán chữ ký số hiện đại
  - *Ed25519*: Variant của ECC, tối ưu cho signing
  - *Lợi ích*: Rất nhanh, rất an toàn, ít bug

**🔒 Symmetric Encryption (Mã hóa đối xứng):**
- **AES-256**: Advanced Encryption Standard với key 256-bit
  - *AES*: Thuật toán mã hóa được chính phủ Mỹ chứng nhận
  - *256-bit*: Độ dài key, càng dài càng an toàn
  - *Sử dụng*: Mã hóa dữ liệu lưu trữ, bảo vệ private keys

**🏷️ Hash Functions (Hàm băm):**
- **SHA-256, SHA-512**: Secure Hash Algorithm
  - *Hash function*: Chuyển dữ liệu bất kỳ thành chuỗi số có độ dài cố định
  - *SHA-256*: Tạo ra hash 256-bit (32 bytes)
  - *Sử dụng*: Tạo fingerprint cho dữ liệu, verify tính toàn vẹn

**🎲 Random Number Generation (Tạo số ngẫu nhiên):**
- **True RNG**: True Random Number Generator
  - *True RNG*: Dùng noise vật lý (nhiệt, điện) để tạo số thật sự ngẫu nhiên
  - *Khác với Pseudo-RNG*: Không dự đoán được, an toàn tuyệt đối
  - *Quan trọng*: Keys yếu = bảo mật yếu

**🗄️ Key Storage (Lưu trữ khóa):**
- **Secure storage**: Lưu trữ an toàn, chống trích xuất
- **Tamper-resistant**: Chống can thiệp vật lý
  - *Tamper-resistant*: Nếu ai đó cố mở thiết bị để lấy key thì key tự xóa

### 5.2. Performance Requirements (Yêu cầu hiệu năng)

**⚡ Response time < 200ms cho authentication**
- *Authentication*: Quá trình xác thực (đăng nhập)
- *200ms*: Người dùng không cảm thấy chậm, trải nghiệm mượt mà
- *Bao gồm*: Nhận lệnh → xử lý → tạo chữ ký → trả về kết quả

**💾 Support 25+ resident keys**
- *Resident keys*: Keys được lưu trữ trên thiết bị (không cần server)
- *25+ keys*: Đủ cho người dùng bình thường (Gmail, Facebook, GitHub, etc.)
- *Memory requirement*: Mỗi key ~100-200 bytes

**🔄 100K+ authentication cycles**
- *Authentication cycle*: 1 lần đăng nhập hoàn chỉnh
- *100K cycles*: Thiết bị hoạt động ít nhất 5-10 năm với sử dụng hàng ngày
- *Wear leveling*: Phân tán ghi/xóa để Flash memory không bị hỏng sớm

**🔋 Low power consumption**
- *Low power*: Tiêu thụ điện thấp
- *Lý do*: USB cung cấp điện có hạn, không có pin
- *Target*: < 100mA trong hoạt động, < 10mA khi idle

### 5.3. Compliance Requirements (Yêu cầu tuân thủ)

**🏅 FIDO2 Level 1/2 certification**
- *FIDO2 Level 1*: Chứng nhận cơ bản từ FIDO Alliance
- *FIDO2 Level 2*: Chứng nhận cao cấp (yêu cầu bảo mật vật lý)
- *Lợi ích*: Được browser tin tưởng, hoạt động với mọi website FIDO2

**🔌 USB-IF compliance**
- *USB-IF*: USB Implementers Forum - tổ chức quản lý chuẩn USB
- *Compliance*: Tuân thủ các quy định USB (electrical, protocol, compatibility)
- *Lợi ích*: Hoạt động với mọi máy tính, không gây conflict

**♻️ RoHS compliance**
- *RoHS*: Restriction of Hazardous Substances - hạn chế chất độc hại
- *Yêu cầu EU*: Không dùng chì, thủy ngân, cadmium trong sản xuất
- *Cần thiết*: Để bán tại EU và nhiều thị trường khác

**📋 CE/FCC certification**
- *CE*: Conformité Européenne - chứng nhận EU
- *FCC*: Federal Communications Commission - chứng nhận Mỹ  
- *Nội dung*: EMC (electromagnetic compatibility), RF emissions
- *Cần thiết*: Để bán hợp pháp tại EU và Mỹ

### 📋 Tóm tắt requirements theo độ ưu tiên:

**🔴 Critical (Phase 1 - Prototype):**
- ECC P-256 + SHA-256
- Basic secure storage
- USB HID compliance
- Response time < 200ms

**🟡 Important (Phase 2-3):**
- AES-256 encryption
- True RNG
- 25+ resident keys support
- FIDO2 Level 1 certification

**🟢 Nice-to-have (Phase 4-5):**
- Ed25519 support
- Hardware tamper resistance
- FIDO2 Level 2 certification
- Full regulatory compliance
