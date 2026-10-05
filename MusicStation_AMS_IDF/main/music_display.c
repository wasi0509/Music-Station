#include "music_display.h"
#include "st7789.h"
#include "ble_ams.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ============================================================
// DISPLAY POSITIONS
// ============================================================
// Album art
#define ART_X           30
#define ART_Y           15
#define ART_W           160
#define ART_H           160

// Title
#define TITLE_Y         180

// Artist / album
#define ARTIST_Y        224
#define ALBUM_Y         240

// Progress
#define PROGRESS_Y      260

// Timeline
#define TIME_Y          274

// Bottom
#define STATE_Y         296

#define LEFT_X          5


// ============================================================
// State
// ============================================================

static float display_elapsed = 0.0f;

static int64_t last_update_ms = 0;

static char last_title[256] = "";
static char last_artist[256] = "";
static char last_album[256] = "";

static int last_playback_state = -1;


// ============================================================
// Draw title in up to TWO lines
// ============================================================

static void draw_title(
    uint16_t x,
    uint16_t y,
    const char *text
)
{
    if (text == NULL || text[0] == '\0') {
        return;
    }

    /*
     * Title uses scale 1.
     *
     * Each character is approximately 6 pixels wide.
     * The screen is 240 pixels wide.
     *
     * Starting at x=5 gives us about:
     *
     * (240 - 5) / 6 = 39 characters
     *
     * per line.
     *
     * We wrap ONLY at spaces.
     */

    const int MAX_CHARS = 38;
    const int MAX_LINES = 3;

    char line[64];

    size_t text_pos = 0;

    for (int line_number = 0;
         line_number < MAX_LINES;
         line_number++) {

        if (text[text_pos] == '\0') {
            break;
        }

        size_t remaining =
            strlen(&text[text_pos]);

        size_t line_length;

        // ----------------------------------------------------
        // Short enough to fit on this line
        // ----------------------------------------------------

        if (remaining <= MAX_CHARS) {

            line_length = remaining;

        } else {

            // ------------------------------------------------
            // Start at maximum width
            // ------------------------------------------------

            line_length = MAX_CHARS;

            // ------------------------------------------------
            // Move backwards until we find a space
            // ------------------------------------------------

            while (
                line_length > 0 &&
                text[text_pos + line_length] != ' '
            ) {
                line_length--;
            }

            // ------------------------------------------------
            // If no space was found, hard break
            // ------------------------------------------------

            if (line_length == 0) {
                line_length = MAX_CHARS;
            }
        }

        // ----------------------------------------------------
        // Copy this line
        // ----------------------------------------------------

        memset(line, 0, sizeof(line));

        if (line_length >= sizeof(line)) {
            line_length = sizeof(line) - 1;
        }

        memcpy(
            line,
            &text[text_pos],
            line_length
        );

        line[line_length] = '\0';

        // ----------------------------------------------------
        // Draw line
        // ----------------------------------------------------

        st7789_draw_text(
            x,
            y + line_number * 12,
            line,
            0xFFFF,
            1
        );

        // ----------------------------------------------------
        // Move to next part of title
        // ----------------------------------------------------

        text_pos += line_length;

        // Skip spaces
        while (text[text_pos] == ' ') {
            text_pos++;
        }
    }
}


// ============================================================
// Draw one-line small text
// ============================================================

static void draw_small_text(
    uint16_t x,
    uint16_t y,
    const char *text,
    uint16_t color
)
{
    if (text == NULL || text[0] == '\0') {
        return;
    }

    /*
     * Scale 1 = approximately 6 pixels per character.
     * 38 characters fit inside 240 pixels.
     */

    char line[40];

    strncpy(
        line,
        text,
        sizeof(line) - 1
    );

    line[sizeof(line) - 1] = '\0';

    st7789_draw_text(
        x,
        y,
        line,
        color,
        1
    );
}


// ============================================================
// Draw song information
// ============================================================

static void draw_song_info(
    const ble_now_playing_t *song
)
{
    // Clear entire upper section
    st7789_fill_rect(
        0,
        0,
        TFT_WIDTH,
        258,
        0x0000
    );

    // --------------------------------------------------------
    // TITLE - LARGE
    // --------------------------------------------------------

    draw_title(
        LEFT_X,
        TITLE_Y,
        song->title
    );

    // --------------------------------------------------------
    // ARTIST - smaller grey
    // --------------------------------------------------------

    draw_small_text(
        LEFT_X,
        ARTIST_Y,
        song->artist,
        0xC618
    );

    // --------------------------------------------------------
    // ALBUM - smaller grey
    // --------------------------------------------------------

    draw_small_text(
        LEFT_X,
        ALBUM_Y,
        song->album,
        0xC618
    );
}


// ============================================================
// Draw progress + time
// ============================================================

static void draw_progress(
    const ble_now_playing_t *song
)
{
    // Clear ONLY the progress bar area.
    // Do NOT clear artist or album.
    st7789_fill_rect(
        0,
        258,
        TFT_WIDTH,
        12,
        0x0000
    );

    // --------------------------------------------------------
    // Progress bar
    // --------------------------------------------------------

    if (song->duration > 0) {

        uint16_t bar_width = 220;
        uint16_t bar_height = 6;

        float progress =
            display_elapsed / song->duration;

        if (progress < 0.0f)
            progress = 0.0f;

        if (progress > 1.0f)
            progress = 1.0f;

        uint16_t filled =
            (uint16_t)(bar_width * progress);

        // Empty bar
        st7789_fill_rect(
            LEFT_X,
            PROGRESS_Y,
            bar_width,
            bar_height,
            0x7BEF
        );

        // Filled bar
        if (filled > 0) {

            st7789_fill_rect(
                LEFT_X,
                PROGRESS_Y,
                filled,
                bar_height,
                0xFFFF
            );
        }
    }

    // --------------------------------------------------------
    // Timeline - SMALL
    // --------------------------------------------------------

    char time_text[32];

    int elapsed_seconds =
        (int)display_elapsed;

    int duration_seconds =
        (int)song->duration;

    snprintf(
        time_text,
        sizeof(time_text),
        "%d / %d",
        elapsed_seconds,
        duration_seconds
    );

    // Clear old timeline
    st7789_fill_rect(
        160,
        TIME_Y - 2,
        80,
        12,
        0x0000
    );

    // Draw new timeline on the right
    st7789_draw_text(
        165,
        TIME_Y,
        time_text,
        0xC618,
        1
    );
}


// ============================================================
// Draw PLAYING / PAUSED
// ============================================================

static void draw_playback_state(
    const ble_now_playing_t *song
)
{
    // Clear old state
    st7789_fill_rect(
        0,
        STATE_Y - 2,
        180,
        25,
        0x0000
    );

    if (song->playback_state == 1) {

        st7789_draw_text(
            LEFT_X,
            STATE_Y,
            "PLAYING",
            0x07E0,
            2
        );

    } else {

        st7789_draw_text(
            LEFT_X,
            STATE_Y,
            "PAUSED",
            0xF800,
            2
        );
    }
}


// ============================================================
// Main display update
// ============================================================

void music_display_update(void)
{
    const ble_now_playing_t *song =
        ble_get_now_playing();

    int64_t now_ms =
        esp_timer_get_time() / 1000;


    // ========================================================
    // Detect song change
    // ========================================================

    bool new_song = false;

    if (strcmp(last_title, song->title) != 0 ||
        strcmp(last_artist, song->artist) != 0 ||
        strcmp(last_album, song->album) != 0) {

        new_song = true;
    }


    // ========================================================
    // New song
    // ========================================================

    if (new_song) {

        strncpy(
            last_title,
            song->title,
            sizeof(last_title) - 1
        );

        last_title[
            sizeof(last_title) - 1
        ] = '\0';


        strncpy(
            last_artist,
            song->artist,
            sizeof(last_artist) - 1
        );

        last_artist[
            sizeof(last_artist) - 1
        ] = '\0';


        strncpy(
            last_album,
            song->album,
            sizeof(last_album) - 1
        );

        last_album[
            sizeof(last_album) - 1
        ] = '\0';


        display_elapsed =
            song->elapsed;

        last_update_ms =
            now_ms;


        draw_song_info(song);

        draw_playback_state(song);

        last_playback_state =
            song->playback_state;
    }


    // ========================================================
    // First update
    // ========================================================

    if (last_update_ms == 0) {

        display_elapsed =
            song->elapsed;

        last_update_ms =
            now_ms;

        draw_song_info(song);

        draw_playback_state(song);

        last_playback_state =
            song->playback_state;
    }


    // ========================================================
    // Local time advancement
    // ========================================================

    int64_t ams_update_ms =
        ble_get_now_playing_update_ms();

    if (ams_update_ms > 0) {

        float time_since_ams =
            (now_ms - ams_update_ms) / 1000.0f;

        if (time_since_ams < 0.0f) {
            time_since_ams = 0.0f;
        }

        if (song->playback_state == 1) {

            display_elapsed =
                song->elapsed +
                time_since_ams * song->rate;

        } else {

            display_elapsed =
                song->elapsed;
        }
    }


    if (song->duration > 0 &&
        display_elapsed > song->duration) {

        display_elapsed =
            song->duration;
    }

    if (display_elapsed < 0) {
        display_elapsed = 0;
    }


    // ========================================================
    // Update progress/time
    // ========================================================

    draw_progress(song);


    // ========================================================
    // Playback state changed?
    // ========================================================

    if (song->playback_state !=
        last_playback_state) {

        draw_playback_state(song);

        last_playback_state =
            song->playback_state;
    }


    // ========================================================
    // Debug
    // ========================================================

    ESP_LOGI(
        "DISPLAY",
        "Display elapsed: %.1f / %.1f",
        display_elapsed,
        song->duration
    );
}