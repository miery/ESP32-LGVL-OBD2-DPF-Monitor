# 🚗 ESP32 OBD2 DPF Monitor (Opel Astra J)

Real-time **DPF monitoring system** running on ESP32 with a round AMOLED display.  
Designed for **Opel Astra J (A17DTJ)** using BLE communication with ELM327-compatible adapters (e.g. vLinker MC).

---

## 📟 Target Hardware

- **Device:** VIEWE SMARTRING (ESP32-S3 + SH8601 AMOLED)  
- **Display:** 466x466 round AMOLED  
- **MCU:** ESP32-S3  
- **Manufacturer Source:**  
  https://github.com/VIEWESMART/VIEWE-SMARTRING  

---

## 🔧 Features

- 🔥 **DPF Regeneration Detection**
  - Live regeneration status (PID 223274)
  - Visual + LED indication when active

- 🌫️ **Soot Level Monitoring**
  - OEM soot percentage

- 🌡️ **Coolant Temperature**
  - Real-time ECT data

- 🌬️ **Differential Pressure**
  - DPF pressure sensor

- 📏 **Distance Between Regenerations**
  - Helps track DPF condition

- 📡 **BLE OBD Communication**
  - NimBLE stack (ESP-IDF)
  - Tested with **vLinker MC (BLE)**

- 💡 **LED Feedback**
  - Idle → white
  - Regeneration → red

- 🖥️ **LVGL UI**
  - Optimized for round display
  - High contrast
  - Large readable values

---

## 🧠 How It Works

1. BLE scan for OBD adapter  
2. Connect to device (vLinker MC)  
3. Discover writable + notify characteristic  
4. Initialize OBD (AT commands, CAN 500k)  
5. Send cyclic OEM queries  
6. Parse ECU responses  
7. Update UI + LED state  

---

## 📡 Used OBD PIDs (Opel Astra J)

| PID     | Description                      |
|--------|----------------------------------|
| 223275 | DPF soot level (%)              |
| 220005 | Coolant temperature             |
| 223035 | Differential pressure           |
| 223277 | Distance since regeneration     |
| 223274 | Regeneration status (%)         |

> ⚠️ OEM-specific — may NOT work on other cars.

---

## 🧩 Module Responsibilities

### 🧠 `main`
- Application entry point
- Initializes subsystems
- Runs main loop

---

### 🖥️ `display`
- SPI + SH8601 initialization
- LVGL setup
- Framebuffer + flush callback

---

### 🎨 `ui`
- Layout creation (grid system)
- Labels and metric boxes
- Regeneration state visualization

---

### 📡 `ble_obd`
- BLE scanning + connection
- GATT characteristic discovery
- OBD command TX
- ECU response parsing (Mode 22)

---

### ⚙️ `obd_logic`
- State machine:
  - INIT → READY
- Command scheduler
- Data update pipeline

---

### 💡 `led`
- WS2812 LED strip control
- Visual feedback:
  - Idle
  - Regeneration active

---

## 🛠️ Tech Stack

- **ESP-IDF**
- **FreeRTOS**
- **NimBLE**
- **LVGL**
- **SPI (QSPI mode)**
- **WS2812 LED (RMT driver)**

---

## 📸 Preview

<img src="https://github.com/miery/ESP32-LGVL-OBD2-DPF-Monitor/blob/main/img/view.jpg"/> <img src="https://github.com/miery/ESP32-LGVL-OBD2-DPF-Monitor/blob/main/img/view2.jpg"/> 


---

## 🚀 Future Improvements

- 🔄 OBD parser as separate module
- 📊 Data logging (DPF history)
- ⚠️ Smart alerts (regen warnings)
- 🎨 UI themes / skins
- 🔍 Auto PID detection

---

## 🤝 Contributions

PRs welcome!

If you test this on:
- other Opel engines  
- other ECUs  
- different BLE adapters  

👉 let me know — especially if PIDs differ.

---

## 📜 License

MIT
