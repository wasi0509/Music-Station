# 🎵 Music Station

A DIY music display and remote-control project based on an **ESP32-S3**.

The device displays the currently playing music from a computer and provides physical controls for playback and system volume.

> **Project status:** 🚧 In development

---

## 📌 Overview

Music Station consists of an **ESP32-S3**, a color TFT display, physical buttons, and a Windows-based Python music bridge.

The ESP32 communicates with the computer over Wi-Fi to retrieve:

* 🎵 Song title
* 👤 Artist
* 💿 Album
* 🖼️ Album artwork
* ⏱️ Current playback position
* ⏳ Song duration
* 🔊 System volume
* ▶️ Playback state

The ESP32 can also send commands to the computer:

* ▶️ Play / Pause
* ⏮️ Previous track
* ⏭️ Next track
* 🔊 Volume Up
* 🔉 Volume Down

The ESP32 itself does **not** play audio. Audio continues to play through the computer's existing audio device.

---

## 🧩 Hardware

### ESP32

**ESP32-S3 N16R8 Development Board**

* ESP32-S3
* N16R8 configuration
* USB Type-C
* Wi-Fi
* Used as the main controller for the project

---

### TFT Display

**2.0-inch IPS TFT LCD — ST7789**

Name: 高清IPS 2.0寸TFT液晶显示屏240*320彩屏 ST7789模块 LCD全视角

Specifications:

* Resolution: **240 × 320**
* Color IPS LCD
* ST7789 display controller
* Full viewing angle
* SPI interface

Display pins:

| Pin | Function        |
| --- | --------------- |
| GND | Ground          |
| VCC | Power           |
| SCL | SPI Clock       |
| SDA | SPI Data / MOSI |
| RST | Reset           |
| DC  | Data / Command  |
| CS  | Chip Select     |
| BL  | Backlight       |

The TFT is currently the **main display** used by the project.

---

### E-Paper Display

**Good Display GDEY0266T90H**

Specifications:

* Size: **2.66-inch**
* Resolution: **360 × 184**
* SPI interface
* 24-pin connector
* 0.5 mm pin pitch
* Model: **GDEY0266T90H**

The e-paper display is planned for the project but is **not currently integrated into the main system**.

---

### Additional Components

* Breadboard
* Jumper wires
* Push buttons
* Rotary push button

---

## 🖥️ Software Architecture

```text
┌─────────────────────────────┐
│        Music Source         │
│                             │
│   YouTube Music / Browser   │
│        Windows PC           │
└──────────────┬──────────────┘
               │
               │ Windows Media
               │ Session API
               ▼
┌─────────────────────────────┐
│       Python Music Bridge   │
│                             │
│  • Song information         │
│  • Artist / Album           │
│  • Album artwork            │
│  • Position / Duration      │
│  • System volume            │
│  • Playback controls        │
│                             │
│        HTTP Server          │
│          :8080              │
└──────────────┬──────────────┘
               │
               │ Wi-Fi / HTTP
               ▼
┌─────────────────────────────┐
│          ESP32-S3           │
│                             │
│  • ST7789 Display           │
│  • Physical Buttons         │
│  • Music Controls           │
│  • Volume Controls          │
└─────────────────────────────┘
```

---

## 📂 Project Structure

```text
Music Station/
│
├── ESP32/
│   └── music_player/
│       └── music_player.ino
│
├── Bridge/
│   └── music_bridge.py
│
├── README.md
├── platformio.ini
└── .gitignore
```

### ESP32

The ESP32 firmware is responsible for:

* Connecting to Wi-Fi
* Communicating with the music bridge
* Updating the TFT display
* Displaying album artwork
* Reading physical buttons
* Sending playback commands
* Sending volume commands

### Python Bridge

The Python bridge runs on the Windows computer and acts as the connection between the music application and the ESP32.

It provides an HTTP API on port `8080`.

---

## 🌐 Communication

The ESP32 communicates with the Python bridge using HTTP over the local Wi-Fi network.

### API Endpoints

| Endpoint           | Function                      |
| ------------------ | ----------------------------- |
| `/api/status`      | Get current music information |
| `/album_art.jpg`   | Get current album artwork     |
| `/api/playpause`   | Play / Pause                  |
| `/api/next`        | Next track                    |
| `/api/previous`    | Previous track                |
| `/api/volume/up`   | Increase system volume        |
| `/api/volume/down` | Decrease system volume        |

Example:

```text
http://<COMPUTER-IP>:8080/api/status
```

---

## 🎨 Display

The TFT display currently shows:

* Song title
* Artist
* Album
* Album artwork
* Playback progress
* Current time
* Total duration
* Play / Pause status
* Previous / Next controls

Volume is displayed as a **temporary popup** when the volume buttons are pressed rather than as a permanent volume bar.

---

## 🎛️ Controls

The physical buttons are used for:

| Control      | Function                        |
| ------------ | ------------------------------- |
| Play / Pause | Toggle playback                 |
| Previous     | Previous track                  |
| Next         | Next track                      |
| Volume Up    | Increase computer system volume |
| Volume Down  | Decrease computer system volume |

The volume control changes the **Windows computer's system volume**, not the volume of the ESP32.

---

## 🔌 Current TFT Wiring

Current ESP32-S3 pin assignments:

| TFT        | ESP32-S3 |
| ---------- | -------: |
| CS         |  GPIO 10 |
| DC         |   GPIO 9 |
| RST        |   GPIO 8 |
| SCLK / SCL |  GPIO 12 |
| MOSI / SDA |  GPIO 11 |
| BL         |   GPIO 7 |
| GND        |      GND |
| VCC        |    3.3V* |

> ⚠️ Confirm the exact power/logic requirements of the specific ST7789 module before final wiring.

---

## 🔘 Current Button Pins

| Button       | ESP32-S3 GPIO |
| ------------ | ------------: |
| Play / Pause |        GPIO 1 |
| Previous     |        GPIO 2 |
| Next         |        GPIO 4 |
| Volume Up    |        GPIO 5 |
| Volume Down  |        GPIO 6 |

The buttons use the ESP32's internal pull-up resistors.

---

## 🛠️ Development Environment

### ESP32

* Arduino IDE / PlatformIO
* ESP32 Arduino framework
* Adafruit GFX
* Adafruit ST7789
* TJpg_Decoder
* ArduinoJson

### Windows Music Bridge

* Python
* Windows Runtime media controls
* pycaw
* comtypes
* HTTP server

---

## 🚧 Current Development Status

### Completed

* [x] ESP32-S3 setup
* [x] ST7789 TFT display
* [x] Wi-Fi connection
* [x] Python music bridge
* [x] Music metadata retrieval
* [x] Playback state
* [x] Song position
* [x] Song duration
* [x] Windows system volume control
* [x] Play / Pause control
* [x] Previous / Next control
* [x] Volume control
* [x] Album artwork retrieval
* [x] Album artwork transfer to ESP32

### In Progress

* [ ] Improve album artwork rendering
* [ ] Smooth playback progress updates
* [ ] Reduce display flickering
* [ ] Improve UI design
* [ ] Improve text layout and truncation
* [ ] Improve volume popup
* [ ] Integrate e-paper display

---

## 🔮 Future Ideas

Possible future improvements:

* iPhone music control
* Better animations
* Improved album-art scaling
* Touch controls
* Rotary encoder navigation
* E-paper version
* More polished music-player interface
* Multiple music-service support


