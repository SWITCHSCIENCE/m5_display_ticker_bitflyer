#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#ifdef ARDUINO_M5STACK_Core2
#include <M5Core2.h>
#include "utility/qrcode.h"
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
#include <M5Stack.h>
#include "utility/qrcode.h"
#endif
#include <Free_Fonts.h>

const char *wss_endpoint = "wss://ws.lightstream.bitflyer.com/json-rpc";
const char wss_ca_cert[] =
    /* Go Daddy Root Certificate Authority – G2 */
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"
    "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"
    "EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"
    "ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"
    "NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"
    "EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"
    "AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"
    "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"
    "E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"
    "/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"
    "DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"
    "GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"
    "tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"
    "AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"
    "FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"
    "WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"
    "9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"
    "gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"
    "2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"
    "LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"
    "4uJEvlz36hz1\n"
    "-----END CERTIFICATE-----\n";

using namespace websockets;

WebsocketsClient client;
DynamicJsonDocument doc(1024);

TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

#define MAKE_COLOR(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
#define BACKGROUND MAKE_COLOR(27, 31, 39)
#define BORDER_COLOR WHITE
#define NORMAL_TEXT_COLOR WHITE
//#define BID_TEXT_COLOR    MAKE_COLOR(251,189, 42)
//#define ASK_TEXT_COLOR    MAKE_COLOR(247,105, 77)
#define BID_TEXT_COLOR MAKE_COLOR(255, 192, 44)
#define ASK_TEXT_COLOR MAKE_COLOR(255, 108, 79)

#define REG_FONT "RobotoMono-Regular23"
#define REG_SPACEWIDTH 14

#define BOLD_FONT "RobotoMono-BoldItalic22"
#define BOLD_SPACEWIDTH 13

#define ROW_H 26
#define BASE_OFS 3

#define VOL24_X (320 - REG_SPACEWIDTH * 11)
#define VOL24_Y ROW_H * 0
#define ASK_X 0
#define ASK_Y ROW_H * 1
#define LTP_X REG_SPACEWIDTH * 7
#define LTP_Y ROW_H * 2
#define BID_X REG_SPACEWIDTH * 7
#define BID_Y ROW_H * 3

hw_timer_t *timer = NULL;

void ARDUINO_ISR_ATTR resetModule()
{
    esp_restart();
}

void onMessageCallback(WebsocketsMessage message)
{
    timerWrite(timer, 0);

    // Serial.print("Got Message: ");
    // Serial.println(message.data());

    DeserializationError error = deserializeJson(doc, message.data());
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    const char *params_channel = doc["params"]["channel"];
    JsonObject params_message = doc["params"]["message"];
    static long fx_btc_jpy_ltp = 0;
    static long btc_jpy_ltp = 0;

    if (strncmp(params_channel, "lightning_ticker_FX_BTC_JPY", sizeof("lightning_ticker_FX_BTC_JPY")) == 0)
    {
        drawTicker(params_message, canvas, 0, ROW_H * 0);
        fx_btc_jpy_ltp = long(params_message["ltp"]);
    }
    else if (strncmp(params_channel, "lightning_ticker_BTC_JPY", sizeof("lightning_ticker_BTC_JPY")) == 0)
    {
        drawTicker(params_message, canvas, 0, ROW_H * 4);
        btc_jpy_ltp = long(params_message["ltp"]);
    }
    if (btc_jpy_ltp > 0 && fx_btc_jpy_ltp > 0)
    {
        float disparity = ((float)fx_btc_jpy_ltp / (float)btc_jpy_ltp - 1) * 100.0;
        drawDisparity(disparity, canvas, 0, ROW_H * 8);
    }
}

void onEventsCallback(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        Serial.println((const char *)"Connnection Opened");

        Serial.println("Subscribe channel");
        client.send("{\"method\":\"subscribe\",\"params\":{\"channel\":\"lightning_ticker_FX_BTC_JPY\"}}");
        client.send("{\"method\":\"subscribe\",\"params\":{\"channel\":\"lightning_ticker_BTC_JPY\"}}");
    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("Connnection Closed");
    }
    else if (event == WebsocketsEvent::GotPing)
    {
        // Serial.println("Got a Ping!");
        client.pong();
    }
    else if (event == WebsocketsEvent::GotPong)
    {
        // Serial.println("Got a Pong!");
    }
}

void drawTicker(JsonObject ticker, TFT_eSprite &spr, int ofx, int ofy)
{
    //  Serial.println(long(ticker["ltp"]));
    //  Serial.println(long(ticker["best_ask"]));
    //  Serial.println(long(ticker["best_bid"]));

    long ltp = long(ticker["ltp"]);
    long best_ask = long(ticker["best_ask"]);
    long best_bid = long(ticker["best_bid"]);
    long spread = best_ask - best_bid;
    float best_ask_size = float(ticker["best_ask_size"]);
    float best_bid_size = float(ticker["best_bid_size"]);
    float volume = float(ticker["volume"]);
    // const char *product_code = ticker["product_code"];

    spr.fillScreen(BACKGROUND);
    spr.drawLine(0, ROW_H - 1, spr.width(), ROW_H - 1, BORDER_COLOR);

    // spr.loadFont(BOLD_FONT, SPIFFS);
    // spr.gFont.spaceWidth = BOLD_SPACEWIDTH;
    // spr.setTextColor(NORMAL_TEXT_COLOR, BACKGROUND);
    // spr.setCursor(0, BASE_OFS);
    // spr.print(product_code);

    // spr.loadFont(REG_FONT, SPIFFS);
    // spr.gFont.spaceWidth = REG_SPACEWIDTH;
    spr.setTextColor(NORMAL_TEXT_COLOR, BACKGROUND);
    spr.setCursor(0, BASE_OFS);
    spr.printf("%8dBTC", long(volume));
    spr.pushSprite(VOL24_X + ofx, VOL24_Y + ofy);

    spr.fillScreen(BACKGROUND);
    spr.setTextColor(ASK_TEXT_COLOR, BACKGROUND);
    spr.setCursor(0, BASE_OFS);
    spr.printf("%7.3f%8d", best_ask_size, best_ask);
    spr.pushSprite(ASK_X + ofx, ASK_Y + ofy);

    spr.fillScreen(BACKGROUND);
    spr.setTextColor(NORMAL_TEXT_COLOR, BACKGROUND);
    spr.setCursor(0, BASE_OFS);
    spr.printf("%8d%7d", ltp, spread);
    spr.pushSprite(LTP_X + ofx, LTP_Y + ofy);

    spr.fillScreen(BACKGROUND);
    spr.setTextColor(BID_TEXT_COLOR, BACKGROUND);
    spr.setCursor(0, BASE_OFS);
    spr.printf("%8d%7.3f", best_bid, best_bid_size);
    spr.pushSprite(BID_X + ofx, BID_Y + ofy);
}

void drawDisparity(float disparity, TFT_eSprite &spr, int ofx, int ofy)
{
    spr.fillScreen(BACKGROUND);
    spr.setTextColor(NORMAL_TEXT_COLOR, BACKGROUND);
    spr.setCursor(0, BASE_OFS);
    spr.printf("Disparity:%.3f%%", disparity);
    spr.pushSprite(ofx, ofy);
}

static void qrcode(const char *string, uint16_t x, uint16_t y, uint8_t width, uint8_t version, uint16_t bgcolor, uint16_t mkcolor)
{
    // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, 0, string);

    // Top quiet zone
    uint8_t thickness = width / qrcode.size;
    uint16_t lineLength = qrcode.size * thickness;
    uint8_t xOffset = x + (width - lineLength) / 2;
    uint8_t yOffset = y + (width - lineLength) / 2;
    M5.Lcd.fillRect(x, y, width, width, bgcolor);

    for (uint8_t y = 0; y < qrcode.size; y++)
    {
        for (uint8_t x = 0; x < qrcode.size; x++)
        {
            uint8_t q = qrcode_getModule(&qrcode, x, y);
            if (q)
                M5.Lcd.fillRect(x * thickness + xOffset, y * thickness + yOffset, thickness, thickness, mkcolor);
        }
    }
}

void drawProvQRcode(const char *service, const char *pop)
{
    static char msg[128];
    M5.Lcd.fillScreen(BACKGROUND);
    snprintf(msg, sizeof(msg), "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\",\"security\":\"1\"}", service, pop);
    qrcode(msg, 69, 0, 182, 6, BACKGROUND, WHITE);
    snprintf(msg, sizeof(msg), "%s/%s", service, pop);
    drawProvMsg("Provisioning started", msg);
}

void drawProvMsg(const char *msg1, const char *msg2)
{
    M5.Lcd.fillRect(0, ROW_H * 7, M5.Lcd.width(), ROW_H * 2, BACKGROUND);
    M5.Lcd.setTextColor(WHITE, BACKGROUND);
    M5.Lcd.setCursor(160 - M5.Lcd.textWidth(msg1) / 2, BASE_OFS + ROW_H * 7);
    M5.Lcd.printf("%s", msg1);
    M5.Lcd.setCursor(160 - M5.Lcd.textWidth(msg2) / 2, BASE_OFS + ROW_H * 8);
    M5.Lcd.printf("%s", msg2);
}

bool provisioning = false;
char provisioning_service[16] = "PROV_123";
char provisioning_pop[16];

void setup()
{
    M5.begin();
#ifdef ARDUINO_M5STACK_Core2
    M5.Axp.SetLcdVoltage(3100);
    M5.Axp.SetSpkEnable(false);
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
    M5.Power.begin();
    M5.Speaker.end();
#endif

    if (!SPIFFS.begin(false))
    {
        Serial.println("SPIFFS Mount Failed");
    }

    M5.Lcd.fillScreen(BACKGROUND);
    M5.Lcd.loadFont(REG_FONT, SPIFFS);
    M5.Lcd.gFont.spaceWidth = REG_SPACEWIDTH;

    WiFi.enableSTA(true);

    if (wifi_is_provisioned())
    {
        canvas.createSprite(320, ROW_H);
        canvas.fillScreen(BACKGROUND);
        canvas.loadFont(BOLD_FONT, SPIFFS);
        canvas.gFont.spaceWidth = BOLD_SPACEWIDTH;
        canvas.setTextColor(WHITE, BACKGROUND);
        canvas.setCursor(0, BASE_OFS);
        canvas.print("BTC-FX/JPY");
        canvas.drawLine(0, ROW_H - 1, canvas.width(), ROW_H - 1, BORDER_COLOR);
        canvas.pushSprite(0, 0);

        canvas.fillScreen(BACKGROUND);
        canvas.setCursor(0, BASE_OFS);
        canvas.print("BTC/JPY");
        canvas.drawLine(0, ROW_H - 1, canvas.width(), ROW_H - 1, BORDER_COLOR);
        canvas.pushSprite(0, ROW_H * 4);

        canvas.fillScreen(BACKGROUND);
        canvas.loadFont(REG_FONT, SPIFFS);
        canvas.gFont.spaceWidth = REG_SPACEWIDTH;
        canvas.setCursor(0, BASE_OFS);
        canvas.print("Disparity:");
        canvas.pushSprite(0, ROW_H * 8);

        // Connect to wifi
        WiFi.begin();

        Serial.print("WiFi connecting");

        // Wait some time to connect to wifi
        for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
        {
            Serial.print(".");
            delay(1000);
        }

        Serial.println("Done");

        // run callback when messages are received
        client.onMessage(onMessageCallback);

        // run callback when events are occuring
        client.onEvent(onEventsCallback);

        // Before connecting, set the ssl fingerprint of the server
        client.setCACert(wss_ca_cert);

        timer = timerBegin(0, 80, true);                 //timer 0, div 80
        timerAttachInterrupt(timer, &resetModule, true); //attach callback
        timerAlarmWrite(timer, 60 * 1000 * 1000, false); //set time in us
        timerAlarmEnable(timer);                         //enable interrupt
    }
    else
    {
        provisioning = true;
        snprintf(provisioning_pop, sizeof(provisioning_pop), "%04d", esp_random() % 10000);
        drawProvQRcode(provisioning_service, provisioning_pop);
        wifi_start_provision(provisioning_service, provisioning_pop);
    }
}

void loop()
{
    if (!provisioning)
    {
        M5.update();
        if (M5.BtnA.pressedFor(3000))
        {
            wifi_clear_credentials();
            delay(500);
            esp_restart();
        }
        if (WiFi.isConnected())
        {
            if (client.available())
            {
                client.poll();
            }
            else
            {
                // Connect to server
                Serial.print("Websocket connecting...");
                if (client.connect(wss_endpoint))
                {
                    Serial.println("Done!");
                    client.ping();
                }
                else
                {
                    Serial.println("Not Connected!");
                    delay(5000);
                }
            }
        }
        delay(10);
    }
    else
    {
        delay(1000);
    }
}
