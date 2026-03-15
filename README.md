# ❄️ Smart AC Controller (PWA + ESP8266)

A premium, modern web interface and hardware firmware for controlling air conditioners via an ESP8266. This project transforms a standard AC remote into a smart controller using transistors as digital switches.

## 🚀 Overview

This project consists of two main parts:
1.  **A Premium PWA Frontend**: A high-end, responsive web app built with a modern "Glassmorphism" design. It features a custom temperature dial, mode selection, and interactive controls.
2.  **ESP8266 Firmware**: A custom Arduino-based firmware that acts as a standalone WiFi Access Point, hosting the web app and responding to commands by switching hardware pins.

---

## 🛠️ Technology Stack

### **Frontend (The Remote UI)**
*   **HTML5 & CSS3**: Custom design using the **Inter** font, featuring smooth gradients, subtle micro-animations, and a highly polished "app-like" mobile experience.
*   **Vanilla JavaScript**: Handles the complex mathematics of the temperature dial (270° interactive arc) and communicates with the ESP8266 API.
*   **PWA (Progressive Web App)**: 
    *   **Service Workers (`sw.js`)**: Enabling offline support and instant loading.
    *   **Web Manifest (`manifest.json`)**: Allows "Add to Home Screen" on iOS and Android for a full-screen, status-bar-hidden native app experience.

### **Backend (Firmware)**
*   **C++ / Arduino**: Running on the **ESP8266 (NodeMCU 1.0)**.
*   **WiFi AP Mode**: The ESP8266 creates its own WiFi hotspot (`Akil_AC_Control`), meaning it works even without an existing internet connection.
*   **PROGMEM Storage**: The entire web application is compressed and embedded directly into the ESP8266's flash memory via `web_data.h` for high-performance serving.

---

## 🔌 Hardware Setup

The controller works by using **BC547 NPN Transistors** to "press" buttons on a physical AC remote by shorting the button contacts to ground when a signal is received from the ESP8266.

### **Pin Mapping**
| Function | ESP8266 Pin (NodeMCU) | Component |
| :--- | :--- | :--- |
| **Power** | `D1` | BC547 Base (via 1kΩ resistor) |
| **Temp UP** | `D2` | BC547 Base (via 1kΩ resistor) |
| **Temp DOWN** | `D5` | BC547 Base (via 1kΩ resistor) |

---

## 📈 Current Project Status

- [x] **Universal UI**: Fully fixed and proportional temperature dial (mathematically accurate 270° arc).
- [x] **Real-time Feedback**: Added a **Connection Status Badge** (🟢 Connected / 🔴 Offline) that pings the ESP8266 every 5 seconds.
- [x] **PWA Optimized**: Updated to **Cache v2**, ensuring that Home Screen installations on iPhone/Android pick up the latest UI changes.
- [x] **Code Stability**: Split the firmware into `.ino` and `.h` files to bypass the Arduino IDE's preprocessor bugs with JavaScript keywords.
- [x] **API Connectivity**: Wired all UI buttons (Power, Temp+, Temp-) to the real ESP8266 hardware endpoints.

### **Endpoints Implemented**
*   `GET /power`: Toggles the AC Power pin.
*   `GET /tempup`: Triggers the Temperature Up pin.
*   `GET /tempdown`: Triggers the Temperature Down pin.
*   `GET /schedule?time=HH:MM`: (Backend ready, requires NTP).

---

## 📖 How to Use

1.  **Flash the ESP8266**: Open `AC_Controller/AC_Controller.ino` in Arduino IDE and upload it to your board.
2.  **Connect to WiFi**: On your phone, connect to the WiFi network **`Akil_AC_Control`** (Password: `12345678`).
3.  **Launch the App**: Open your browser and go to `http://192.168.4.1`.
4.  **Install**: Select "Add to Home Screen" from your browser menu to use it as a standalone app.

---

*Developed by Akil - 2026*
