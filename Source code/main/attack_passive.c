#include "attack_passive.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "attack.h"
#include "wifi_controller.h"
#include "pcap_serializer.h"

static const char *TAG = "attack_passive";

static void data_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    // Cast the event data to the promiscuous packet type
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    
    // Append the captured raw IEEE 802.11 frame directly to the PCAP buffer
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
}

void attack_passive_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting passive sniffer attack...");
    
    // Initialize the PCAP format structure
    pcap_serializer_init();

    // Configure the sniffer to listen for ALL frames (Management, Control, Data)
    wifictl_sniffer_filter_frame_types(true, true, true);
    
    // Switch to the target AP's channel since we only want to sniff its traffic
    wifictl_sniffer_start(attack_config->ap_record->primary);
    
    // Register event handler to catch the sniffer's raw frames
    // Wait, the sniffer posts events using SNIFFER_EVENTS base. We must capture MGMT, CTRL, DATA
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, ESP_EVENT_ANY_ID, &data_frame_handler, NULL));
}

void attack_passive_stop() {
    ESP_LOGI(TAG, "Stopping passive sniffer attack...");
    
    wifictl_sniffer_stop();
    wifictl_mgmt_ap_start();
    
    esp_err_t err = esp_event_handler_unregister(SNIFFER_EVENTS, ESP_EVENT_ANY_ID, &data_frame_handler);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed unregister passive handler: %d", err);
    }
    
    ESP_LOGI(TAG, "Passive attack stopped");
}
