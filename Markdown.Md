# KEJU-BW16-V1X | Dual-Band WiFi Dashboard

The **KEJU-BW16-V1X** is a specialized wireless auditing tool built on the **Realtek RTL8720DN (BW16)** architecture. Unlike standard 2.4GHz tools, this device utilizes the AmebaD dual-core processor to monitor and interact with both 2.4GHz and 5GHz network environments.

## 🛠 Hardware Specifications
- **Core**: BW16 (AmebaD RTL8720DN) ARM Cortex-M4 + M0.
- **Display**: 1.8" ST7735 TFT (128x160) via SPI1.
- **Input**: 3-button tactical navigation (Up, Down, Select).
- **Haptics**: Active Buzzer (PA26) for signal tracking.
- **Power**: LiPo compatible with ADC voltage monitoring (PA_0).



## ⚡ Software Features
- **Dual-Band Scan**: Real-time discovery of 2.4G and 5G SSIDs.
- **Infiltration Engine**: SSID cloning and beaconing for roaming analysis.
- **Tactical UI**: High-contrast Cyber-UI with stealth backlight dimming.
- **Audio Tracker**: RSSI-to-frequency audio feedback for signal locating.
- **Optimized Footprint**: Compiled at <35% flash usage for maximum stability.

## 🏗 Wiring Map
| Component | BW16 Pin |
| :--- | :--- |
| TFT MOSI / SCK | PA12 / PA14 |
| TFT CS / DC | PA15 / PA27 |
| Buttons | PA14, PA13, PA25 |
| Buzzer | PA26 |

---
*Disclaimer: This tool is intended for authorized security auditing and educational purposes only.*
