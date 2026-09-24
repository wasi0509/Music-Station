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

### TFT Display

**2.0-inch IPS TFT LCD — ST7789**

高清IPS 2.0寸TFT液晶显示屏240*320彩屏 ST7789模块 LCD全视角

* Resolution: **240 × 320**
* IPS LCD
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

### E-Paper Display

**Good Display GDEY0266T90H**

* Size: **2.66-inch**
* Resolution: **360 × 184**
* SPI interface
* 24-pin connector
* 0.5 mm pin pitch
* Model: **GDEY0266T90H**

The e-paper display is planned for the project but is **not currently integrated**.

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
│  • Volume Controls           │
└─────────────────────────────┘
```

---

# ⚙️ Setup & Run

## 1. Requirements

### Hardware

Connect the ESP32-S3 to the ST7789 display and buttons according to the wiring described in the [Current TFT Wiring](#-current-tft-wiring) and [Button Pins](#-button-pins) sections.

The ESP32 and Windows computer must be connected to the **same Wi-Fi network**.

### Software

Install:

* **Arduino IDE** or **PlatformIO**
* **Python 3**
* Required Arduino libraries:

  * Adafruit GFX Library
  * Adafruit ST7789
  * ArduinoJson
  * TJpg_Decoder

Python packages:

```powershell
pip install pycaw comtypes
```

The Windows Runtime package used by the bridge should also be installed/configured as required by the Python bridge.

---

## 2. Clone / Open the Project

Open the project directory:

```text
Music Station/
```

The project should contain:

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

---

## 3. Configure the Wi-Fi

Open the ESP32 code:

```text
ESP32/music_player/music_player.ino
```

Find:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Replace them with the Wi-Fi network used by both the computer and ESP32:

```cpp
const char* WIFI_SSID = "your_wifi";
const char* WIFI_PASSWORD = "your_password";
```

---

## 4. Find the Computer's Local IP Address

The Python bridge runs on the Windows computer.

Open PowerShell:

```powershell
ipconfig
```

Find the computer's local IPv4 address, for example:

```text
IPv4 Address . . . . . . . . . . : 192.168.0.122
```

---

## 5. Configure the ESP32 Bridge URL

In:

```text
ESP32/music_player/music_player.ino
```

update the computer IP address in the bridge URLs.

For example:

```cpp
const char* MUSIC_BRIDGE_URL =
  "http://192.168.0.122:8080/api/status";

const char* ALBUM_ART_URL =
  "http://192.168.0.122:8080/album_art.jpg";

const char* VOLUME_UP_URL =
  "http://192.168.0.122:8080/api/volume/up";

const char* VOLUME_DOWN_URL =
  "http://192.168.0.122:8080/api/volume/down";

const char* PLAY_PAUSE_URL =
  "http://192.168.0.122:8080/api/playpause";

const char* NEXT_URL =
  "http://192.168.0.122:8080/api/next";

const char* PREVIOUS_URL =
  "http://192.168.0.122:8080/api/previous";
```

Replace `192.168.0.122` with the actual IP address of the computer running the Python bridge.

> **Important:** The ESP32 and computer must be on the same local network.

---

## 6. Install the Python Dependencies

Open PowerShell in the `Bridge` directory:

```powershell
cd "C:\Users\tinke\Documents\PlatformIO\Projects\Music Station\Bridge"
```

Install the required packages:

```powershell
pip install pycaw comtypes
```

If the bridge uses additional packages, install them according to the imports at the top of `music_bridge.py`.

---

## 7. Start the Python Music Bridge

From the `Bridge` directory:

```powershell
py music_bridge.py
```

The bridge should start its HTTP server on:

```text
http://0.0.0.0:8080
```

The bridge should detect the currently active Windows media session.

For example, the terminal may show information similar to:

```text
Title : Song Name
Artist: Artist Name
Album : Album Name
Position: 17 / Duration: 193
Volume: 14 %
Playback: True
```

The bridge also generates:

```text
album_art.jpg
```

when album artwork is available.

---

## 8. Test the Bridge

Before starting the ESP32, test the bridge from the Windows computer.

Open a browser and visit:

```text
http://localhost:8080/api/status
```

You should receive JSON similar to:

```json
{
  "title": "Song Name",
  "artist": "Artist Name",
  "album": "Album Name",
  "playing": true,
  "position": 17,
  "duration": 193,
  "volume": 14
}
```

Then test the album artwork:

```text
http://localhost:8080/album_art.jpg
```

The current album artwork should be displayed.

---

## 9. Upload the ESP32 Firmware

Connect the ESP32-S3 to the computer using USB Type-C.

Open:

```text
ESP32/music_player/music_player.ino
```

Select the correct ESP32-S3 board and COM port in Arduino IDE, or configure the board in PlatformIO.

Compile the project.

If compilation succeeds, upload the firmware to the ESP32-S3.

After uploading, open the Serial Monitor.

You should see messages showing:

```text
WiFi connecting...
WiFi connected.
IP address: ...
```

The ESP32 will then begin requesting information from the Python bridge.

---

## 10. Start the System

The normal startup order is:

### ① Start the Python bridge

```powershell
py music_bridge.py
```

### ② Start music playback

Open YouTube Music or another supported Windows media application.

### ③ Power / reset the ESP32

The ESP32 connects to Wi-Fi and starts communicating with the bridge.

### ④ Check the display

The ST7789 should display:

* Current song
* Artist
* Album
* Album artwork
* Playback position
* Duration
* Playback controls

### ⑤ Test the buttons

Use the physical buttons to test:

```text
Play / Pause
Previous
Next
Volume +
Volume -
```

Volume changes are applied to the **Windows computer's system volume**.

---

## 🔧 Troubleshooting

### ESP32 cannot connect to Wi-Fi

Check:

1. Wi-Fi SSID
2. Wi-Fi password
3. ESP32 is within Wi-Fi range
4. The computer and ESP32 are connected to the same network

---

### ESP32 connects to Wi-Fi but cannot get music information

Check the computer's IP address:

```powershell
ipconfig
```

Then make sure the IP in `music_player.ino` matches the computer running the Python bridge.

Also make sure the Python bridge is running:

```powershell
py music_bridge.py
```

---

### `/api/status` does not work

On the computer, open:

```text
http://localhost:8080/api/status
```

If this doesn't work, the Python bridge is not running correctly.

---

### Album artwork does not appear

First test:

```text
http://localhost:8080/album_art.jpg
```

If the artwork works in the browser but not on the ESP32, check the ESP32 Serial Monitor for:

```text
Album art HTTP error
Album art size
Album art downloaded
Album art: decoding
Album art: displayed
```

---

### Buttons do not work

Check:

* Button wiring
* GPIO numbers
* GND connection
* `INPUT_PULLUP` configuration

The buttons should connect the GPIO pin to **GND when pressed**.

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

| Control      | Function                        |
| ------------ | ------------------------------- |
| Play / Pause | Toggle playback                 |
| Previous     | Previous track                  |
| Next         | Next track                      |
| Volume Up    | Increase computer system volume |
| Volume Down  | Decrease computer system volume |

---

## 🔌 Current TFT Wiring

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

* iPhone music control
* Better animations
* Improved album-art scaling
* Touch controls
* Rotary encoder navigation
* E-paper version
* More polished music-player interface
* Multiple music-service support


