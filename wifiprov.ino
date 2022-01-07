#include "WiFiProv.h"
#include <esp_wifi.h>
#include <nvs_flash.h>

bool wifi_is_provisioned(void)
{
    wifi_config_t conf;
    esp_err_t err = esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);
    if (err != ESP_OK)
    {
        Serial.printf("esp_wifi_get_config => %s\n", esp_err_to_name(err));
        return false;
    }

    // Serial.print("SSID ");
    // Serial.println((const char *)conf.sta.ssid);

    // Serial.print("psk ");
    // Serial.println((const char *)conf.sta.password);

    return strlen((const char *)conf.sta.ssid) > 0;
}

void wifi_clear_credentials(void)
{
    // wifi_config_t conf;
    // esp_err_t err = esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);
    // if (err != ESP_OK)
    // {
    //     Serial.printf("esp_wifi_get_config => %s\n", esp_err_to_name(err));
    //     return;
    // }
    // strcpy((char *)conf.sta.ssid, "");
    // strcpy((char *)conf.sta.password, "");
    // err = esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);
    // if (err != ESP_OK)
    // {
    //     Serial.printf("esp_wifi_set_config => %s\n", esp_err_to_name(err));
    // }
    nvs_flash_init();
    nvs_flash_erase();
}

static void wifi_prov_event(arduino_event_t *sys_event)
{
    switch (sys_event->event_id)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("\nConnected IP address : ");
        Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("\nDisconnected. Connecting to the AP again... ");
        break;
    case ARDUINO_EVENT_PROV_START:
        Serial.println("\nProvisioning started\nGive Credentials of your access point using \" Android app \"");
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
    {
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *)sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *)sys_event->event_info.prov_cred_recv.password);
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_FAIL:
    {
        drawProvMsg("Provisioning", "Failed");
        Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
        if (sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
            Serial.println("\nWi-Fi AP password incorrect");
        else
            Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
        wifi_clear_credentials();
        delay(3000);
        esp_restart();
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        drawProvMsg("Provisioning", "Successful");
        Serial.println("\nProvisioning Successful");
        delay(3000);
        esp_restart();
        break;
    case ARDUINO_EVENT_PROV_END:
        Serial.println("\nProvisioning Ends");
        break;
    default:
        break;
    }
}

void wifi_start_provision(const char *service, const char *pop)
{
    WiFi.onEvent(wifi_prov_event);
    WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service);
}
