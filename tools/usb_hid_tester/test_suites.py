"""
Test Suites Module
Predefined test suites for different scenarios

Author: USB Key Authentication Team
Date: 2025-09-12
"""

import threading
import time
from typing import Dict, List, Callable
from hid_tester import HIDTester, TestCase, TestResult

class TestSuiteRunner:
    """Manages and runs predefined test suites"""
    
    def __init__(self, hid_tester: HIDTester):
        self.hid_tester = hid_tester
        self.current_suite = None
        self.running = False
        self.suite_thread = None
        
    def get_available_suites(self) -> Dict[str, Dict]:
        """Get list of available test suites"""
        return {
            'quick_validation': {
                'name': 'Quick Validation',
                'description': 'Fast validation of basic functionality',
                'duration_estimate': '2-3 minutes',
                'tests': ['basic_connectivity', 'echo_test'],
                'config': {'packet_size': 8, 'test_duration': 10}
            },
            'nxp_compatibility': {
                'name': 'NXP HID Generic Compatibility',
                'description': 'Full compatibility test with NXP example',
                'duration_estimate': '5-10 minutes',
                'tests': ['basic_connectivity', 'echo_test', 'data_integrity', 'performance'],
                'config': {'packet_size': 8, 'test_duration': 30}
            },
            'fido2_readiness': {
                'name': 'FIDO2 Readiness',
                'description': 'Test readiness for FIDO2 implementation',
                'duration_estimate': '10-15 minutes',
                'tests': ['basic_connectivity', 'echo_test', 'data_integrity', 'fido2_prep'],
                'config': {'packet_size': 64, 'test_duration': 60}
            },
            'comprehensive': {
                'name': 'Comprehensive Test',
                'description': 'Complete test suite including stress testing',
                'duration_estimate': '20-30 minutes',
                'tests': ['basic_connectivity', 'echo_test', 'data_integrity', 'performance', 'stress', 'fido2_prep'],
                'config': {'packet_size': 8, 'test_duration': 120}
            },
            'stress_endurance': {
                'name': 'Stress & Endurance',
                'description': 'Extended stress testing for stability',
                'duration_estimate': '30-60 minutes',
                'tests': ['basic_connectivity', 'stress'],
                'config': {'packet_size': 8, 'test_duration': 1800}  # 30 minutes
            },
            'performance_benchmark': {
                'name': 'Performance Benchmark',
                'description': 'Detailed performance analysis',
                'duration_estimate': '15-20 minutes',
                'tests': ['basic_connectivity', 'performance'],
                'config': {'packet_size': 8, 'test_duration': 300}  # 5 minutes
            }
        }
    
    def run_suite(self, suite_name: str, progress_callback: Callable = None, 
                  status_callback: Callable = None, result_callback: Callable = None) -> bool:
        """Run a predefined test suite"""
        
        if self.running:
            return False
        
        suites = self.get_available_suites()
        if suite_name not in suites:
            return False
        
        self.current_suite = suites[suite_name]
        self.running = True
        
        # Set callbacks
        if progress_callback:
            self.hid_tester.set_callback('progress', progress_callback)
        if status_callback:
            self.hid_tester.set_callback('status', status_callback)
        if result_callback:
            self.hid_tester.set_callback('result', result_callback)
        
        # Run in separate thread
        self.suite_thread = threading.Thread(
            target=self._run_suite_thread,
            args=(self.current_suite,)
        )
        self.suite_thread.start()
        
        return True
    
    def _run_suite_thread(self, suite_config: Dict):
        """Run test suite in separate thread"""
        try:
            tests = suite_config['tests']
            config = suite_config['config']
            
            # Run the test suite
            results = self.hid_tester.run_test_suite(tests, config)
            
            # Generate suite summary
            self._generate_suite_summary(suite_config, results)
            
        except Exception as e:
            print(f"Error running test suite: {e}")
        finally:
            self.running = False
    
    def _generate_suite_summary(self, suite_config: Dict, results: List[TestCase]):
        """Generate summary report for test suite"""
        summary = self.hid_tester.get_test_summary()
        
        print(f"\n=== {suite_config['name']} - Test Suite Summary ===")
        print(f"Description: {suite_config['description']}")
        print(f"Total Tests: {summary['total']}")
        print(f"Passed: {summary['passed']}")
        print(f"Failed: {summary['failed']}")
        print(f"Errors: {summary['errors']}")
        print(f"Skipped: {summary['skipped']}")
        print(f"Success Rate: {summary['success_rate']:.1f}%")
        print(f"Total Duration: {summary['total_duration']:.2f} seconds")
        print("=" * 50)
    
    def stop_suite(self):
        """Stop running test suite"""
        if self.running:
            self.hid_tester.stop_tests()
            self.running = False
    
    def is_running(self) -> bool:
        """Check if test suite is running"""
        return self.running
    
    def get_current_suite(self) -> Dict:
        """Get current running suite info"""
        return self.current_suite

class CustomTestBuilder:
    """Builder for creating custom test configurations"""
    
    def __init__(self):
        self.tests = []
        self.config = {}
        self.name = ""
        self.description = ""
    
    def set_name(self, name: str):
        """Set test suite name"""
        self.name = name
        return self
    
    def set_description(self, description: str):
        """Set test suite description"""
        self.description = description
        return self
    
    def add_connectivity_test(self):
        """Add basic connectivity test"""
        self.tests.append('basic_connectivity')
        return self
    
    def add_echo_test(self, packet_size: int = 8, num_packets: int = 10):
        """Add echo test with configuration"""
        self.tests.append('echo_test')
        self.config.update({
            'packet_size': packet_size,
            'echo_packets': num_packets
        })
        return self
    
    def add_data_integrity_test(self, packet_size: int = 8):
        """Add data integrity test"""
        self.tests.append('data_integrity')
        self.config.update({'packet_size': packet_size})
        return self
    
    def add_performance_test(self, duration_seconds: int = 30, packet_size: int = 8):
        """Add performance test"""
        self.tests.append('performance')
        self.config.update({
            'test_duration': duration_seconds,
            'packet_size': packet_size
        })
        return self
    
    def add_stress_test(self, duration_seconds: int = 60):
        """Add stress test"""
        self.tests.append('stress')
        self.config.update({'test_duration': duration_seconds})
        return self
    
    def add_fido2_test(self):
        """Add FIDO2 preparation test"""
        self.tests.append('fido2_prep')
        return self
    
    def set_packet_size(self, size: int):
        """Set default packet size"""
        self.config['packet_size'] = size
        return self
    
    def set_duration(self, seconds: int):
        """Set default test duration"""
        self.config['test_duration'] = seconds
        return self
    
    def build(self) -> Dict:
        """Build the custom test suite configuration"""
        return {
            'name': self.name or 'Custom Test Suite',
            'description': self.description or 'User-defined test configuration',
            'tests': self.tests,
            'config': self.config
        }

class TestScenarios:
    """Predefined test scenarios for specific use cases"""
    
    @staticmethod
    def nxp_generic_validation() -> Dict:
        """Test scenario matching NXP HID Generic example"""
        builder = CustomTestBuilder()
        return (builder
                .set_name("NXP Generic HID Validation")
                .set_description("Validates functionality matching NXP USB Device HID Generic example")
                .add_connectivity_test()
                .add_echo_test(packet_size=8, num_packets=20)
                .add_data_integrity_test(packet_size=8)
                .add_performance_test(duration_seconds=60, packet_size=8)
                .build())
    
    @staticmethod
    def fido2_migration_test() -> Dict:
        """Test scenario for FIDO2 migration readiness"""
        builder = CustomTestBuilder()
        return (builder
                .set_name("FIDO2 Migration Readiness")
                .set_description("Tests device readiness for FIDO2/WebAuthn implementation")
                .add_connectivity_test()
                .add_echo_test(packet_size=64, num_packets=10)
                .add_data_integrity_test(packet_size=64)
                .add_fido2_test()
                .add_performance_test(duration_seconds=120, packet_size=64)
                .build())
    
    @staticmethod
    def production_validation() -> Dict:
        """Test scenario for production device validation"""
        builder = CustomTestBuilder()
        return (builder
                .set_name("Production Device Validation")
                .set_description("Comprehensive validation for production devices")
                .add_connectivity_test()
                .add_echo_test(packet_size=8, num_packets=50)
                .add_echo_test(packet_size=64, num_packets=50)
                .add_data_integrity_test(packet_size=8)
                .add_data_integrity_test(packet_size=64)
                .add_performance_test(duration_seconds=300, packet_size=8)
                .add_performance_test(duration_seconds=300, packet_size=64)
                .add_stress_test(duration_seconds=600)
                .add_fido2_test()
                .build())
    
    @staticmethod
    def quick_smoke_test() -> Dict:
        """Quick smoke test for basic functionality"""
        builder = CustomTestBuilder()
        return (builder
                .set_name("Quick Smoke Test")
                .set_description("Fast validation of basic device functionality")
                .add_connectivity_test()
                .add_echo_test(packet_size=8, num_packets=5)
                .build())
    
    @staticmethod
    def regression_test() -> Dict:
        """Regression test for firmware updates"""
        builder = CustomTestBuilder()
        return (builder
                .set_name("Regression Test Suite")
                .set_description("Regression testing after firmware updates")
                .add_connectivity_test()
                .add_echo_test(packet_size=8, num_packets=30)
                .add_data_integrity_test(packet_size=8)
                .add_performance_test(duration_seconds=180, packet_size=8)
                .build())

class TestReportGenerator:
    """Generates detailed test reports"""
    
    def __init__(self):
        self.reports = []
    
    def generate_summary_report(self, suite_name: str, results: List[TestCase]) -> str:
        """Generate summary report text"""
        summary = self._calculate_summary(results)
        
        report = f"""
USB HID Test Report - {suite_name}
{'=' * 50}
Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}

SUMMARY:
--------
Total Tests: {summary['total']}
Passed: {summary['passed']} ({summary['pass_rate']:.1f}%)
Failed: {summary['failed']} ({summary['fail_rate']:.1f}%)
Errors: {summary['errors']} ({summary['error_rate']:.1f}%)
Skipped: {summary['skipped']} ({summary['skip_rate']:.1f}%)

Total Duration: {summary['total_duration']:.2f} seconds
Average Test Duration: {summary['avg_duration']:.2f} seconds

DETAILED RESULTS:
----------------
"""
        
        for result in results:
            status_symbol = {
                TestResult.PASSED: "✓",
                TestResult.FAILED: "✗",
                TestResult.ERROR: "⚠",
                TestResult.SKIPPED: "⊝"
            }.get(result.result, "?")
            
            report += f"{status_symbol} {result.name:<30} {result.result.value:<8} {result.duration:>8.2f}s\n"
            if result.details:
                report += f"   Details: {result.details}\n"
            if result.error_message:
                report += f"   Error: {result.error_message}\n"
            report += "\n"
        
        return report
    
    def generate_csv_report(self, results: List[TestCase]) -> str:
        """Generate CSV format report"""
        csv_content = "Test Name,Result,Duration (s),Details,Error Message\n"
        
        for result in results:
            csv_content += f'"{result.name}","{result.result.value}",{result.duration:.2f},'
            csv_content += f'"{result.details}","{result.error_message or ""}"\n'
        
        return csv_content
    
    def generate_json_report(self, suite_name: str, results: List[TestCase]) -> Dict:
        """Generate JSON format report"""
        return {
            'suite_name': suite_name,
            'timestamp': time.time(),
            'summary': self._calculate_summary(results),
            'results': [
                {
                    'name': r.name,
                    'description': r.description,
                    'result': r.result.value,
                    'duration': r.duration,
                    'details': r.details,
                    'error_message': r.error_message,
                    'data_sent': r.data_sent.hex() if r.data_sent else None,
                    'data_received': r.data_received.hex() if r.data_received else None
                }
                for r in results
            ]
        }
    
    def _calculate_summary(self, results: List[TestCase]) -> Dict:
        """Calculate summary statistics"""
        total = len(results)
        if total == 0:
            return {}
        
        passed = sum(1 for r in results if r.result == TestResult.PASSED)
        failed = sum(1 for r in results if r.result == TestResult.FAILED)
        errors = sum(1 for r in results if r.result == TestResult.ERROR)
        skipped = sum(1 for r in results if r.result == TestResult.SKIPPED)
        
        total_duration = sum(r.duration for r in results)
        avg_duration = total_duration / total
        
        return {
            'total': total,
            'passed': passed,
            'failed': failed,
            'errors': errors,
            'skipped': skipped,
            'pass_rate': (passed / total) * 100,
            'fail_rate': (failed / total) * 100,
            'error_rate': (errors / total) * 100,
            'skip_rate': (skipped / total) * 100,
            'total_duration': total_duration,
            'avg_duration': avg_duration
        }
