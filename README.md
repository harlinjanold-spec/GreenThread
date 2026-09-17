# 🤖 GreenThread - ESP32 Smart Rover

GreenThread is an advanced, fully integrated ESP32 rover that drives around and analyzes soil and plants using a suite of environmental sensors and a lowering mechanism!

## Features
- **PlantBot OS**: A beautifully animated graphical dashboard on an ST7735 1.8" TFT display.
- **Remote Control**: Python-based Tkinter UI to drive the bot and scan via USB Serial.
- **Drop Mechanism**: Uses a 28BYJ-48 Stepper motor to physically lower a sensor array into the soil.
- **Environmental Telemetry**: Live Temperature and Humidity using a DHT11 sensor.
- **Soil Moisture**: Analog soil moisture sensor integration.
- **Spectroscopy**: AS7265x Triad Spectroscopy sensor for deep material analysis!

---

## 🚀 Quick Start

### 1. Hardware Requirements
- ESP32 Development Board
- ST7735 1.8" SPI TFT Display
- DHT11 Sensor
- Analog Soil Moisture Sensor
- AS7265x Spectroscopy Sensor
- 2x DC Motors & MX1508/L298N Motor Driver
- 1x 28BYJ-48 Stepper Motor & ULN2003 Driver

### 2. Wiring Guide
Please follow this wiring table strictly. The AS7265x SCL pin has been moved to D33 to avoid a conflict with the Display CS pin on D22!

| Component | Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- | :--- |
| **Display** | CS | **D22** | Chip Select |
| **Display** | RST | **D4** | Reset |
| **Display** | DC | **D2** | Data/Command |
| **Display** | SDA | **D13** | MOSI |
| **Display** | SCK | **D14** | Clock |
| **AS7265x** | SDA | **D21** | I2C Data |
| **AS7265x** | SCL | **D33** | I2C Clock (Special Pin!) |
| **DHT11** | OUT | **D32** | Temp/Humidity Data |
| **Moisture** | A0 | **D34** | Analog In |
| **Stepper** | IN1..4 | **19, 18, 5, 17** | ULN2003 Driver (Coils A,B,C,D) |
| **DC Motor** | IN1, IN2 | **26, 27** | Forward/Reverse Control |

*(Ensure all devices share a common GND with the ESP32!)*

### 3. Flashing the Firmware
This project is built using PlatformIO.
```bash
pio run --target upload
```

### 4. Running the Remote Control
Once the firmware is flashed, make sure you CLOSE your Serial Monitor. Then, run the Python controller from the `pc_controller` folder:
```bash
cd pc_controller
pip install pyserial
python remote_ui.py
```
