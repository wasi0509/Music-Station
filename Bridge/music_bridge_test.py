import asyncio
import json
import threading
import time
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from winrt.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager
)

from pycaw.pycaw import AudioUtilities


# ============================================================
# SETTINGS
# ============================================================

ARTWORK_FILE = Path("album_art.jpg")

HTTP_PORT = 8080


# ============================================================
# CURRENT MUSIC STATE
# ============================================================

current_title = ""
current_artist = ""
current_album = ""
current_playing = False

current_position = 0.0
current_duration = 0.0

current_volume = 0

last_title = None
last_artist = None

last_timeline_position = 0.0
last_timeline_update = None


# ============================================================
# WINDOWS VOLUME
# ============================================================

def get_system_volume():

    try:

        device = AudioUtilities.GetSpeakers()

        volume = device.EndpointVolume

        value = volume.GetMasterVolumeLevelScalar()

        return round(value * 100)

    except Exception as e:

        print("Volume read error:", e)

        return current_volume


def change_system_volume(amount):

    global current_volume

    try:

        device = AudioUtilities.GetSpeakers()

        volume = device.EndpointVolume

        current = volume.GetMasterVolumeLevelScalar()

        new_volume = current + (amount / 100.0)

        new_volume = max(
            0.0,
            min(1.0, new_volume)
        )

        volume.SetMasterVolumeLevelScalar(
            new_volume,
            None
        )

        current_volume = round(
            new_volume * 100
        )

        print(
            "System volume:",
            current_volume,
            "%"
        )

        return current_volume

    except Exception as e:

        print("Volume change error:", e)

        return current_volume


# ============================================================
# MUSIC COMMANDS
# ============================================================

async def send_music_command(command):

    manager = (
        await GlobalSystemMediaTransportControlsSessionManager
        .request_async()
    )

    session = manager.get_current_session()

    if session is None:

        print("No active music session.")

        return


    try:

        if command == "playpause":

            await session.try_toggle_play_pause_async()

            print("Command: Play/Pause")


        elif command == "next":

            await session.try_skip_next_async()

            print("Command: Next")


        elif command == "previous":

            await session.try_skip_previous_async()

            print("Command: Previous")


    except Exception as e:

        print(
            "Music command error:",
            e
        )

# ============================================================
# ALBUM ARTWORK
# ============================================================

def update_album_artwork(media):

    async def download_artwork():

        try:

            thumbnail = media.thumbnail

            if thumbnail is None:

                print("Artwork: NOT AVAILABLE")
                return


            print("Artwork: downloading...")


            # Use the same code that worked in the
            # previous version.

            stream = await thumbnail.open_read_async()

            size = stream.size

            print(
                "Artwork size:",
                size,
                "bytes"
            )


            if size <= 0:

                print("Artwork: EMPTY")
                return


            data = bytearray(size)

            await stream.read_async(
                data,
                size,
                0
            )


            ARTWORK_FILE.write_bytes(data)

            print(
                "Artwork: UPDATED"
            )


        except Exception as e:

            print(
                "Artwork error:",
                repr(e)
            )


    def run():

        try:

            asyncio.run(
                download_artwork()
            )

        except Exception as e:

            print(
                "Artwork thread error:",
                repr(e)
            )


    artwork_thread = threading.Thread(
        target=run,
        daemon=True
    )

    artwork_thread.start()

# ============================================================
# HTTP SERVER
# ============================================================

class MusicRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):

        # ----------------------------------------------------
        # /api/status
        # ----------------------------------------------------

        if self.path == "/api/status":

            response = {
                "title": current_title,
                "artist": current_artist,
                "album": current_album,
                "playing": current_playing,
                "position": int(current_position),
                "duration": int(current_duration),
                "volume": int(current_volume)
            }

            data = json.dumps(
                response,
                ensure_ascii=False
            ).encode("utf-8")


            self.send_response(200)

            self.send_header(
                "Content-Type",
                "application/json; charset=utf-8"
            )

            self.send_header(
                "Content-Length",
                str(len(data))
            )

            self.end_headers()

            self.wfile.write(data)

            return


        # ----------------------------------------------------
        # /album_art.jpg
        # ----------------------------------------------------

        if self.path == "/album_art.jpg":

            if not ARTWORK_FILE.exists():

                self.send_response(404)

                self.end_headers()

                return


            try:

                data = ARTWORK_FILE.read_bytes()

                self.send_response(200)

                self.send_header(
                    "Content-Type",
                    "image/jpeg"
                )

                self.send_header(
                    "Content-Length",
                    str(len(data))
                )

                self.end_headers()

                self.wfile.write(data)


            except Exception as e:

                print(
                    "Artwork HTTP error:",
                    e
                )

            return


        # ----------------------------------------------------
        # /api/playpause
        # ----------------------------------------------------

        if self.path == "/api/playpause":

            asyncio.run(
                send_music_command("playpause")
            )

            self.send_response(200)

            self.end_headers()

            return


        # ----------------------------------------------------
        # /api/next
        # ----------------------------------------------------

        if self.path == "/api/next":

            asyncio.run(
                send_music_command("next")
            )

            self.send_response(200)

            self.end_headers()

            return


        # ----------------------------------------------------
        # /api/previous
        # ----------------------------------------------------

        if self.path == "/api/previous":

            asyncio.run(
                send_music_command("previous")
            )

            self.send_response(200)

            self.end_headers()

            return


        # ----------------------------------------------------
        # /api/volume/up
        # ----------------------------------------------------

        if self.path == "/api/volume/up":

            change_system_volume(5)

            self.send_response(200)

            self.end_headers()

            return


        # ----------------------------------------------------
        # /api/volume/down
        # ----------------------------------------------------

        if self.path == "/api/volume/down":

            change_system_volume(-5)

            self.send_response(200)

            self.end_headers()

            return


        # ----------------------------------------------------
        # Unknown URL
        # ----------------------------------------------------

        self.send_response(404)

        self.end_headers()


    # --------------------------------------------------------
    # Hide HTTP request logging
    # --------------------------------------------------------

    def log_message(self, format, *args):

        return


# ============================================================
# START HTTP SERVER
# ============================================================

def start_http_server():

    server = ThreadingHTTPServer(
        ("0.0.0.0", HTTP_PORT),
        MusicRequestHandler
    )

    print(
        f"HTTP server running on port {HTTP_PORT}"
    )

    print(
        f"Status: http://localhost:{HTTP_PORT}/api/status"
    )

    print(
        f"Artwork: http://localhost:{HTTP_PORT}/album_art.jpg"
    )

    print()

    server.serve_forever()


# ============================================================
# MUSIC CHECK
# ============================================================

async def check_music():

    global last_title
    global last_artist

    global current_title
    global current_artist
    global current_album
    global current_playing
    global current_position
    global current_duration
    global current_volume

    global last_timeline_position
    global last_timeline_update


    manager = (
        await GlobalSystemMediaTransportControlsSessionManager
        .request_async()
    )

    session = manager.get_current_session()


    if session is None:

        current_playing = False

        current_volume = get_system_volume()

        return


    # ========================================================
    # MEDIA INFORMATION
    # ========================================================

    media = await session.try_get_media_properties_async()

    title = media.title
    artist = media.artist
    album = media.album_title


    # ========================================================
    # PLAYBACK STATE
    # ========================================================

    playback = session.get_playback_info()

    # Windows Media Session:
    # 4 = Playing

    current_playing = (
        playback.playback_status == 4
    )


    # ========================================================
    # SYSTEM VOLUME
    # ========================================================

    current_volume = get_system_volume()


    # ========================================================
    # TIMELINE
    # ========================================================

    try:

        timeline = session.get_timeline_properties()

        windows_position = (
            timeline.position.total_seconds()
        )

        current_duration = (
            timeline.end_time.total_seconds()
        )

        now = time.monotonic()


        # ----------------------------------------------------
        # First reading
        # ----------------------------------------------------

        if last_timeline_update is None:

            current_position = windows_position

            last_timeline_position = (
                windows_position
            )

            last_timeline_update = now


        # ----------------------------------------------------
        # Windows gave us a newer position
        # ----------------------------------------------------

        elif abs(
            windows_position
            - last_timeline_position
        ) > 0.5:

            current_position = windows_position

            last_timeline_position = (
                windows_position
            )

            last_timeline_update = now


        # ----------------------------------------------------
        # Windows position has not changed
        # ----------------------------------------------------

        elif current_playing:

            current_position = (
                last_timeline_position
                + (
                    now
                    - last_timeline_update
                )
            )


        # ----------------------------------------------------
        # Don't exceed duration
        # ----------------------------------------------------

        if current_duration > 0:

            current_position = min(
                current_position,
                current_duration
            )


        # ----------------------------------------------------
        # Debug
        # ----------------------------------------------------

        print(
            time.strftime("%H:%M:%S"),
            "| Position:",
            int(current_position),
            "/ Duration:",
            int(current_duration),
            "| Volume:",
            current_volume,
            "%",
            "| Playback:",
            current_playing
        )


    except Exception as e:

        print(
            "Timeline error:",
            e
        )


    # ========================================================
    # UPDATE CURRENT METADATA
    # ========================================================

    current_title = title
    current_artist = artist
    current_album = album


    # ========================================================
    # CHECK IF SONG CHANGED
    # ========================================================

    if (
        title != last_title
        or artist != last_artist
    ):

        print()

        print(
            "=============================="
        )

        print(
            "       SONG PLAYING NOW       "
        )

        print(
            "=============================="
        )

        print(
            "Title :",
            title
        )

        print(
            "Artist:",
            artist
        )

        print(
            "Album :",
            album
        )

        print(
            "State :",
            playback.playback_status
        )

        # ----------------------------------------------------
        # Album artwork
        # ----------------------------------------------------
        print(
            "Artwork: starting background download..."
        )

        update_album_artwork(media)


        print(
            "=============================="
        )


        last_title = title
        last_artist = artist

# ============================================================
# MAIN
# ============================================================

async def main():

    # --------------------------------------------------------
    # Start HTTP server in background
    # --------------------------------------------------------

    server_thread = threading.Thread(
        target=start_http_server,
        daemon=True
    )

    server_thread.start()


    # --------------------------------------------------------
    # Music monitoring
    # --------------------------------------------------------

    print(
        "Music Bridge started."
    )

    print(
        "Waiting for music..."
    )

    print()

    print(
        "Press Ctrl+C to stop."
    )

    print()


    while True:

        try:

            await check_music()


        except Exception as e:

            print(
                "Music error:",
                e
            )


        await asyncio.sleep(1)


# ============================================================
# START
# ============================================================

asyncio.run(main())