#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/i2s.h"
#include "soc/i2s_reg.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//NETWORK STUFF
#define HOST_IP_ADDR "XXX.XXX.X.X"
#define PORT 1111

static const char *TAG_UDP = "UDP_TASK";
static const char *TAG = "scan";

int counter=0;
#define DEFAULT_SSID "XXXXX"
#define DEFAULT_PWD "XXXXX"

//I2S stuff
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 1024;
static const char *TAG_I2S = "I2S LOG";
static const char *TAG_I2S_TASK = "I2S TASK";
int mic_data[2048] = {0};




static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}


/* Initialize Wi-Fi as sta and set scan method */
static void fast_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void I2SSetup() {
    esp_err_t err;

    // The I2S config as per the example
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
        .sample_rate = 16000, //16000, 32000, 44100, 48000, 96000, 112000, 128000, 144000, 160000, 176000, 192000
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // WS signal must be BCLK/64 - this is how we manage it
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // Left by default
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = 0,     						// Interrupt level 1
        .dma_buf_count = 2,                           // number of buffers
        .dma_buf_len = BLOCK_SIZE,                    // samples per buffer
		.use_apll = false
    };

    // The pin config as per the setup
    const i2s_pin_config_t pin_config = {
        .bck_io_num = 5,   // Bit Clk
        .ws_io_num = 21,    // LR Clk
        .data_out_num = -1, // Data out
        .data_in_num = 19   // Data in
    };

    // Configuring the I2S driver and pins.
    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGI(TAG_I2S," ! Failed installing driver: %d\n", err);


        while (true);
    }

    // Alterations for SPH0645 to ensure we receive MSB correctly.
    REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));   		// I2S_RX_SD_IN_DELAY
    REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);  // Phillips I2S - WS changes a cycle earlier

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
    	ESP_LOGI(TAG_I2S," ! Failed setting pin: %d\n", err);
        while (true);
    }
    ESP_LOGI(TAG_I2S,"I2S driver successfully installed.");
}


void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( ret );

    //Wifi scan and connect. Connects way after you leave the function
    fast_scan();
    I2SSetup();

    int addr_family = 0;
    int ip_protocol = 0;
    size_t bytes_read = 0;
    ESP_LOGI(TAG_I2S_TASK, "Entering while superloop??");
    while (1) {

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        //SOCK CREATE
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
        }
        ESP_LOGI(TAG_UDP, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);


        //READ SAMPLE DATA
        memset(mic_data, 0, sizeof(mic_data));
        //This Reads from DMA with double buffering 1024 bytes
        ret = i2s_read(I2S_PORT, mic_data, 1024, &bytes_read, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_I2S_TASK, "[echo] i2s read failed, errcode: %d", ret);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); //Delay for yielding to other esp32 tasks running in background
        //char payload[52]={0};
        //snprintf(payload,52,"count +%d",counter++);

        //UDP TRANSMIT
        int err = sendto(sock, mic_data, 1024, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
        	ESP_LOGE(TAG_UDP, "Error occurred during sending: errno %d", errno);
         }

        //UDP shut socket down
        if (sock != -1) {
            ESP_LOGE(TAG_UDP, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

}
