#include "artwork.h"
#include "jpeg_decoder.h"
#include "st7789.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "esp_system.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "cJSON.h"

#define WIFI_SSID     "ivy"
#define WIFI_PASSWORD "ringo496162"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "ARTWORK";

static EventGroupHandle_t wifi_event_group;

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();

    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Wi-Fi connected! IP address: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}

esp_err_t artwork_init(void)
{
    ESP_LOGI(TAG, "Starting Wi-Fi...");

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG, "Waiting for Wi-Fi...");

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000)
    );

    if (bits & WIFI_CONNECTED_BIT) {

        ESP_LOGI(TAG, "Artwork Wi-Fi ready");

        return ESP_OK;

    } else {

        ESP_LOGE(
            TAG,
            "Failed to connect to Wi-Fi"
        );

        return ESP_FAIL;
    }
}

static void url_encode(
    const char *src,
    char *dst,
    size_t dst_size
)
{
    const char hex[] = "0123456789ABCDEF";

    size_t pos = 0;

    while (*src && pos + 1 < dst_size) {

        unsigned char c = (unsigned char)*src;

        if (isalnum(c) ||
            c == '-' ||
            c == '_' ||
            c == '.' ||
            c == '~') {

            dst[pos++] = c;

        } else {

            if (pos + 3 >= dst_size) {
                break;
            }

            dst[pos++] = '%';
            dst[pos++] = hex[c >> 4];
            dst[pos++] = hex[c & 0x0F];
        }

        src++;
    }

    dst[pos] = '\0';
}

static esp_err_t decode_artwork(
    uint8_t *jpeg_data,
    size_t jpeg_size
)
{
    ESP_LOGI(TAG, "Starting JPEG decode...");

    /*
     * Our iTunes artwork is currently requested at 100x100.
     *
     * RGB565 = 2 bytes per pixel.
     * 100 x 100 x 2 = 20,000 bytes.
     *
     * We allocate a little extra so the decoder has room
     * if the returned image dimensions are slightly different.
     */
    size_t output_buffer_size = 100 * 100 * 2;

    ESP_LOGI(
        TAG,
        "Free heap: %lu",
        (unsigned long)esp_get_free_heap_size()
    );

    ESP_LOGI(
        TAG,
        "Free PSRAM: %lu",
        (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
    );

    ESP_LOGI(
        TAG,
        "Largest PSRAM block: %lu",
        (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)
    );

    uint8_t *output_buffer =
        heap_caps_malloc(
            output_buffer_size,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
        );

    if (output_buffer == NULL) {

        ESP_LOGE(
            TAG,
            "Failed to allocate JPEG output buffer"
        );

        return ESP_ERR_NO_MEM;
    }

    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = jpeg_data,
        .indata_size = jpeg_size,

        .outbuf = output_buffer,
        .outbuf_size = output_buffer_size,

        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,

        .flags = {
            /*
             * ST7789 normally expects the bytes in
             * big-endian RGB565 order.
             */
            .swap_color_bytes = 1,
        }
    };

    esp_jpeg_image_output_t outimg = {0};

    esp_err_t err =
        esp_jpeg_decode(
            &jpeg_cfg,
            &outimg
        );

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "JPEG decode failed: %s",
            esp_err_to_name(err)
        );

        free(output_buffer);

        return err;
    }

    ESP_LOGI(
        TAG,
        "JPEG decoded successfully: %d x %d",
        outimg.width,
        outimg.height
    );

    ESP_LOGI(
        TAG,
        "Decoded pixel data size: %d bytes",
        outimg.width * outimg.height * 2
    );


    uint16_t x = 40 + (160 - outimg.width) / 2;
    uint16_t y = 10 + (160 - outimg.height) / 2;

    st7789_draw_rgb565(
        x,
        y,
        outimg.width,
        outimg.height,
        output_buffer
    );

    ESP_LOGI(
        TAG,
        "Artwork drawn to TFT at %d,%d",
        x,
        y
    );

    free(output_buffer);

    return ESP_OK;
}

esp_err_t artwork_search(
    const char *artist,
    const char *title
)
{
    if (artist == NULL || title == NULL) {
        ESP_LOGE(TAG, "Artist or title is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Searching artwork for:");
    ESP_LOGI(TAG, "  Artist: %s", artist);
    ESP_LOGI(TAG, "  Title:  %s", title);

    char encoded_artist[256];
    char encoded_title[256];
    char url[768];

    url_encode(
        artist,
        encoded_artist,
        sizeof(encoded_artist)
    );

    url_encode(
        title,
        encoded_title,
        sizeof(encoded_title)
    );

    int written = snprintf(
        url,
        sizeof(url),
        "https://itunes.apple.com/search?term=%s%%20%s&entity=song&limit=1",
        encoded_artist,
        encoded_title
    );

    if (written < 0 || written >= sizeof(url)) {
        ESP_LOGE(TAG, "Artwork URL is too long");
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "iTunes URL:");
    ESP_LOGI(TAG, "%s", url);

    char *response = malloc(8192);

    if (response == NULL) {
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .buffer_size = 4096,
        .buffer_size_tx = 2048,
        .keep_alive_enable = false,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to create HTTP client");
        free(response);
        return ESP_FAIL;
    }

    /*
     * Open the HTTP connection manually.
     *
     * We do this instead of esp_http_client_perform()
     * because we want to manually read the response body.
     */
    esp_err_t err = esp_http_client_open(client, 0);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "HTTP open failed: %s",
            esp_err_to_name(err)
        );

        esp_http_client_cleanup(client);
        free(response);

        return err;
    }

    /*
     * Read the HTTP response headers.
     */
    int content_length =
        esp_http_client_fetch_headers(client);

    if (content_length < 0) {
        ESP_LOGE(
            TAG,
            "Failed to fetch HTTP headers"
        );

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(response);

        return ESP_FAIL;
    }

    int status_code =
        esp_http_client_get_status_code(client);

    ESP_LOGI(TAG, "HTTP status: %d", status_code);
    ESP_LOGI(TAG, "HTTP content length: %d", content_length);

    /*
     * Read the response body.
     */
    int total_read = 0;

    while (total_read < 8191) {

        int remaining = 8191 - total_read;

        int read_len = esp_http_client_read(
            client,
            response + total_read,
            remaining
        );

        if (read_len < 0) {

            ESP_LOGE(
                TAG,
                "HTTP read failed"
            );

            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            free(response);

            return ESP_FAIL;
        }

        if (read_len == 0) {
            break;
        }

        total_read += read_len;

        /*
         * If the server gave us a known content length,
         * stop once we have received everything.
         */
        if (content_length > 0 &&
            total_read >= content_length) {
            break;
        }
    }

    response[total_read] = '\0';

    ESP_LOGI(
        TAG,
        "Received %d bytes",
        total_read
    );

    /*
     * Print the JSON temporarily so we can verify
     * that the iTunes response is actually present.
     */
    ESP_LOGI(
        TAG,
        "iTunes response:\n%s",
        response
    );

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total_read == 0) {

        ESP_LOGE(
            TAG,
            "No response received"
        );

        free(response);

        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response);

    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse iTunes JSON");
        free(response);
        return ESP_FAIL;
    }

    cJSON *result_count =
        cJSON_GetObjectItem(root, "resultCount");

    if (result_count != NULL &&
        cJSON_IsNumber(result_count)) {

        ESP_LOGI(
            TAG,
            "iTunes resultCount: %d",
            result_count->valueint
        );
    }

    cJSON *results =
        cJSON_GetObjectItem(root, "results");

    if (results == NULL ||
        !cJSON_IsArray(results) ||
        cJSON_GetArraySize(results) == 0) {

        ESP_LOGW(TAG, "No artwork found");

        cJSON_Delete(root);
        free(response);

        return ESP_ERR_NOT_FOUND;
    }

    cJSON *first_result =
        cJSON_GetArrayItem(results, 0);

    cJSON *artwork_url =
        cJSON_GetObjectItem(
            first_result,
            "artworkUrl100"
        );

    if (artwork_url == NULL ||
        !cJSON_IsString(artwork_url)) {

        ESP_LOGW(
            TAG,
            "Artwork URL not found in result"
        );

        cJSON_Delete(root);
        free(response);

        return ESP_ERR_NOT_FOUND;
    }

ESP_LOGI(TAG, "Artwork URL:");
ESP_LOGI(TAG, "%s", artwork_url->valuestring);


/*
 * Download the artwork image.
 */
ESP_LOGI(TAG, "Starting artwork download...");

esp_http_client_config_t image_config = {
    .url = artwork_url->valuestring,
    .method = HTTP_METHOD_GET,
    .timeout_ms = 15000,
    .buffer_size = 4096,
    .buffer_size_tx = 2048,
    .keep_alive_enable = false,
    .crt_bundle_attach = esp_crt_bundle_attach,
};

esp_http_client_handle_t image_client =
    esp_http_client_init(&image_config);

if (image_client == NULL) {

    ESP_LOGE(
        TAG,
        "Failed to create artwork HTTP client"
    );

    cJSON_Delete(root);
    free(response);

    return ESP_FAIL;
}


/*
 * Open the artwork connection.
 */
err = esp_http_client_open(image_client, 0);

if (err != ESP_OK) {

    ESP_LOGE(
        TAG,
        "Artwork HTTP open failed: %s",
        esp_err_to_name(err)
    );

    esp_http_client_cleanup(image_client);
    cJSON_Delete(root);
    free(response);

    return err;
}


/*
 * Read artwork HTTP headers.
 */
int image_content_length =
    esp_http_client_fetch_headers(image_client);

int image_status =
    esp_http_client_get_status_code(image_client);

ESP_LOGI(
    TAG,
    "Artwork HTTP status: %d",
    image_status
);

ESP_LOGI(
    TAG,
    "Artwork content length: %d",
    image_content_length
);


/*
 * Allocate memory for the image.
 *
 * 100x100 artwork should normally be much smaller
 * than this limit.
 */
#define ARTWORK_MAX_SIZE (64 * 1024)

int image_buffer_size;

if (image_content_length > 0 &&
    image_content_length < ARTWORK_MAX_SIZE) {

    image_buffer_size = image_content_length;

} else {

    image_buffer_size = ARTWORK_MAX_SIZE;
}

uint8_t *image_data =
    malloc(image_buffer_size);

if (image_data == NULL) {

    ESP_LOGE(
        TAG,
        "Failed to allocate artwork buffer (%d bytes)",
        image_buffer_size
    );

    esp_http_client_close(image_client);
    esp_http_client_cleanup(image_client);

    cJSON_Delete(root);
    free(response);

    return ESP_ERR_NO_MEM;
}

/*
 * Read the image bytes.
 */
int image_total_read = 0;

while (image_total_read < image_buffer_size) {

    int remaining =
        image_buffer_size - image_total_read;

    int read_len =
        esp_http_client_read(
            image_client,
            (char *)image_data + image_total_read,
            remaining
        );

    if (read_len < 0) {

        ESP_LOGE(
            TAG,
            "Artwork HTTP read failed"
        );

        free(image_data);

        esp_http_client_close(image_client);
        esp_http_client_cleanup(image_client);

        cJSON_Delete(root);
        free(response);

        return ESP_FAIL;
    }

    if (read_len == 0) {
        break;
    }

    image_total_read += read_len;
}


ESP_LOGI(TAG, "Artwork downloaded: %d bytes", image_total_read);

if (image_total_read >= 3) {
    ESP_LOGI(
        TAG,
        "Artwork first bytes: %02X %02X %02X",
        image_data[0],
        image_data[1],
        image_data[2]
    );
}

/*
 * Decode JPEG → RGB565.
 */
err = decode_artwork(
    image_data,
    image_total_read
);

if (err != ESP_OK) {

    ESP_LOGE(
        TAG,
        "Artwork JPEG decode failed"
    );

} else {

    ESP_LOGI(
        TAG,
        "Artwork JPEG decode successful"
    );
}

/*
 * We are NOT decoding the image yet.
 *
 * Just free it after confirming the download works.
 */
free(image_data);

esp_http_client_close(image_client);
esp_http_client_cleanup(image_client);


cJSON_Delete(root);
free(response);

return ESP_OK;
}