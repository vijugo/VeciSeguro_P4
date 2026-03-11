idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ble_config ec25_modem wifi_manager ethernet_manager sd_card_manager nvs_flash log esp_wifi esp_event esp_netif)
