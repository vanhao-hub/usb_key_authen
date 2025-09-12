"""
Data Logger Module
Handles logging and data persistence

Author: USB Key Authentication Team
Date: 2025-09-12
"""

import json
import csv
import time
import os
from datetime import datetime
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict

@dataclass
class LogEntry:
    """Single log entry"""
    timestamp: float
    level: str  # DEBUG, INFO, WARN, ERROR
    message: str
    data: Optional[Dict] = None

class DataLogger:
    """Manages logging and data persistence"""
    
    def __init__(self, log_dir: str = "logs"):
        self.log_dir = log_dir
        self.log_entries = []
        self.session_id = f"session_{int(time.time())}"
        
        # Create logs directory if it doesn't exist
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
    
    def log(self, level: str, message: str, data: Optional[Dict] = None):
        """Add log entry"""
        entry = LogEntry(
            timestamp=time.time(),
            level=level.upper(),
            message=message,
            data=data
        )
        self.log_entries.append(entry)
        
        # Also print to console for debugging
        timestamp_str = datetime.fromtimestamp(entry.timestamp).strftime('%H:%M:%S.%f')[:-3]
        print(f"[{timestamp_str}] {entry.level}: {entry.message}")
        
        if data:
            print(f"    Data: {data}")
    
    def debug(self, message: str, data: Optional[Dict] = None):
        """Log debug message"""
        self.log("DEBUG", message, data)
    
    def info(self, message: str, data: Optional[Dict] = None):
        """Log info message"""
        self.log("INFO", message, data)
    
    def warn(self, message: str, data: Optional[Dict] = None):
        """Log warning message"""
        self.log("WARN", message, data)
    
    def error(self, message: str, data: Optional[Dict] = None):
        """Log error message"""
        self.log("ERROR", message, data)
    
    def log_packet(self, direction: str, data: bytes, endpoint: int = 0):
        """Log USB packet data"""
        packet_info = {
            'direction': direction,  # 'SEND' or 'RECV'
            'endpoint': endpoint,
            'data_hex': data.hex().upper(),
            'data_length': len(data),
            'data_bytes': list(data)
        }
        
        self.log("INFO", f"{direction} packet ({len(data)} bytes)", packet_info)
    
    def log_test_result(self, test_name: str, result: str, duration: float, details: str = ""):
        """Log test result"""
        test_data = {
            'test_name': test_name,
            'result': result,
            'duration': duration,
            'details': details
        }
        
        self.log("INFO", f"Test completed: {test_name} - {result}", test_data)
    
    def log_device_event(self, event: str, device_info: Optional[Dict] = None):
        """Log device-related events"""
        self.log("INFO", f"Device event: {event}", device_info)
    
    def save_session_log(self, filename: Optional[str] = None) -> str:
        """Save current session log to file"""
        if not filename:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"{self.session_id}_{timestamp}.log"
        
        filepath = os.path.join(self.log_dir, filename)
        
        try:
            with open(filepath, 'w') as f:
                f.write(f"USB HID Tester Session Log\n")
                f.write(f"Session ID: {self.session_id}\n")
                f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write("=" * 50 + "\n\n")
                
                for entry in self.log_entries:
                    timestamp_str = datetime.fromtimestamp(entry.timestamp).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                    f.write(f"[{timestamp_str}] {entry.level}: {entry.message}\n")
                    
                    if entry.data:
                        f.write(f"    Data: {json.dumps(entry.data, indent=2)}\n")
                    f.write("\n")
            
            return filepath
            
        except Exception as e:
            self.error(f"Failed to save session log: {e}")
            return ""
    
    def save_json_log(self, filename: Optional[str] = None) -> str:
        """Save log entries as JSON"""
        if not filename:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"{self.session_id}_{timestamp}.json"
        
        filepath = os.path.join(self.log_dir, filename)
        
        try:
            log_data = {
                'session_id': self.session_id,
                'timestamp': time.time(),
                'entries': [asdict(entry) for entry in self.log_entries]
            }
            
            with open(filepath, 'w') as f:
                json.dump(log_data, f, indent=2)
            
            return filepath
            
        except Exception as e:
            self.error(f"Failed to save JSON log: {e}")
            return ""
    
    def save_csv_log(self, filename: Optional[str] = None) -> str:
        """Save log entries as CSV"""
        if not filename:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"{self.session_id}_{timestamp}.csv"
        
        filepath = os.path.join(self.log_dir, filename)
        
        try:
            with open(filepath, 'w', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(['Timestamp', 'Level', 'Message', 'Data'])
                
                for entry in self.log_entries:
                    timestamp_str = datetime.fromtimestamp(entry.timestamp).strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
                    data_str = json.dumps(entry.data) if entry.data else ""
                    writer.writerow([timestamp_str, entry.level, entry.message, data_str])
            
            return filepath
            
        except Exception as e:
            self.error(f"Failed to save CSV log: {e}")
            return ""
    
    def clear_log(self):
        """Clear current log entries"""
        self.log_entries.clear()
        self.session_id = f"session_{int(time.time())}"
    
    def get_log_entries(self, level: Optional[str] = None) -> List[LogEntry]:
        """Get log entries, optionally filtered by level"""
        if level:
            return [entry for entry in self.log_entries if entry.level == level.upper()]
        return self.log_entries.copy()
    
    def get_packet_log(self) -> List[LogEntry]:
        """Get packet-related log entries"""
        return [entry for entry in self.log_entries 
                if entry.data and 'direction' in entry.data]
    
    def get_test_log(self) -> List[LogEntry]:
        """Get test-related log entries"""
        return [entry for entry in self.log_entries 
                if entry.data and 'test_name' in entry.data]
    
    def get_statistics(self) -> Dict:
        """Get logging statistics"""
        total_entries = len(self.log_entries)
        
        if total_entries == 0:
            return {}
        
        levels = {}
        for entry in self.log_entries:
            levels[entry.level] = levels.get(entry.level, 0) + 1
        
        # Packet statistics
        packet_entries = self.get_packet_log()
        packets_sent = sum(1 for entry in packet_entries 
                          if entry.data and entry.data.get('direction') == 'SEND')
        packets_received = sum(1 for entry in packet_entries 
                              if entry.data and entry.data.get('direction') == 'RECV')
        
        bytes_sent = sum(entry.data.get('data_length', 0) for entry in packet_entries 
                        if entry.data and entry.data.get('direction') == 'SEND')
        bytes_received = sum(entry.data.get('data_length', 0) for entry in packet_entries 
                            if entry.data and entry.data.get('direction') == 'RECV')
        
        # Test statistics
        test_entries = self.get_test_log()
        tests_passed = sum(1 for entry in test_entries 
                          if entry.data and entry.data.get('result') == 'PASSED')
        tests_failed = sum(1 for entry in test_entries 
                          if entry.data and entry.data.get('result') == 'FAILED')
        
        # Time span
        if self.log_entries:
            start_time = self.log_entries[0].timestamp
            end_time = self.log_entries[-1].timestamp
            duration = end_time - start_time
        else:
            duration = 0
        
        return {
            'total_entries': total_entries,
            'levels': levels,
            'duration': duration,
            'packets': {
                'sent': packets_sent,
                'received': packets_received,
                'bytes_sent': bytes_sent,
                'bytes_received': bytes_received
            },
            'tests': {
                'total': len(test_entries),
                'passed': tests_passed,
                'failed': tests_failed
            }
        }

class PerformanceTracker:
    """Tracks performance metrics"""
    
    def __init__(self):
        self.metrics = {}
        self.start_times = {}
    
    def start_metric(self, name: str):
        """Start tracking a metric"""
        self.start_times[name] = time.time()
    
    def end_metric(self, name: str) -> float:
        """End tracking a metric and return duration"""
        if name in self.start_times:
            duration = time.time() - self.start_times[name]
            if name not in self.metrics:
                self.metrics[name] = []
            self.metrics[name].append(duration)
            del self.start_times[name]
            return duration
        return 0.0
    
    def add_metric(self, name: str, value: float):
        """Add a metric value directly"""
        if name not in self.metrics:
            self.metrics[name] = []
        self.metrics[name].append(value)
    
    def get_metric_stats(self, name: str) -> Dict:
        """Get statistics for a metric"""
        if name not in self.metrics or not self.metrics[name]:
            return {}
        
        values = self.metrics[name]
        return {
            'count': len(values),
            'min': min(values),
            'max': max(values),
            'avg': sum(values) / len(values),
            'total': sum(values)
        }
    
    def get_all_stats(self) -> Dict:
        """Get statistics for all metrics"""
        return {name: self.get_metric_stats(name) for name in self.metrics}
    
    def clear_metrics(self):
        """Clear all metrics"""
        self.metrics.clear()
        self.start_times.clear()

class ConfigManager:
    """Manages configuration settings"""
    
    def __init__(self, config_file: str = "config.json"):
        self.config_file = config_file
        self.config = self.load_config()
    
    def load_config(self) -> Dict:
        """Load configuration from file"""
        default_config = {
            'device': {
                'default_packet_size': 8,
                'timeout_ms': 1000,
                'retry_count': 3
            },
            'testing': {
                'default_test_duration': 60,
                'echo_packet_count': 10,
                'stress_test_duration': 300
            },
            'logging': {
                'level': 'INFO',
                'save_packet_data': True,
                'auto_save_logs': True
            },
            'ui': {
                'auto_refresh_devices': True,
                'refresh_interval_ms': 5000,
                'auto_scroll_logs': True
            }
        }
        
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    loaded_config = json.load(f)
                    # Merge with defaults
                    default_config.update(loaded_config)
        except Exception as e:
            print(f"Error loading config: {e}, using defaults")
        
        return default_config
    
    def save_config(self):
        """Save configuration to file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=2)
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def get(self, key_path: str, default=None):
        """Get configuration value using dot notation"""
        keys = key_path.split('.')
        value = self.config
        
        for key in keys:
            if isinstance(value, dict) and key in value:
                value = value[key]
            else:
                return default
        
        return value
    
    def set(self, key_path: str, value):
        """Set configuration value using dot notation"""
        keys = key_path.split('.')
        config = self.config
        
        for key in keys[:-1]:
            if key not in config:
                config[key] = {}
            config = config[key]
        
        config[keys[-1]] = value
        self.save_config()
