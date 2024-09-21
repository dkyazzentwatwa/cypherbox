#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>

// Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define LED_GPIO_PIN  13
#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define WIFI_CHANNEL_MAX  (13)

// Function Prototypes
void initWifiSniffer();  // Declaration for initialization
void runWifiSniffer();   // Declaration for loop functionality
void initDisplay();
void printOnDisplay(String message);
void wifi_sniffer_init();
void wifi_sniffer_set_channel(uint8_t channel);
const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif