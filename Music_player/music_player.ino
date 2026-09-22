/*
  ESP32-S3 Music Player
  Step 3 - Music Player Logic

  No hardware display required yet.
*/

#include <WiFi.h>
#include <WebServer.h>

// ========================================
// WIFI
// ========================================

const char* WIFI_SSID = "ivy";
const char* WIFI_PASSWORD = "ringo496162";

WebServer server(80);


// ========================================
// SONG DATA
// ========================================

struct Song {

  String title;
  String artist;
  String album;
};


// Our temporary playlist

Song playlist[] = {

  {
    "Blinding Lights",
    "The Weeknd",
    "After Hours"
  },

  {
    "Save Your Tears",
    "The Weeknd",
    "After Hours"
  },

  {
    "Starboy",
    "The Weeknd",
    "Starboy"
  },

  {
    "Die For You",
    "The Weeknd",
    "Starboy"
  }

};


// Number of songs

const int SONG_COUNT =
  sizeof(playlist) / sizeof(playlist[0]);


// Current song

int currentSong = 0;


// Player state

bool isPlaying = false;


// Volume

int volume = 75;


// ========================================
// GET CURRENT SONG
// ========================================

Song getCurrentSong() {

  return playlist[currentSong];

}


// ========================================
// SHOW NOW PLAYING
// ========================================

void showNowPlaying() {

  Song song = getCurrentSong();

  Serial.println();
  Serial.println("================================");
  Serial.println("          NOW PLAYING");
  Serial.println("================================");

  Serial.print("Song:   ");
  Serial.println(song.title);

  Serial.print("Artist: ");
  Serial.println(song.artist);

  Serial.print("Album:  ");
  Serial.println(song.album);

  Serial.print("Status: ");

  if (isPlaying) {

    Serial.println("PLAYING");

  } else {

    Serial.println("PAUSED");

  }

  Serial.print("Volume: ");
  Serial.print(volume);
  Serial.println("%");

  Serial.print("Track:  ");
  Serial.print(currentSong + 1);
  Serial.print("/");
  Serial.println(SONG_COUNT);

  Serial.println("================================");
}


// ========================================
// PLAY / PAUSE
// ========================================

void playPause() {

  isPlaying = !isPlaying;

  if (isPlaying) {

    Serial.println("▶ PLAY");

  } else {

    Serial.println("Ⅱ PAUSE");

  }

  showNowPlaying();
}


// ========================================
// NEXT SONG
// ========================================

void nextSong() {

  currentSong++;

  if (currentSong >= SONG_COUNT) {

    currentSong = 0;

  }

  isPlaying = true;

  Serial.println("NEXT SONG");

  showNowPlaying();
}


// ========================================
// PREVIOUS SONG
// ========================================

void previousSong() {

  currentSong--;

  if (currentSong < 0) {

    currentSong = SONG_COUNT - 1;

  }

  isPlaying = true;

  Serial.println("PREVIOUS SONG");

  showNowPlaying();
}


// ========================================
// VOLUME UP
// ========================================

void volumeUp() {

  volume += 5;

  if (volume > 100) {

    volume = 100;

  }

  Serial.print("Volume: ");
  Serial.print(volume);
  Serial.println("%");

  showNowPlaying();
}


// ========================================
// VOLUME DOWN
// ========================================

void volumeDown() {

  volume -= 5;

  if (volume < 0) {

    volume = 0;

  }

  Serial.print("Volume: ");
  Serial.print(volume);
  Serial.println("%");

  showNowPlaying();
}


// ========================================
// WEB PAGE
// ========================================

String createWebPage() {

  Song song = getCurrentSong();

  String page = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>ESP32 Music Player</title>

<style>

body {

  background: #111;

  color: white;

  font-family: Arial;

  text-align: center;

  padding-top: 30px;

}

h1 {

  font-size: 28px;

}

.song {

  margin-top: 30px;

}

.title {

  font-size: 26px;

  font-weight: bold;

}

.artist {

  font-size: 20px;

  color: #aaa;

}

.album {

  font-size: 16px;

  color: #777;

}

button {

  font-size: 18px;

  padding: 15px 20px;

  margin: 8px;

  border-radius: 12px;

  border: none;

}

</style>

</head>

<body>

<h1>ESP32 MUSIC PLAYER</h1>

<div class="song">

<div class="title">

)rawliteral";

  page += song.title;

  page += R"rawliteral(

</div>

<div class="artist">

)rawliteral";

  page += song.artist;

  page += R"rawliteral(

</div>

<div class="album">

)rawliteral";

  page += song.album;

  page += R"rawliteral(

</div>

<br>

<div>

Status:

)rawliteral";

  page += isPlaying ? "PLAYING" : "PAUSED";

  page += R"rawliteral(

</div>

<br>

<div>

Volume:

)rawliteral";

  page += String(volume);

  page += "%";

  page += R"rawliteral(

</div>

<br>

<a href="/previous">

<button>◀ Previous</button>

</a>

<a href="/playpause">

<button>▶ / Ⅱ</button>

</a>

<a href="/next">

<button>Next ▶</button>

</a>

<br>

<a href="/volume/down">

<button>Volume −</button>

</a>

<a href="/volume/up">

<button>Volume +</button>

</a>

</div>

</body>

</html>

)rawliteral";

  return page;
}


// ========================================
// WEB ROUTES
// ========================================

void handleRoot() {

  server.send(

    200,

    "text/html",

    createWebPage()

  );

}


void handlePlayPause() {

  playPause();

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);

}


void handleNext() {

  nextSong();

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);

}


void handlePrevious() {

  previousSong();

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);

}


void handleVolumeUp() {

  volumeUp();

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);

}


void handleVolumeDown() {

  volumeDown();

  server.sendHeader(
    "Location",
    "/"
  );

  server.send(303);

}


// ========================================
// SETUP
// ========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 MUSIC PLAYER");

  // Wi-Fi

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print("Connecting to Wi-Fi");

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");

  }

  Serial.println();

  Serial.println("Wi-Fi connected!");

  Serial.print("IP address: ");

  Serial.println(
    WiFi.localIP()
  );


  // Web routes

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/playpause",
    handlePlayPause
  );

  server.on(
    "/next",
    handleNext
  );

  server.on(
    "/previous",
    handlePrevious
  );

  server.on(
    "/volume/up",
    handleVolumeUp
  );

  server.on(
    "/volume/down",
    handleVolumeDown
  );


  server.begin();

  Serial.println(
    "Music server started!"
  );


  // Initial state

  showNowPlaying();

}


// ========================================
// LOOP
// ========================================

void loop() {

  server.handleClient();

}