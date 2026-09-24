import asyncio
import json
import threading

from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from winrt.windows.media.control import (
    GlobalSystemMediaTransportControlsSessionManager
)


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

last_title = None
last_artist = None


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
                "playing": current_playing
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

                print("Artwork HTTP error:", e)

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


    manager = (
        await GlobalSystemMediaTransportControlsSessionManager
        .request_async()
    )

    session = manager.get_current_session()

    if session is None:

        current_playing = False

        return


    media = await session.try_get_media_properties_async()

    title = media.title
    artist = media.artist
    album = media.album_title


    # --------------------------------------------------------
    # Playback state
    # --------------------------------------------------------

    playback = session.get_playback_info()

    # Windows Media Session:
    # 4 = Playing

    current_playing = (
        playback.playback_status == 4
    )

    # --------------------------------------------------------
    # Playback position and duration
    # --------------------------------------------------------

    try:

        timeline = session.get_timeline_properties()

        current_position = timeline.position.total_seconds()


    except Exception as e:

        print("Timeline error:", e)
    # --------------------------------------------------------
    # Update current metadata
    # --------------------------------------------------------

    current_title = title
    current_artist = artist
    current_album = album


    # --------------------------------------------------------
    # Check if song changed
    # --------------------------------------------------------

    if title != last_title or artist != last_artist:

        print()
        print("==============================")
        print("       SONG CHANGED")
        print("==============================")

        print("Title :", title)
        print("Artist:", artist)
        print("Album :", album)

        print(
            "State :",
            playback.playback_status
        )


        # ----------------------------------------------------
        # Update album artwork
        # ----------------------------------------------------

        thumbnail = media.thumbnail

        if thumbnail is not None:

            try:

                stream = await thumbnail.open_read_async()

                size = stream.size

                data = bytearray(size)

                await stream.read_async(
                    data,
                    size,
                    0
                )

                ARTWORK_FILE.write_bytes(data)

                print("Artwork: UPDATED")

            except Exception as e:

                print(
                    "Artwork error:",
                    e
                )

        else:

            print(
                "Artwork: NOT AVAILABLE"
            )


        print("==============================")

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

    print("Music Bridge started.")
    print("Waiting for music...")
    print()
    print("Press Ctrl+C to stop.")
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