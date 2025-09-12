# USB HID Tester

A comprehensive GUI-based testing tool for USB HID devices, specifically designed for testing USB authentication keys and FIDO2 devices.

## Features

- **Device Discovery**: Automatic detection of USB HID devices across platforms
- **Manual Communication**: Send and receive raw HID packets with hex input/output
- **Automated Testing**: Comprehensive test suites for connectivity, echo, integrity, performance, and FIDO2
- **Real-time Monitoring**: Live packet monitoring with timestamped logs
- **Data Logging**: Persistent logging with multiple export formats (text, JSON, CSV)
- **Performance Tracking**: Detailed performance metrics and statistics
- **Configuration Management**: Customizable settings for testing parameters

## Architecture

The tool is built with a modular architecture:

```
usb_hid_tester/
├── main.py                 # Main GUI application
├── usb_device_manager.py   # USB device discovery and management
├── hid_tester.py          # Core testing functionality
├── test_suites.py         # Predefined test scenarios
├── data_logger.py         # Logging and data persistence
├── requirements.txt       # Python dependencies
└── README.md             # This file
```

## Installation

### Prerequisites

- Python 3.7 or higher
- Administrator/root privileges (required for USB device access)

### Step 1: Install Python Dependencies

```bash
cd tools/usb_hid_tester
pip install -r requirements.txt
```

### Step 2: Install System Dependencies

#### Linux (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install libusb-1.0-0-dev libudev-dev
sudo apt-get install python3-tk  # For GUI support
```

#### Windows:
- Install latest Visual C++ Redistributable
- libusb drivers are included with the Python packages

#### macOS:
```bash
brew install libusb
```

### Step 3: USB Permissions (Linux only)

Create a udev rule for USB device access without root:

```bash
sudo nano /etc/udev/rules.d/99-usb-hid.rules
```

Add the following content:
```
# Allow access to USB HID devices
SUBSYSTEM=="usb", MODE="0666"
SUBSYSTEM=="hidraw", MODE="0666"
```

Reload udev rules:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Usage

### Starting the Application

```bash
cd tools/usb_hid_tester
python main.py
```

### Basic Workflow

1. **Device Discovery**: 
   - The application automatically scans for USB HID devices
   - Click "Refresh Devices" to rescan
   - Select your target device from the list

2. **Connection**:
   - Click "Connect" to establish communication with the selected device
   - Connection status is displayed in the status bar

3. **Manual Testing**:
   - Go to the "Communication" tab
   - Enter hex data to send (e.g., "01 02 03 04")
   - Click "Send" to transmit data
   - Received data is displayed in the receive area

4. **Automated Testing**:
   - Go to the "Testing" tab
   - Select predefined test suites or create custom tests
   - Click "Run Test" to execute
   - Results are displayed in real-time

5. **Monitoring**:
   - Go to the "Monitor" tab
   - View live packet data and statistics
   - All communication is logged with timestamps

6. **Data Export**:
   - Go to the "Results" tab
   - Export logs in various formats
   - View performance statistics and test summaries

### Test Suites

#### Basic Connectivity Test
- Verifies device enumeration and basic communication
- Tests open/close operations
- Validates device descriptors

#### Echo Test
- Sends data packets and verifies echo responses
- Tests data integrity and round-trip timing
- Configurable packet sizes and counts

#### Stress Test
- High-frequency packet transmission
- Memory and buffer overflow testing
- Long-duration stability testing

#### FIDO2 Authentication Test
- Tests FIDO2/WebAuthn protocol commands
- Verifies authenticator responses
- Tests various credential operations

#### Performance Test
- Measures throughput and latency
- Tests various packet sizes
- Generates performance reports

### Configuration

Configuration settings are stored in `config.json` and can be modified through the GUI or manually:

```json
{
  "device": {
    "default_packet_size": 8,
    "timeout_ms": 1000,
    "retry_count": 3
  },
  "testing": {
    "default_test_duration": 60,
    "echo_packet_count": 10,
    "stress_test_duration": 300
  },
  "logging": {
    "level": "INFO",
    "save_packet_data": true,
    "auto_save_logs": true
  }
}
```

## Troubleshooting

### Common Issues

1. **Device Not Found**:
   - Ensure device is properly connected
   - Check USB permissions (Linux)
   - Run as administrator (Windows)
   - Try different USB ports

2. **Permission Denied**:
   - Linux: Set up udev rules as described above
   - Windows: Run as administrator
   - macOS: Grant permission in System Preferences

3. **Import Errors**:
   - Ensure all dependencies are installed
   - Check Python version compatibility
   - Try reinstalling packages with `pip install --upgrade`

4. **GUI Issues**:
   - Ensure tkinter is installed
   - Check display/X11 forwarding for remote systems
   - Try different display settings

### Debug Mode

Enable debug logging by setting the log level to DEBUG in the configuration:

```python
# In the GUI, go to Settings and change log level to DEBUG
# Or modify config.json directly
{
  "logging": {
    "level": "DEBUG"
  }
}
```

## Development

### Adding New Tests

1. Create a new test function in `test_suites.py`:

```python
def custom_test(device_manager, logger, progress_callback=None):
    """Your custom test implementation"""
    logger.info("Starting custom test")
    
    # Test implementation here
    
    return {"result": "PASSED", "details": "Test completed successfully"}
```

2. Add the test to the test registry:

```python
TEST_REGISTRY["Custom Test"] = {
    "function": custom_test,
    "description": "Description of your custom test",
    "duration_estimate": 30  # seconds
}
```

### Extending Device Support

Modify `usb_device_manager.py` to add support for new device types or protocols.

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This project is part of the USB Key Authentication system and follows the project's licensing terms.

## Support

For issues and questions:
1. Check the troubleshooting section above
2. Review the debug logs
3. Create an issue in the project repository
