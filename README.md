# 🚗 ESP32 OBD2 DPF Monitor (Astra J)

Real-time **DPF monitoring system** built on ESP32 with a round AMOLED display.  
Designed specifically for **Opel Astra J (A17DTJ)**, using BLE communication with ELM327-compatible adapters (e.g. vLinker MC).

---

## 📟 Target Hardware

- **Device:** VIEWE SMARTRING (ESP32-S3 + SH8601 AMOLED)  
- **Manufacturer Source:** https://github.com/VIEWESMART/VIEWE-SMARTRING  

This project is optimized for a **circular AMOLED display** with LVGL UI.

---

## 🔧 Features

- 🔥 **DPF Regeneration Detection**
  - Real-time regeneration status (PID 223274)
  - Visual alert when regeneration is active

- 🌫️ **Soot Level Monitoring**
  - DPF soot percentage (OEM PID)

- 🌡️ **Temperature Monitoring**
  - Coolant temperature (OEM PID)

- 🌬️ **Differential Pressure**
  - DPF pressure sensor values

- 📏 **Distance Between Regenerations**
  - Useful for tracking DPF health over time

- 📡 **BLE OBD Communication**
  - Works with adapters like **vLinker MC (BLE)**
  - Uses NimBLE stack on ESP32

- 🖥️ **Modern UI (LVGL)**
  - Optimized for round display
  - Clean, high-contrast layout
  - Large readable values for in-car usage

---

## 🧠 How It Works

1. Connects via BLE to OBD adapter  
2. Initializes ECU communication (CAN 500k)  
3. Sends OEM Mode 22 requests  
4. Parses ECU responses in real time  
5. Updates UI continuously  

---

## 📡 Used OBD PIDs (Opel Astra J)

| PID     | Description                      |
|--------|----------------------------------|
| 223275 | DPF soot level (%)              |
| 220005 | Coolant temperature             |
| 22007A | Differential pressure           |
| 223277 | Distance since last regeneration|
| 223274 | Regeneration status (%)         |

> ⚠️ These are **OEM-specific PIDs** and may not work on other vehicles.

---

## 🛠️ Tech Stack

- ESP-IDF  
- FreeRTOS  
- NimBLE (BLE stack)  
- LVGL (GUI)  
- SPI + SH8601 AMOLED driver  

---

## ⚠️ Notes

- This project is **vehicle-specific** (Opel Astra J ECU)  
- OEM PIDs may vary depending on:
  - ECU version  
  - Engine variant  
  - Software revision  

---

## 🚀 Future Improvements

- Auto-detection of working PIDs  
- Logging & statistics (regen cycles, soot trends)  
- Alerts (e.g. *"Do not stop engine during regen"*)  
- UI themes and customization  

---

## 📸 Preview

<img src="https://github.com/miery/ESP32-LGVL-OBD2-DPF-Monitor/blob/main/img/view.jpg" style="height:100%;"></img>
---

## 📄 License

MIT (or your preferred license)

---

## 🤝 Contributions

Feel free to fork, test on other vehicles, and submit improvements!
