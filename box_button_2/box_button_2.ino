#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <ClickButton.h>
#include <ArduinoJson.h>

#define WIFI_SSID "W-IOT"
#define WIFI_AUTH ""

// data from DHCP: IP address, network mask, Gateway, DNS
#define DATA_IPADDR IPAddress(10, 99, 20, 137)
#define DATA_IPMASK IPAddress(255, 255, 255, 0)
#define DATA_GATEWY IPAddress(10, 99, 20, 1)
#define DATA_DNS1 IPAddress(1, 1, 1, 1)

// data from your Wifi connect: Channel, BSSID
#define DATA_WIFI_CH 11
#define DATA_WIFI_BSSID                \
  {                                    \
    0x86, 0x2a, 0xa8, 0xd4, 0xc2, 0xca \
  }

ClickButton B(0, LOW);

CRGB leds[1];

int hue = 0;

int fadeVal = 0;
int fadeOff = 50;
bool fadeDir = 0;

unsigned long action = 0;
bool rssi = false;

#define MSG_BUFFER_SIZE 64
char msg[MSG_BUFFER_SIZE];
char topic[16];

const char *mqtt_server = "10.99.20.12";

WiFiClient box;
PubSubClient client(box);

void reconnect()
{
  while (!client.connected())
  {

    leds[0] = CRGB(255, 255, 255);
    FastLED.show();

    if (client.connect("Remote", "mqtt", "passwd"))
    {
      client.publish("devices", "box");

      client.subscribe("box");
    }
    else
    {
      delay(500);
    }
  }
}

int wifi_fast()
{
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);

  WiFi.config(DATA_IPADDR, DATA_GATEWY, DATA_IPMASK);
  uint8_t my_bssid[] = DATA_WIFI_BSSID;
  char my_ssid[] = WIFI_SSID;
  char my_auth[] = WIFI_AUTH;
  WiFi.begin(&my_ssid[0], &my_auth[0], DATA_WIFI_CH, &my_bssid[0], true);

  //  WiFi.begin(WIFI_SSID, WIFI_AUTH);
  uint32_t timeout = millis() + 5000;
  while ((WiFi.status() != WL_CONNECTED) && (millis() < timeout))
  {
    delay(5);

    leds[0] = CRGB(fadeVal, 0, 0);
    if (fadeDir)
      fadeVal--;
    else
      fadeVal++;
    if (fadeVal >= 255)
      fadeDir = 1;
    if (fadeVal <= 0)
      fadeDir = 0;
    FastLED.show();
  }
  return (WiFi.status() == WL_CONNECTED);
}

void setup()
{
  pinMode(1, FUNCTION_3);
  pinMode(3, FUNCTION_3);
  FastLED.addLeds<WS2812, 3, GRB>(leds, 1);
  leds[0] = CRGB::Black;
  FastLED.show();

  //  leds[0] = CRGB::Red;
  //  FastLED.show();
  //  delay(200);
  //  leds[0] = CRGB::Green;
  //  FastLED.show();
  //  delay(200);
  //  leds[0] = CRGB::Blue;
  //  FastLED.show();
  //  delay(200);
  //  leds[0] = CRGB::Black;
  //  FastLED.show();
  //  delay(200);

  //  WiFi.mode(WIFI_STA);
  //  WiFi.begin(ssid, password);
  //
  //  while (WiFi.status() != WL_CONNECTED) {
  //
  //    leds[0] = CRGB(fadeVal, 0, 0);
  //    if (fadeDir) fadeVal--;
  //    else fadeVal++;
  //    if (fadeVal >= 255)
  //      fadeDir = 1;
  //    if (fadeVal <= 0)
  //      fadeDir = 0;
  //    FastLED.show();
  //
  //    delay(10);
  //  }

  wifi_fast();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  B.Update();

  if (B.clicks != 0)
  {

    if (B.clicks == -2)
    {
      rssi = !rssi;
    }
    else
    {
      StaticJsonDocument<32> doc;
      doc["Button1"]["Action"] = B.clicks;

      char output[32];
      serializeJson(doc, output);

      client.publish("stat/box/RESULT", output);
    }

    action = millis();
  }

  if (millis() - action < 600)
  {

    leds[0] = CRGB(0, 0, ((millis() - action) / 200) % 2 ? 255 : 0);
  }
  else
  {
    if (rssi)
    {
      int rssi_now = constrain(WiFi.RSSI(), -80, -20);
      int rssi_green = map(rssi_now, -80, -20, 0, 255);
      int rssi_red = map(rssi_now, -80, -20, 255, 0);
      leds[0] = CRGB(rssi_red, rssi_green, 0);
    }
    else
    {
      leds[0] = CHSV(hue / 10, 255, 255);
      hue++;
      if (hue > 255 * 10)
        hue = 0;
    }
  }
  FastLED.show();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  memcpy(msg, payload, length);
  msg[length] = NULL;

  //  if (strcmp(topic, "box") == 0) {
  //    if (strcmp(msg, "test") == 0) {
  //
  //    }
  //  }
}
