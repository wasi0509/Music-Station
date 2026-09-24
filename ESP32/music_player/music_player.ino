#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <TJpg_Decoder.h>


// =====================================================
// Wi-Fi
// =====================================================

const char* WIFI_SSID = "ivy";
const char* WIFI_PASSWORD = "ringo496162";

WebServer server(80);

// =====================================================
// MUSIC BRIDGE
// =====================================================

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

// =====================================================
// ST7789 DISPLAY PINS
// =====================================================
// PLACEHOLDERS FOR NOW
// We will replace these with your board's actual
// GPIO pin mapping once we have the board pinout.

#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8
#define TFT_SCLK  12
#define TFT_MOSI  11
#define TFT_BL     7

#define BUTTON_PLAY       1
#define BUTTON_PREVIOUS   2
#define BUTTON_NEXT       4
#define BUTTON_VOLUME_UP  5
#define BUTTON_VOLUME_DOWN 6


Adafruit_ST7789 tft = Adafruit_ST7789(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

// =====================================================
// JPEG DECODER
// =====================================================

bool jpegOutput(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t* bitmap
) {

  // Don't draw outside the screen
  if (y >= tft.height()) {
    return 0;
  }

  // Draw decoded JPEG block
  tft.drawRGBBitmap(
    x,
    y,
    bitmap,
    w,
    h
  );

  return 1;
}

// =====================================================
// MUSIC DATA
// =====================================================

struct Song {
  String title;
  String artist;
  String album;
};


Song playlist[] = {
  {"Blinding Lights", "The Weeknd", "After Hours"},
  {"Save Your Tears", "The Weeknd", "After Hours"},
  {"Starboy", "The Weeknd", "Starboy"},
  {"Die For You", "The Weeknd", "Starboy"}
};


const int SONG_COUNT =
  sizeof(playlist) / sizeof(playlist[0]);


int currentSong = 0;

bool isPlaying = false;

int volume = 75;

int currentPosition = 0;
int currentDuration = 0;
 
// =====================================================
// GET CURRENT SONG
// =====================================================

Song getCurrentSong() {
  return playlist[currentSong];
}
// =====================================================
// UPDATE MUSIC FROM LAPTOP
// =====================================================

void updateMusicFromLaptop() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  HTTPClient http;

  http.begin(MUSIC_BRIDGE_URL);

  int httpCode = http.GET();

  if (httpCode == 200) {

    String payload = http.getString();

    Serial.println();
    Serial.println("Music bridge response:");
    Serial.println(payload);

    JsonDocument doc;

    DeserializationError error =
      deserializeJson(doc, payload);

    if (!error) {

      // =================================================
      // GET DATA FROM PYTHON BRIDGE
      // =================================================

      String newTitle =
        doc["title"].as<String>();

      String newArtist =
        doc["artist"].as<String>();

      String newAlbum =
        doc["album"].as<String>();

      bool newPlaying =
        doc["playing"].as<bool>();

      int newPosition =
        doc["position"].as<int>();

      int newDuration =
        doc["duration"].as<int>();

      int newVolume =
        doc["volume"].as<int>();


      // =================================================
      // CHECK IF SONG CHANGED
      // =================================================

      bool songChanged =
        newTitle != playlist[currentSong].title;


      // =================================================
      // UPDATE MUSIC INFORMATION FIRST
      // =================================================

      if (newTitle.length() > 0) {

        playlist[currentSong].title =
          newTitle;
      }

      if (newArtist.length() > 0) {

        playlist[currentSong].artist =
          newArtist;
      }

      if (newAlbum.length() > 0) {

        playlist[currentSong].album =
          newAlbum;
      }

      isPlaying =
        newPlaying;

      currentPosition =
        newPosition;

      currentDuration =
        newDuration;

      volume =
        newVolume;


      // =================================================
      // SONG CHANGED
      // =================================================

      if (songChanged) {

        Serial.println();
        Serial.println(
          "=============================="
        );

        Serial.println(
          "NEW SONG DETECTED"
        );

        Serial.println(
          "=============================="
        );

        Serial.print(
          "Title: "
        );

        Serial.println(
          newTitle
        );


        // -----------------------------------------------
        // Draw the new song information
        // -----------------------------------------------

        showNowPlaying();


        // -----------------------------------------------
        // Download the new album artwork
        // -----------------------------------------------

        downloadAlbumArt();


        Serial.println(
          "=============================="
        );
      }


      // =================================================
      // NORMAL DISPLAY UPDATE
      // =================================================

      else {

        showNowPlaying();
      }


      Serial.println(
        "ESP32 music information updated."
      );
    }

    else {

      Serial.print(
        "JSON error: "
      );

      Serial.println(
        error.c_str()
      );
    }
  }

  else {

    Serial.print(
      "Music bridge HTTP error: "
    );

    Serial.println(
      httpCode
    );
  }

  http.end();
}

// =====================================================
// SEND COMMAND TO MUSIC BRIDGE
// =====================================================

void sendMusicCommand(const char* url) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi not connected.");
    return;
  }

  HTTPClient http;

  http.begin(url);

  int httpCode = http.GET();

  Serial.print("Command: ");
  Serial.print(url);
  Serial.print(" | HTTP: ");
  Serial.println(httpCode);

  http.end();
}

// =====================================================
// DRAW PLAYBACK CONTROLS
// =====================================================

void drawControls() {

  tft.setTextColor(ST77XX_WHITE);

  tft.setTextSize(2);

  // Previous
  tft.setCursor(45, 180);
  tft.print("|<");

  // Play / Pause
  tft.setCursor(150, 180);

  if (isPlaying) {
    tft.print("||");
  }
  else {
    tft.print(">");
  }

  // Next
  tft.setCursor(250, 180);
  tft.print(">|");
}

// =====================================================
// DRAW SONG PROGRESS BAR
// =====================================================

void drawProgressBar() {

  int barX = 40;
  int barY = 150;

  int barWidth = 280;
  int barHeight = 6;


  // Empty progress bar

  tft.drawRect(
    barX,
    barY,
    barWidth,
    barHeight,
    ST77XX_WHITE
  );


  // Calculate progress

  int filledWidth = 0;

  if (currentDuration > 0) {

    filledWidth =
      (barWidth * currentPosition)
      / currentDuration;

  }


  // Prevent going outside the bar

  if (filledWidth < 0) {
    filledWidth = 0;
  }

  if (filledWidth > barWidth) {
    filledWidth = barWidth;
  }


  // Filled progress

  if (filledWidth > 0) {

    tft.fillRect(
      barX,
      barY,
      filledWidth,
      barHeight,
      ST77XX_WHITE
    );

  }


  // Current time

  tft.setTextSize(1);

  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(
    barX,
    barY + 12
  );

  tft.print(
    formatTime(currentPosition)
  );


  // Duration

  String durationText =
    formatTime(currentDuration);

  tft.setCursor(
    barX + barWidth - (durationText.length() * 6),
    barY + 12
  );

  tft.print(
    durationText
  );
}

// =====================================================
// DOWNLOAD AND DISPLAY ALBUM ART
// =====================================================

bool downloadAlbumArt() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "Album art: Wi-Fi not connected."
    );

    return false;
  }


  HTTPClient http;

  Serial.println(
    "Album art: downloading..."
  );


  http.begin(ALBUM_ART_URL);

  http.setTimeout(5000);


  int httpCode = http.GET();


  if (httpCode != HTTP_CODE_OK) {

    Serial.print(
      "Album art HTTP error: "
    );

    Serial.println(
      httpCode
    );

    http.end();

    return false;
  }


  int imageSize =
    http.getSize();


  Serial.print(
    "Album art size: "
  );

  Serial.print(
    imageSize
  );

  Serial.println(
    " bytes"
  );


  if (imageSize <= 0) {

    Serial.println(
      "Album art: invalid size."
    );

    http.end();

    return false;
  }


  // Prevent unexpectedly huge files
  if (imageSize > 300000) {

    Serial.println(
      "Album art: file too large."
    );

    http.end();

    return false;
  }


  // Allocate memory
  uint8_t* imageData =
    (uint8_t*)malloc(imageSize);


  if (imageData == nullptr) {

    Serial.println(
      "Album art: memory allocation failed."
    );

    http.end();

    return false;
  }


  WiFiClient* stream =
    http.getStreamPtr();


  int totalRead = 0;


  while (
    totalRead < imageSize
  ) {

    int available =
      stream->available();


    if (available > 0) {

      int remaining =
        imageSize - totalRead;

      int toRead =
        min(
          available,
          remaining
        );


      int bytesRead =
        stream->readBytes(
          imageData + totalRead,
          toRead
        );


      if (bytesRead <= 0) {
        break;
      }


      totalRead += bytesRead;
    }

    else {

      delay(1);
    }
  }


  http.end();


  Serial.print(
    "Album art downloaded: "
  );

  Serial.print(
    totalRead
  );

  Serial.println(
    " bytes"
  );


  if (totalRead != imageSize) {

    Serial.println(
      "Album art: incomplete download."
    );

    free(imageData);

    return false;
  }


  // -----------------------------------------------------
  // Decode JPEG
  // -----------------------------------------------------

  Serial.println(
    "Album art: decoding..."
  );


  TJpgDec.setJpgScale(1);

  TJpgDec.setCallback(
    jpegOutput
  );


  // Album art position
  int artX = 10;
  int artY = 45;


  TJpgDec.drawJpg(
    artX,
    artY,
    imageData,
    imageSize
  );


  free(imageData);


  Serial.println(
    "Album art: displayed."
  );


  return true;
}

void showNowPlaying() {

  Song song = getCurrentSong();

  // ===================================================
  // CLEAR SCREEN
  // ===================================================

  tft.fillScreen(ST77XX_BLACK);


  // ===================================================
  // HEADER
  // ===================================================

  tft.setTextColor(ST77XX_WHITE);

  tft.setTextSize(2);

  tft.setCursor(10, 10);

  tft.print("NOW PLAYING");


  // Playing status

  tft.setTextSize(1);

  tft.setCursor(250, 14);

  if (isPlaying) {
    tft.print("PLAYING");
  }
  else {
    tft.print("PAUSED");
  }

  // ===================================================
  // ALBUM ART
  // ===================================================

  int artX = 10;
  int artY = 45;
  int artSize = 125;


  // Album art background

  tft.fillRect(
    artX,
    artY,
    artSize,
    artSize,
    ST77XX_BLACK
  );


  // Album art border

  tft.drawRect(
    artX,
    artY,
    artSize,
    artSize,
    ST77XX_WHITE
  );

  // ===================================================
  // SONG INFORMATION
  // ===================================================

  tft.setTextColor(ST77XX_WHITE);


  // Song title

  tft.setTextSize(2);

  tft.setCursor(150, 55);

  tft.print(song.title);


  // Artist

  tft.setTextSize(1);

  tft.setCursor(150, 85);

  tft.print(song.artist);


  // Album

  tft.setCursor(150, 105);

  tft.print(song.album);

  // ===================================================
  // SONG PROGRESS
  // ===================================================

  drawProgressBar();

  // ===================================================
  // PLAYBACK CONTROLS
  // ===================================================

  drawControls();

// =====================================================
// SHOW VOLUME POP-UP
// =====================================================

void showVolumePopup() {

  // Draw popup box
  tft.fillRect(
    90,
    95,
    140,
    60,
    ST77XX_BLACK
  );

  tft.drawRect(
    90,
    95,
    140,
    60,
    ST77XX_WHITE
  );

  // Volume text
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  tft.setCursor(115, 110);

  tft.print("VOL ");

  tft.print(volume);

  tft.print("%");

  delay(700);

  // Restore normal screen
  showNowPlaying();
}

// =====================================================
// MUSIC CONTROLS
// =====================================================

void playPause() {

  sendMusicCommand(PLAY_PAUSE_URL);

  Serial.println("Play/Pause command sent.");
}


void nextSong() {

  sendMusicCommand(NEXT_URL);

  Serial.println("Next command sent.");
}


void previousSong() {

  sendMusicCommand(PREVIOUS_URL);

  Serial.println("Previous command sent.");
}


void volumeUp() {

  sendMusicCommand(VOLUME_UP_URL);

  Serial.println("Volume + command sent.");

  showVolumePopup();
}


void volumeDown() {

  sendMusicCommand(VOLUME_DOWN_URL);

  Serial.println("Volume - command sent.");

  showVolumePopup();
}


// =====================================================
// WEB PAGE
// =====================================================

void createWebPage() {

  Song song = getCurrentSong();

  String page = "";

  page += "<html>";

  page += "<head>";

  page += "<meta name='viewport' ";
  page += "content='width=device-width,initial-scale=1'>";

  page += "<title>ESP32 Music Player</title>";

  page += "</head>";

  page += "<body>";

  page += "<h1>ESP32 Music Player</h1>";


  // Song information

  page += "<h2>";
  page += song.title;
  page += "</h2>";

  page += "<p>";
  page += song.artist;
  page += "</p>";

  page += "<p>";
  page += song.album;
  page += "</p>";


  // Controls

  page += "<br>";


  page += "<a href='/previous'>";
  page += "<button>Previous</button>";
  page += "</a>";


  page += " ";


  page += "<a href='/playpause'>";
  page += "<button>Play / Pause</button>";
  page += "</a>";


  page += " ";


  page += "<a href='/next'>";
  page += "<button>Next</button>";
  page += "</a>";


  // Volume

  page += "<br><br>";


  page += "<a href='/volume/down'>";
  page += "<button>Volume -</button>";
  page += "</a>";


  page += " ";


  page += "<a href='/volume/up'>";
  page += "<button>Volume +</button>";
  page += "</a>";


  page += "<p>Volume: ";

  page += volume;

  page += "%</p>";


  page += "</body>";

  page += "</html>";


  server.send(
    200,
    "text/html",
    page
  );
}

// =====================================================
// CHECK PHYSICAL BUTTONS
// =====================================================

void checkButtons() {

  static bool lastPlayState = HIGH;
  static bool lastPreviousState = HIGH;
  static bool lastNextState = HIGH;
  static bool lastVolumeUpState = HIGH;
  static bool lastVolumeDownState = HIGH;

  bool playState =
    digitalRead(BUTTON_PLAY);

  bool previousState =
    digitalRead(BUTTON_PREVIOUS);

  bool nextState =
    digitalRead(BUTTON_NEXT);

  bool volumeUpState =
    digitalRead(BUTTON_VOLUME_UP);

  bool volumeDownState =
    digitalRead(BUTTON_VOLUME_DOWN);


  // Play / Pause

  if (
    lastPlayState == HIGH &&
    playState == LOW
  ) {

    playPause();

    delay(200);
  }


  // Previous

  if (
    lastPreviousState == HIGH &&
    previousState == LOW
  ) {

    previousSong();

    delay(200);
  }


  // Next

  if (
    lastNextState == HIGH &&
    nextState == LOW
  ) {

    nextSong();

    delay(200);
  }


  // Volume +

  if (
    lastVolumeUpState == HIGH &&
    volumeUpState == LOW
  ) {

    volumeUp();

    delay(200);
  }


  // Volume -

  if (
    lastVolumeDownState == HIGH &&
    volumeDownState == LOW
  ) {

    volumeDown();

    delay(200);
  }


  // Remember states

  lastPlayState = playState;

  lastPreviousState = previousState;

  lastNextState = nextState;

  lastVolumeUpState = volumeUpState;

  lastVolumeDownState = volumeDownState;
}

String formatTime(int seconds) {
  int minutes = seconds / 60;
  int secs = seconds % 60;

  char buffer[10];

  sprintf(buffer, "%d:%02d", minutes, secs);

  return String(buffer);
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // ===================================================
  // BUTTONS
  // ===================================================

  pinMode(
    BUTTON_PLAY,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_PREVIOUS,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_NEXT,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_VOLUME_UP,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_VOLUME_DOWN,
    INPUT_PULLUP
  );

  delay(1000);


  Serial.println();

  Serial.println("==============================");

  Serial.println("ESP32 Music Player");

  Serial.println("==============================");


  // ===================================================
  // DISPLAY
  // ===================================================

  pinMode(
    TFT_BL,
    OUTPUT
  );

  digitalWrite(
    TFT_BL,
    HIGH
  );


  SPI.begin(
    TFT_SCLK,
    -1,
    TFT_MOSI,
    TFT_CS
  );


  tft.init(
    240,
    320
  );


  // Landscape
  tft.setRotation(1);


  showNowPlaying();


  // ===================================================
  // WI-FI
  // ===================================================

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  Serial.print(
    "Connecting to Wi-Fi"
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }


  Serial.println();

  Serial.println(
    "Wi-Fi connected!"
  );


  Serial.print(
    "IP address: "
  );


  Serial.println(
    WiFi.localIP()
  );


  // ===================================================
  // WEB SERVER
  // ===================================================

  server.on("/", []() {

    createWebPage();

  });


  server.on("/playpause", []() {

    playPause();

    createWebPage();

  });


  server.on("/next", []() {

    nextSong();

    createWebPage();

  });


  server.on("/previous", []() {

    previousSong();

    createWebPage();

  });


  server.on("/volume/up", []() {

    volumeUp();

    createWebPage();

  });


  server.on("/volume/down", []() {

    volumeDown();

    createWebPage();

  });


  server.begin();


  Serial.println(
    "Web server started."
  );

  // ===================================================
  // TEST ALBUM ART
  // ===================================================

  delay(1000);

  downloadAlbumArt();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  server.handleClient();

  checkButtons();


  // Update music information from laptop
  static unsigned long lastMusicUpdate = 0;

  if (millis() - lastMusicUpdate >= 2000) {

    lastMusicUpdate = millis();

    updateMusicFromLaptop();
  }
}