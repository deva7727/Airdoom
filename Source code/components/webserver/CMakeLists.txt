idf_component_register(SRCS "webserver.c"
                    INCLUDE_DIRS "interface"
                    REQUIRES esp_http_server esp_wifi esp_event esp_netif
                    PRIV_REQUIRES hccapx_serializer pcap_serializer wifi_controller)
target_include_directories(${COMPONENT_LIB} PRIVATE "../../main")