# HS-1



# 🦾 HS-1 Kinematic Suit — Real-Time Motion Tracking System

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--C3-orange.svg)
![Python](https://img.shields.io/badge/Python-3.11-green.svg)
![Framework](https://img.shields.io/badge/Visualizer-Ursina%20Engine-purple.svg)

**HS-1 Kinematic Suit** is a hardware-software system designed for real-time spatial orientation tracking of the human upper limb with continuous 3D rendering. The system utilizes a cascaded hardware architecture of **ESP32-C3 SuperMini** microcontrollers paired with high-precision **Bosch BNO055** 9-axis IMU sensors.

---

## 📌 Key Features

* **UART Cascade Data Bus:** Sequential inter-node data transmission eliminating the need for bulky central controllers.
* **Hardware Orientation Calculation:** Direct readout of absolute Euler angles (Pitch, Yaw, Roll) computed onboard by the Bosch BNO055 DSP fusion engine.
* **High Refresh Rate:** Real-time kinematic data stream sent to PC at **50 Hz** (20 ms interval).
* **Lightweight 3D Visualizer:** Python-based 3D scene implemented via **Ursina Engine** featuring non-blocking background JSON parsing.
* **Modular C++ Architecture:** Strictly defined node responsibilities (Master-Clock, Transmit Relay, USB Hub).

---

## 🛠️ Hardware Architecture

The system consists of three sequentially connected kinematic nodes:


```

┌─────────────────┐       UART       ┌──────────────────┐       UART       ┌─────────────────┐       USB       ┌─────────────────┐
│    Hand Node    ├─────────────────►│   Forearm Node   ├─────────────────►│  Shoulder Node  ├────────────────►│   PC / Python   │
│    (Node #1)    │  14 bytes/pkt    │    (Node #2)     │  28 bytes/pkt    │  (Master Hub)   │   JSON Stream   │  3D Visualizer  │
└─────────────────┘                  └──────────────────┘                  └─────────────────┘                 └─────────────────┘

```

### Component Specifications

| Node | Microcontroller | IMU Sensor | IMU Interface | UART RX / TX | Network Role |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Hand** | ESP32-C3 SuperMini | Bosch BNO055 | I2C (`0x28`) | `-` / `GPIO 4` | Master Clock Generator (50Hz) |
| **Forearm** | ESP32-C3 SuperMini | Bosch BNO055 | I2C (`0x28`) | `GPIO 1` / `GPIO 4` | Data Aggregator & Transmit Relay |
| **Shoulder** | ESP32-C3 SuperMini | Bosch BNO055 | I2C (`0x28`) | `GPIO 1` / `-` | Master Serial Hub to USB CDC |

---

## 📡 Transmission Protocol

### 1. Internal Binary Packet (UART Cascade)

Inter-node communication between ESP32 microcontrollers uses packed binary structures defined in `ArmPackets.h`:

```cpp
struct __attribute__((packed)) SensorData {
    float yaw;        // 4 bytes (Deg)
    float pitch;      // 4 bytes (Deg)
    float roll;       // 4 bytes (Deg)
    uint8_t calibSys; // 1 byte
    uint8_t calibGyro;// 1 byte
};                    // Total: 14 bytes

```

### 2. Final Output JSON Stream (Serial USB)

The Shoulder Hub aggregates all incoming frame packets and outputs a single-line JSON stream to the PC via USB CDC:

```json
{"shoulder":{"p":12.7,"y":223.9,"r":178.4},"forearm":{"p":45.2,"y":190.1,"r":12.0},"hand":{"p":5.1,"y":188.4,"r":2.3}}

```

---

## 💻 Software & 3D Visualization

The visualizer is built on **Ursina Engine** (Python 3.11). Serial communication runs in a dedicated daemon thread (`threading`) to prevent GUI thread blocking and eliminate rendering lag.

```
Project_Root/
├── app.py                     # Main Ursina visualizer script
├── requirements.txt           # Python dependencies
├── Models/                    # 3D CAD meshes (.obj format)
│   ├── shoulder.obj
│   ├── forearm.obj
│   └── hand.obj
└── Firmware/                  # PlatformIO C++ Source Code
    ├── hand/
    ├── forearm/
    └── shoulder/

```

---

## 🚀 Quick Start


```markdown
## 🚀 Quick Start

### 1. Download & Installation

You can clone the repository via Git or download the source code package directly:

#### Option A: Clone via Git (Recommended)
```bash
git clone [https://github.com/Perry132/HS-1-Kinematic-Suit.git](https://github.com/Perry132/HS-1-Kinematic-Suit.git)
cd HS-1-Kinematic-Suit

```

#### Option B: Direct ZIP Download

Download the latest source code archive directly:
[📥 Download HS-1 Kinematic Suit (.zip)](https://www.google.com/search?q=https://github.com/Perry132/HS-1-Kinematic-Suit/archive/refs/heads/main.zip)

---

### 2. Firmware Deployment

Flash each node with its corresponding PlatformIO project located in `Firmware/`:

```bash
# Upload Hand Firmware
platformio run --target upload --project-dir Firmware/hand

# Upload Forearm Firmware
platformio run --target upload --project-dir Firmware/forearm

# Upload Shoulder Firmware
platformio run --target upload --project-dir Firmware/shoulder

```

---

### 3. Python Environment Setup

Install all required dependencies using `requirements.txt`:

```bash
pip install -r requirements.txt

```

---

### 4. Running the Visualizer

Set your active COM port inside `app.py` (`SERIAL_PORT = 'COMx'`) and launch the application:

```bash
python app.py

```

```

<ElicitationsGroup message="Next steps for repo optimization:">
  <Elicitation label="Generate requirements.txt file contents" query="Provide the exact contents for the requirements.txt file for the HS-1 project"/>
  <Elicitation label="Add a Troubleshooting section to README.md" query="Add a comprehensive Troubleshooting section to the English README.md covering UART, COM port, and BNO055 issues"/>
</ElicitationsGroup>

```

### 3. Running the Visualizer

Specify your active COM port inside `app.py` (`SERIAL_PORT = 'COMx'`) and launch:

```bash
python app.py

```

---
## Documentation

[Review HS-1 Documentation](https://github.com/Perry1231/HS-1/blob/main/HS-1%20Short_Documentation.txt)
---

## 👤 Author

**Vladyslav Vytrykush**

* GitHub: [@Perry132](https://www.google.com/search?q=https://github.com/Perry132)
* Specialty: Electronics & Embedded Systems (Lviv Polytechnic National University)

---

*Developed as part of the HS-1 spatial orientation tracking suit project.*

```
