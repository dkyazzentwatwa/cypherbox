#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_4x6_mf); // Set the font 
}

// Add this function to print on the display
void printOnDisplay(String message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  // Set the box coordinates and size
  /*int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  display.drawRect(x1 - 2, y1 - 2, w + 4, h + 4, SSD1306_WHITE);
*/
  display.println(message);
  display.display();
  //u8g2 addition
  /*
  display.clearDisplay()'
  u8g2_for_adafruit_gfx.setFont(u8g2_font_tom_thumb_4x6_mr); // Set back to small font
  u8g2_for_adafruit_gfx.setCursor(0, 0);
  u8g2_for_adafruit_gfx.print(message);
  display.display();
  */
}

#define LED_GPIO_PIN                     13
#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define WIFI_CHANNEL_MAX               (13)

uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

//static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
//static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

/*esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}
*/
//gemini
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
  // Handle events here
}

void wifi_sniffer_init(void)
{
  nvs_flash_init();
  //tcpip_adapter_init();
  //ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  //gemini
  ESP_ERROR_CHECK( esp_event_loop_create_default() );
  ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:  
  case WIFI_PKT_MISC: return "MISC";
  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  String message = "PACKET TYPE=";
  message += wifi_sniffer_packet_type2str(type);
  message += "\nCHAN=";
  message += ppkt->rx_ctrl.channel;
  message += "\nRSSI=";
  message += ppkt->rx_ctrl.rssi;
  message += "\nA1=";
  message += String(hdr->addr1[0], HEX) + ":" + String(hdr->addr1[1], HEX) + ":" + String(hdr->addr1[2], HEX) + ":" + String(hdr->addr1[3], HEX) + ":" + String(hdr->addr1[4], HEX) + ":" + String(hdr->addr1[5], HEX);
  message += "\nA2=";
  message += String(hdr->addr2[0], HEX) + ":" + String(hdr->addr2[1], HEX) + ":" + String(hdr->addr2[2], HEX) + ":" + String(hdr->addr2[3], HEX) + ":" + String(hdr->addr2[4], HEX) + ":" + String(hdr->addr2[5], HEX);
  message += "\nA3=";
  message += String(hdr->addr3[0], HEX) + ":" + String(hdr->addr3[1], HEX) + ":" + String(hdr->addr3[2], HEX) + ":" + String(hdr->addr3[3], HEX) + ":" + String(hdr->addr3[4], HEX) + ":" + String(hdr->addr3[5], HEX);

  Serial.println(message);
  printOnDisplay(message); // Print on the display
}

// the loop function runs over and over again forever
void setup() {
  Serial.begin(115200);
  delay(10);
  initDisplay(); // Initialize the display
  wifi_sniffer_init();
  pinMode(LED_GPIO_PIN, OUTPUT);
}

void loop() {
  delay(1000);
  
  if (digitalRead(LED_GPIO_PIN) == LOW)
    digitalWrite(LED_GPIO_PIN, HIGH);
  else
    digitalWrite(LED_GPIO_PIN, LOW);
  
  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  wifi_sniffer_set_channel(channel);
  channel = (channel % WIFI_CHANNEL_MAX) + 1;
}