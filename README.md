# ESP8266 IoT Temperature Sensor with Web Configuration

A self-configurable ESP8266-based IoT temperature sensor that publishes readings to MQTT with an easy-to-use web interface for device setup. The device automatically handles WiFi and MQTT connections, falling back to an access point mode when needed for configuration.

This project provides a robust solution for remote temperature monitoring with the following key features:
- Web-based configuration interface for all device settings
- Automatic WiFi connection management with fallback AP mode
- Secure MQTT communication for temperature data publishing
- Persistent configuration storage using EEPROM
- Real-time temperature monitoring using Dallas temperature sensors
- Automatic recovery from connection failures

## Repository Structure
```
.
├── platformio.ini         # PlatformIO configuration file with build settings and dependencies
└── src/
    └── main.cpp          # Core application logic for WiFi, MQTT, and sensor management
```

## Usage Instructions

### Prerequisites
- PlatformIO IDE or CLI
- ESP8266 development board (ESP-01 or compatible)
- Dallas temperature sensor (DS18B20 or compatible)
- USB-to-Serial adapter for programming
- 3.3V power supply

Required Libraries:
- ESP8266WiFi
- PubSubClient (v2.8 or later)
- ESPAsyncWebServer-esphome (v3.4.0 or later)
- OneWire
- DallasTemperature

### Installation

1. Clone the repository:
```bash
git clone [repository-url]
cd [repository-name]
```

2. Install PlatformIO (if not already installed):
```bash
pip install platformio
```

3. Build the project:
```bash
platformio run
```

4. Upload to your ESP8266:
```bash
platformio run --target upload
```

### Quick Start

1. Power up the ESP8266 device
2. On first boot, the device creates an access point named "ESP8266-Access-Point"
3. Connect to this network using password "123456789"
4. Navigate to the device's IP address (typically 192.168.4.1)
5. Configure the following settings through the web interface:
   - Device ID
   - WiFi credentials
   - MQTT broker details
   - MQTT authentication

### More Detailed Examples

#### Temperature Monitoring Setup
```cpp
// Configure temperature sensor
OneWire onewire(2);  // Connect sensor to GPIO2
DallasTemperature sensors(&onewire);
sensors.begin();
```

#### MQTT Topic Structure
Temperature readings are published to:
```
[device-id]/temperature
```

### Troubleshooting

#### Common Issues

1. Device Not Connecting to WiFi
- Symptom: Device keeps creating access point
- Solution: 
  1. Connect to the AP "ESP8266-Access-Point"
  2. Verify WiFi credentials are correct
  3. Check WiFi signal strength

2. MQTT Connection Failures
- Symptom: No temperature data being published
- Debug steps:
  1. Verify MQTT broker IP and port
  2. Check MQTT credentials
  3. Ensure broker is accessible from the network

#### Debug Mode
Serial monitoring is available at 115200 baud rate:
```bash
platformio device monitor
```

## Data Flow
The device reads temperature data from the Dallas sensor, processes it locally, and publishes to an MQTT broker while maintaining WiFi connectivity.

```ascii
[Temperature Sensor] -> [ESP8266] -> [WiFi] -> [MQTT Broker]
                          |
                          v
                    [Web Config UI]
```

Component Interactions:
1. Temperature sensor readings occur every cycle
2. WiFi connection is monitored continuously
3. Web server handles configuration requests
4. MQTT client manages broker connection
5. EEPROM stores persistent configuration
6. Automatic fallback to AP mode on connection failure
7. Real-time temperature data publishing to MQTT broker