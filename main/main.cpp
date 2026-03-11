#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

// Componentes propios
#include "ble_config.h"
#include "ec25_modem.h"
#include "wifi_manager.h"
#include "ethernet_manager.h"
#include "sd_card_manager.h"

static const char *TAG = "VECISEGURO_MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Veciseguro en ESP32P4...");
    esp_log_level_set("BLE_CONFIG", ESP_LOG_DEBUG);

    // Inicializar NVS (Requerido por BLE y variables de configuración)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar Red y Event Loop (Esto dispara el arranque del coprocesador C6 via SDIO)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Inicializar el gestor de WiFi (handlers de eventos)
    wifi_manager_init();
    
    // Inicializar Ethernet (Puerto físico RJ45)
    ethernet_manager_init();

    // Inicializar SD Card
    sd_card_manager_init();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Inicializar sub-sistemas
    ble_config_init();
    ec25_modem_init();

    ESP_LOGI(TAG, "Sistema base inicializado. Corriendo en modo headless.");

    // TODO: Loop principal para gestionar máquina de estados de alarmas, MQTTs con el EC25, etc.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
