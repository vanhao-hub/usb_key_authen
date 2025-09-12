"""
USB Device Manager Module
Handles USB HID device discovery and connection management

Author: USB Key Authentication Team
Date: 2025-09-12
"""

import platform
import subprocess
import re
from typing import List, Dict, Optional

try:
    import hid
    HID_AVAILABLE = True
except ImportError:
    HID_AVAILABLE = False
    print("Warning: hidapi not available. Install with: pip install hidapi")

try:
    import usb.core
    import usb.util
    PYUSB_AVAILABLE = True
except ImportError:
    PYUSB_AVAILABLE = False
    print("Warning: pyusb not available. Install with: pip install pyusb")

class USBDeviceManager:
    """Manages USB HID device discovery and connection"""
    
    def __init__(self):
        self.connected_device = None
        self.device_list = []
        
    def discover_devices(self) -> List[Dict]:
        """Discover all available USB HID devices"""
        devices = []
        
        # Try hidapi first (more reliable for HID devices)
        if HID_AVAILABLE:
            devices.extend(self._discover_hid_devices())
        
        # Fallback to pyusb for more device info
        if PYUSB_AVAILABLE:
            devices.extend(self._discover_usb_devices())
        
        # If no libraries available, try system commands
        if not devices:
            devices.extend(self._discover_system_devices())
        
        # Remove duplicates and add common test devices
        devices = self._deduplicate_devices(devices)
        devices.extend(self._get_common_test_devices())
        
        self.device_list = devices
        return devices
    
    def _discover_hid_devices(self) -> List[Dict]:
        """Discover HID devices using hidapi"""
        devices = []
        
        try:
            for device_info in hid.enumerate():
                device = {
                    'vid': device_info.get('vendor_id', 0),
                    'pid': device_info.get('product_id', 0),
                    'manufacturer': device_info.get('manufacturer_string', 'Unknown'),
                    'product': device_info.get('product_string', 'Unknown'),
                    'serial': device_info.get('serial_number', ''),
                    'path': device_info.get('path', b'').decode('utf-8', errors='ignore'),
                    'interface': device_info.get('interface_number', -1),
                    'available': True,
                    'type': 'HID',
                    'source': 'hidapi'
                }
                devices.append(device)
                
        except Exception as e:
            print(f"Error discovering HID devices: {e}")
        
        return devices
    
    def _discover_usb_devices(self) -> List[Dict]:
        """Discover USB devices using pyusb"""
        devices = []
        
        try:
            usb_devices = usb.core.find(find_all=True)
            
            for device in usb_devices:
                # Check if device has HID interface
                if self._is_hid_device(device):
                    device_info = {
                        'vid': device.idVendor,
                        'pid': device.idProduct,
                        'manufacturer': usb.util.get_string(device, device.iManufacturer) if device.iManufacturer else 'Unknown',
                        'product': usb.util.get_string(device, device.iProduct) if device.iProduct else 'Unknown',
                        'serial': usb.util.get_string(device, device.iSerialNumber) if device.iSerialNumber else '',
                        'bus': device.bus,
                        'address': device.address,
                        'available': True,
                        'type': 'USB-HID',
                        'source': 'pyusb'
                    }
                    devices.append(device_info)
                    
        except Exception as e:
            print(f"Error discovering USB devices: {e}")
        
        return devices
    
    def _discover_system_devices(self) -> List[Dict]:
        """Discover devices using system commands"""
        devices = []
        system = platform.system()
        
        try:
            if system == "Linux":
                devices.extend(self._discover_linux_devices())
            elif system == "Windows":
                devices.extend(self._discover_windows_devices())
            elif system == "Darwin":  # macOS
                devices.extend(self._discover_macos_devices())
                
        except Exception as e:
            print(f"Error discovering system devices: {e}")
        
        return devices
    
    def _discover_linux_devices(self) -> List[Dict]:
        """Discover devices on Linux using lsusb"""
        devices = []
        
        try:
            # Use lsusb command
            result = subprocess.run(['lsusb'], capture_output=True, text=True)
            if result.returncode == 0:
                for line in result.stdout.split('\n'):
                    if line.strip():
                        # Parse lsusb output: Bus 001 Device 002: ID 1d6b:0002 Linux Foundation 2.0 root hub
                        match = re.search(r'ID ([0-9a-fA-F]{4}):([0-9a-fA-F]{4})\s+(.+)', line)
                        if match:
                            vid = int(match.group(1), 16)
                            pid = int(match.group(2), 16)
                            description = match.group(3)
                            
                            # Check if likely HID device (basic heuristic)
                            if 'HID' in description.upper() or 'MOUSE' in description.upper() or 'KEYBOARD' in description.upper():
                                device = {
                                    'vid': vid,
                                    'pid': pid,
                                    'manufacturer': 'Unknown',
                                    'product': description,
                                    'available': True,
                                    'type': 'USB-HID',
                                    'source': 'lsusb'
                                }
                                devices.append(device)
        except FileNotFoundError:
            print("lsusb command not found")
        except Exception as e:
            print(f"Error running lsusb: {e}")
        
        return devices
    
    def _discover_windows_devices(self) -> List[Dict]:
        """Discover devices on Windows using PowerShell"""
        devices = []
        
        try:
            # Use PowerShell to query USB devices
            ps_command = '''
            Get-WmiObject -Class Win32_USBDevice | Where-Object {$_.Description -like "*HID*"} | 
            Select-Object DeviceID, Description, Manufacturer | ConvertTo-Json
            '''
            
            result = subprocess.run(['powershell', '-Command', ps_command], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0 and result.stdout.strip():
                import json
                usb_data = json.loads(result.stdout)
                
                if isinstance(usb_data, dict):
                    usb_data = [usb_data]
                
                for item in usb_data:
                    device_id = item.get('DeviceID', '')
                    # Parse VID/PID from device ID
                    vid_match = re.search(r'VID_([0-9A-F]{4})', device_id)
                    pid_match = re.search(r'PID_([0-9A-F]{4})', device_id)
                    
                    if vid_match and pid_match:
                        device = {
                            'vid': int(vid_match.group(1), 16),
                            'pid': int(pid_match.group(1), 16),
                            'manufacturer': item.get('Manufacturer', 'Unknown'),
                            'product': item.get('Description', 'Unknown'),
                            'available': True,
                            'type': 'USB-HID',
                            'source': 'powershell'
                        }
                        devices.append(device)
        
        except Exception as e:
            print(f"Error querying Windows devices: {e}")
        
        return devices
    
    def _discover_macos_devices(self) -> List[Dict]:
        """Discover devices on macOS using system_profiler"""
        devices = []
        
        try:
            result = subprocess.run(['system_profiler', 'SPUSBDataType', '-json'], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                import json
                usb_data = json.loads(result.stdout)
                
                # Parse USB data (structure varies)
                devices.extend(self._parse_macos_usb_data(usb_data))
        
        except Exception as e:
            print(f"Error querying macOS devices: {e}")
        
        return devices
    
    def _parse_macos_usb_data(self, data) -> List[Dict]:
        """Parse macOS USB data structure"""
        devices = []
        # Implementation would parse the complex nested structure
        # For now, return empty list
        return devices
    
    def _is_hid_device(self, device) -> bool:
        """Check if USB device has HID interface"""
        try:
            for config in device:
                for interface in config:
                    if interface.bInterfaceClass == 3:  # HID class
                        return True
        except:
            pass
        return False
    
    def _deduplicate_devices(self, devices: List[Dict]) -> List[Dict]:
        """Remove duplicate devices based on VID/PID"""
        seen = set()
        unique_devices = []
        
        for device in devices:
            key = (device.get('vid', 0), device.get('pid', 0))
            if key not in seen:
                seen.add(key)
                unique_devices.append(device)
        
        return unique_devices
    
    def _get_common_test_devices(self) -> List[Dict]:
        """Get list of common test device entries"""
        common_devices = [
            {
                'vid': 0x1FC9,  # NXP
                'pid': 0x0083,  # Common NXP HID device
                'manufacturer': 'NXP Semiconductors',
                'product': 'HID Generic Example',
                'available': False,  # Placeholder device
                'type': 'Test Device',
                'source': 'common_devices'
            },
            {
                'vid': 0x0483,  # STMicroelectronics
                'pid': 0x5750,  # ST HID device
                'manufacturer': 'STMicroelectronics',
                'product': 'STM32 HID Device',
                'available': False,
                'type': 'Test Device',
                'source': 'common_devices'
            }
        ]
        return common_devices
    
    def connect_device(self, device_info: Dict) -> bool:
        """Connect to a specific device"""
        try:
            if not HID_AVAILABLE:
                print("HID library not available")
                return False
            
            vid = device_info.get('vid', 0)
            pid = device_info.get('pid', 0)
            
            # Try to open HID device
            device = hid.device()
            device.open(vid, pid)
            
            # Set non-blocking mode
            device.set_nonblocking(True)
            
            self.connected_device = {
                'device': device,
                'info': device_info
            }
            
            return True
            
        except Exception as e:
            print(f"Failed to connect to device: {e}")
            return False
    
    def disconnect_device(self) -> bool:
        """Disconnect from current device"""
        try:
            if self.connected_device and 'device' in self.connected_device:
                self.connected_device['device'].close()
                self.connected_device = None
                return True
        except Exception as e:
            print(f"Error disconnecting device: {e}")
        
        return False
    
    def send_data(self, data: bytes) -> bool:
        """Send data to connected device"""
        try:
            if not self.connected_device:
                return False
            
            device = self.connected_device['device']
            device.write(data)
            return True
            
        except Exception as e:
            print(f"Error sending data: {e}")
            return False
    
    def receive_data(self, timeout_ms: int = 1000) -> Optional[bytes]:
        """Receive data from connected device"""
        try:
            if not self.connected_device:
                return None
            
            device = self.connected_device['device']
            data = device.read(64, timeout_ms)  # Read up to 64 bytes
            
            if data:
                return bytes(data)
            
        except Exception as e:
            print(f"Error receiving data: {e}")
        
        return None
    
    def get_device_info(self) -> Optional[Dict]:
        """Get information about connected device"""
        if self.connected_device:
            return self.connected_device['info']
        return None
    
    def is_connected(self) -> bool:
        """Check if device is connected"""
        return self.connected_device is not None
