idf_component_register(SRCS "attack_dos.c" "attack_method.c" "main.c" "attack_handshake.c" "attack_pmkid.c" "attack.c" "attack_passive.c" "attack_beaconspam.c" "attack_massdeauth.c"
                    INCLUDE_DIRS .
                    PRIV_REQUIRES wifi_controller webserver wsl_bypasser frame_analyzer pcap_serializer hccapx_serializer 
                                   esp_wifi esp_event esp_netif esp_timer nvs_flash lwip)
