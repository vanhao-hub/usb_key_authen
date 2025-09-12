"""
HID Tester Module
Core testing functionality for USB HID devices

Author: USB Key Authentication Team
Date: 2025-09-12
"""

import time
import random
import threading
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from enum import Enum

class TestResult(Enum):
    """Test result enumeration"""
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    ERROR = "ERROR"

@dataclass
class TestCase:
    """Individual test case data"""
    name: str
    description: str
    result: TestResult
    duration: float
    details: str
    error_message: Optional[str] = None
    data_sent: Optional[bytes] = None
    data_received: Optional[bytes] = None

class HIDTester:
    """Core HID testing functionality"""
    
    def __init__(self, device_manager):
        self.device_manager = device_manager
        self.test_results = []
        self.callbacks = {
            'progress': None,
            'status': None,
            'result': None
        }
        self.stop_flag = False
        
    def set_callback(self, event: str, callback: Callable):
        """Set event callback"""
        if event in self.callbacks:
            self.callbacks[event] = callback
    
    def _notify_callback(self, event: str, data=None):
        """Notify callback if set"""
        if self.callbacks.get(event):
            self.callbacks[event](data)
    
    def basic_connectivity_test(self) -> TestCase:
        """Test basic device connectivity"""
        test_name = "Basic Connectivity"
        start_time = time.time()
        
        try:
            # Check if device is connected
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description="Verify device connection",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            # Get device info
            device_info = self.device_manager.get_device_info()
            if not device_info:
                return TestCase(
                    name=test_name,
                    description="Verify device connection",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Cannot get device information",
                    error_message="Device info unavailable"
                )
            
            details = f"Connected to {device_info.get('manufacturer', 'Unknown')} {device_info.get('product', 'Unknown')}"
            details += f" (VID: 0x{device_info.get('vid', 0):04X}, PID: 0x{device_info.get('pid', 0):04X})"
            
            return TestCase(
                name=test_name,
                description="Verify device connection",
                result=TestResult.PASSED,
                duration=time.time() - start_time,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description="Verify device connection",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during connectivity test",
                error_message=str(e)
            )
    
    def echo_test(self, packet_size: int = 8, num_packets: int = 10) -> TestCase:
        """NXP-compatible echo test"""
        test_name = f"Echo Test ({packet_size} bytes)"
        start_time = time.time()
        
        try:
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description=f"Send {num_packets} packets and verify echo response",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            successful_echoes = 0
            failed_echoes = 0
            total_latency = 0
            
            for i in range(num_packets):
                if self.stop_flag:
                    break
                
                # Generate test pattern
                test_data = self._generate_test_pattern(packet_size, i)
                
                # Send data
                send_start = time.time()
                if not self.device_manager.send_data(test_data):
                    failed_echoes += 1
                    continue
                
                # Wait for echo response
                received_data = self.device_manager.receive_data(timeout_ms=1000)
                latency = time.time() - send_start
                
                if received_data and received_data == test_data:
                    successful_echoes += 1
                    total_latency += latency
                else:
                    failed_echoes += 1
                
                # Update progress
                progress = (i + 1) / num_packets * 100
                self._notify_callback('progress', progress)
                
                time.sleep(0.01)  # Small delay between packets
            
            success_rate = (successful_echoes / num_packets) * 100 if num_packets > 0 else 0
            avg_latency = (total_latency / successful_echoes) if successful_echoes > 0 else 0
            
            details = f"Success rate: {success_rate:.1f}% ({successful_echoes}/{num_packets})"
            details += f", Avg latency: {avg_latency*1000:.1f}ms"
            
            result = TestResult.PASSED if success_rate >= 90 else TestResult.FAILED
            
            return TestCase(
                name=test_name,
                description=f"Send {num_packets} packets and verify echo response",
                result=result,
                duration=time.time() - start_time,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description=f"Send {num_packets} packets and verify echo response",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during echo test",
                error_message=str(e)
            )
    
    def data_integrity_test(self, packet_size: int = 8) -> TestCase:
        """Test data integrity with various patterns"""
        test_name = f"Data Integrity Test ({packet_size} bytes)"
        start_time = time.time()
        
        try:
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description="Test data integrity with various patterns",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            test_patterns = [
                b'\x00' * packet_size,  # All zeros
                b'\xFF' * packet_size,  # All ones
                bytes(range(packet_size)),  # Sequential
                bytes([(i * 17) % 256 for i in range(packet_size)]),  # Prime pattern
                bytes([random.randint(0, 255) for _ in range(packet_size)])  # Random
            ]
            
            successful_tests = 0
            
            for i, pattern in enumerate(test_patterns):
                if self.stop_flag:
                    break
                
                # Send pattern
                if not self.device_manager.send_data(pattern):
                    continue
                
                # Receive response
                received = self.device_manager.receive_data(timeout_ms=1000)
                
                if received and received == pattern:
                    successful_tests += 1
                
                progress = (i + 1) / len(test_patterns) * 100
                self._notify_callback('progress', progress)
            
            success_rate = (successful_tests / len(test_patterns)) * 100
            details = f"Pattern tests passed: {successful_tests}/{len(test_patterns)} ({success_rate:.1f}%)"
            
            result = TestResult.PASSED if success_rate >= 80 else TestResult.FAILED
            
            return TestCase(
                name=test_name,
                description="Test data integrity with various patterns",
                result=result,
                duration=time.time() - start_time,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description="Test data integrity with various patterns",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during data integrity test",
                error_message=str(e)
            )
    
    def performance_test(self, duration_seconds: int = 30, packet_size: int = 8) -> TestCase:
        """Performance and throughput test"""
        test_name = f"Performance Test ({duration_seconds}s)"
        start_time = time.time()
        
        try:
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description=f"Measure throughput over {duration_seconds} seconds",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            packets_sent = 0
            packets_received = 0
            bytes_sent = 0
            bytes_received = 0
            latencies = []
            
            end_time = start_time + duration_seconds
            
            while time.time() < end_time and not self.stop_flag:
                # Generate test data
                test_data = self._generate_test_pattern(packet_size, packets_sent)
                
                # Send with timing
                send_start = time.time()
                if self.device_manager.send_data(test_data):
                    packets_sent += 1
                    bytes_sent += len(test_data)
                    
                    # Try to receive response
                    received = self.device_manager.receive_data(timeout_ms=100)
                    if received:
                        packets_received += 1
                        bytes_received += len(received)
                        latency = time.time() - send_start
                        latencies.append(latency)
                
                # Update progress
                elapsed = time.time() - start_time
                progress = (elapsed / duration_seconds) * 100
                self._notify_callback('progress', progress)
                
                time.sleep(0.001)  # Small delay to prevent overwhelming
            
            actual_duration = time.time() - start_time
            
            # Calculate metrics
            send_rate = packets_sent / actual_duration if actual_duration > 0 else 0
            recv_rate = packets_received / actual_duration if actual_duration > 0 else 0
            throughput_bps = bytes_received / actual_duration if actual_duration > 0 else 0
            avg_latency = sum(latencies) / len(latencies) if latencies else 0
            max_latency = max(latencies) if latencies else 0
            
            details = f"Send rate: {send_rate:.1f} pkt/s, Recv rate: {recv_rate:.1f} pkt/s"
            details += f", Throughput: {throughput_bps:.0f} B/s"
            details += f", Avg latency: {avg_latency*1000:.1f}ms, Max latency: {max_latency*1000:.1f}ms"
            
            # Performance criteria (adjust as needed)
            result = TestResult.PASSED if send_rate > 10 and recv_rate > 5 else TestResult.FAILED
            
            return TestCase(
                name=test_name,
                description=f"Measure throughput over {duration_seconds} seconds",
                result=result,
                duration=actual_duration,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description=f"Measure throughput over {duration_seconds} seconds",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during performance test",
                error_message=str(e)
            )
    
    def stress_test(self, duration_seconds: int = 60) -> TestCase:
        """Stress test with continuous operation"""
        test_name = f"Stress Test ({duration_seconds}s)"
        start_time = time.time()
        
        try:
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description=f"Continuous operation for {duration_seconds} seconds",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            operations = 0
            errors = 0
            end_time = start_time + duration_seconds
            
            while time.time() < end_time and not self.stop_flag:
                try:
                    # Random packet size (8 or 64 bytes)
                    packet_size = random.choice([8, 64])
                    test_data = self._generate_test_pattern(packet_size, operations)
                    
                    # Send data
                    if self.device_manager.send_data(test_data):
                        operations += 1
                        
                        # Try to receive (don't wait long)
                        self.device_manager.receive_data(timeout_ms=10)
                    else:
                        errors += 1
                    
                    # Random delay
                    time.sleep(random.uniform(0.001, 0.010))
                    
                except Exception:
                    errors += 1
                
                # Update progress
                elapsed = time.time() - start_time
                progress = (elapsed / duration_seconds) * 100
                self._notify_callback('progress', progress)
            
            actual_duration = time.time() - start_time
            error_rate = (errors / operations) * 100 if operations > 0 else 100
            
            details = f"Operations: {operations}, Errors: {errors}, Error rate: {error_rate:.2f}%"
            
            result = TestResult.PASSED if error_rate < 5 else TestResult.FAILED
            
            return TestCase(
                name=test_name,
                description=f"Continuous operation for {duration_seconds} seconds",
                result=result,
                duration=actual_duration,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description=f"Continuous operation for {duration_seconds} seconds",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during stress test",
                error_message=str(e)
            )
    
    def fido2_preparation_test(self) -> TestCase:
        """Test FIDO2 compatibility requirements"""
        test_name = "FIDO2 Preparation"
        start_time = time.time()
        
        try:
            if not self.device_manager.is_connected():
                return TestCase(
                    name=test_name,
                    description="Test 64-byte packet handling for FIDO2",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Device not connected",
                    error_message="No device connected"
                )
            
            # Test 64-byte packets
            test_data_64 = self._generate_test_pattern(64, 0)
            
            # Send 64-byte packet
            if not self.device_manager.send_data(test_data_64):
                return TestCase(
                    name=test_name,
                    description="Test 64-byte packet handling for FIDO2",
                    result=TestResult.FAILED,
                    duration=time.time() - start_time,
                    details="Failed to send 64-byte packet",
                    error_message="Send failed"
                )
            
            # Try to receive response
            received = self.device_manager.receive_data(timeout_ms=2000)
            
            if received and len(received) == 64:
                details = "64-byte packet handling: SUPPORTED"
                result = TestResult.PASSED
            else:
                details = f"64-byte packet handling: FAILED (received {len(received) if received else 0} bytes)"
                result = TestResult.FAILED
            
            return TestCase(
                name=test_name,
                description="Test 64-byte packet handling for FIDO2",
                result=result,
                duration=time.time() - start_time,
                details=details
            )
            
        except Exception as e:
            return TestCase(
                name=test_name,
                description="Test 64-byte packet handling for FIDO2",
                result=TestResult.ERROR,
                duration=time.time() - start_time,
                details="Exception during FIDO2 preparation test",
                error_message=str(e)
            )
    
    def _generate_test_pattern(self, size: int, sequence: int) -> bytes:
        """Generate test data pattern"""
        pattern = []
        for i in range(size):
            # Create a recognizable pattern with sequence number
            value = (sequence + i) % 256
            pattern.append(value)
        return bytes(pattern)
    
    def run_test_suite(self, selected_tests: List[str], config: Dict) -> List[TestCase]:
        """Run a suite of tests"""
        results = []
        total_tests = len(selected_tests)
        
        self.stop_flag = False
        self._notify_callback('status', "Starting test suite...")
        
        for i, test_name in enumerate(selected_tests):
            if self.stop_flag:
                # Add skipped tests
                for remaining_test in selected_tests[i:]:
                    results.append(TestCase(
                        name=remaining_test,
                        description="Test skipped due to stop request",
                        result=TestResult.SKIPPED,
                        duration=0,
                        details="Test execution stopped by user"
                    ))
                break
            
            self._notify_callback('status', f"Running {test_name}...")
            
            # Run specific test
            if test_name == "basic_connectivity":
                result = self.basic_connectivity_test()
            elif test_name == "echo_test":
                packet_size = int(config.get('packet_size', 8))
                result = self.echo_test(packet_size=packet_size)
            elif test_name == "data_integrity":
                packet_size = int(config.get('packet_size', 8))
                result = self.data_integrity_test(packet_size=packet_size)
            elif test_name == "performance":
                duration = int(config.get('test_duration', 30))
                packet_size = int(config.get('packet_size', 8))
                result = self.performance_test(duration_seconds=duration, packet_size=packet_size)
            elif test_name == "stress":
                duration = int(config.get('test_duration', 60))
                result = self.stress_test(duration_seconds=duration)
            elif test_name == "fido2_prep":
                result = self.fido2_preparation_test()
            else:
                result = TestCase(
                    name=test_name,
                    description="Unknown test",
                    result=TestResult.ERROR,
                    duration=0,
                    details="Test not implemented",
                    error_message="Unknown test type"
                )
            
            results.append(result)
            self._notify_callback('result', result)
            
            # Update overall progress
            progress = (i + 1) / total_tests * 100
            self._notify_callback('progress', progress)
        
        self.test_results = results
        self._notify_callback('status', "Test suite completed")
        return results
    
    def stop_tests(self):
        """Stop running tests"""
        self.stop_flag = True
    
    def get_test_summary(self) -> Dict:
        """Get summary of test results"""
        if not self.test_results:
            return {}
        
        total = len(self.test_results)
        passed = sum(1 for r in self.test_results if r.result == TestResult.PASSED)
        failed = sum(1 for r in self.test_results if r.result == TestResult.FAILED)
        errors = sum(1 for r in self.test_results if r.result == TestResult.ERROR)
        skipped = sum(1 for r in self.test_results if r.result == TestResult.SKIPPED)
        
        total_duration = sum(r.duration for r in self.test_results)
        success_rate = (passed / total) * 100 if total > 0 else 0
        
        return {
            'total': total,
            'passed': passed,
            'failed': failed,
            'errors': errors,
            'skipped': skipped,
            'success_rate': success_rate,
            'total_duration': total_duration,
            'results': self.test_results
        }
