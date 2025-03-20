# Arduino Temperature Controller

## Overview
A temperature control system supporting both heating and cooling modes, featuring an OLED display and button interface. The system maintains temperatures from maximum cooling to 60°C.

## Hardware Requirements
- Arduino board
- SSD1306 OLED display (128x64)
- Temperature sensor (analog)
- Relay module
- PWM-controlled output device
- 3 push buttons
- Voltage sensor/divider
- 9V power supply

## Features
- **Temperature Control**: Heating (≥30°C) and cooling (<30°C) modes
- **Temperature Presets**: -10°C (Max cooling), 0°C, 15°C, 30°C, 45°C, 60°C
- **User Interface**: OLED display with three-button control
- **Safety Features**: Voltage monitoring, auto-shutdown after 5 minutes
- **Status Indicators**: Current temperature, selected target, on/off state

## Usage
1. Select desired temperature with increase/decrease buttons
2. Toggle power with center button
3. System regulates temperature automatically
4. Display shows current and target temperatures
