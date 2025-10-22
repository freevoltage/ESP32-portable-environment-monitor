#!/usr/bin/env python3
"""Script to automatically connect and reconnect to the serial monitor for the ESP32 BME280


Returns:
    _type_: _description_
"""

import sys
import time
import serial
import serial.tools.list_ports


def find_esp32_port():
    """Find the ESP32 USB port automatically"""
    ports = serial.tools.list_ports.comports()
    for _port in ports:
        # Look for common ESP32 identifiers
        if any(
            x in _port.description.lower()
            for x in ["cp210", "ch340", "usb serial", "uart"]
        ):
            return _port.device
        if "usbmodem" in _port.device or "ttyUSB" in _port.device:
            return _port.device
    return None


def monitor_serial(port=None, baudrate=115200):
    """Monitor serial with auto-reconnect"""
    ser = None

    while True:
        try:
            # Find port if not specified
            if port is None:
                port = find_esp32_port()
                if port is None:
                    print("Waiting for ESP32 to connect...")
                    time.sleep(1)
                    continue

            # Try to connect
            if ser is None or not ser.is_open:
                print(f"\nConnecting to {port}...")
                ser = serial.Serial(port, baudrate, timeout=1)
                print("Connected!")

            # Read and print data
            if ser.in_waiting:
                line = ser.readline()
                try:
                    print(line.decode("utf-8", errors="ignore"), end="")
                except:
                    pass

        except (serial.SerialException, OSError) as e:
            if ser:
                try:
                    ser.close()
                except:
                    pass
                ser = None

            print(f"\nConnection lost: {e}")
            print("Waiting for device to reconnect...")
            port = None  # Reset port to trigger re-detection
            time.sleep(2)

        except KeyboardInterrupt:
            print("\n\nExiting...")
            if ser:
                ser.close()
            sys.exit(0)


if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else None
    monitor_serial(port)
