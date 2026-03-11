#include "sd_card_manager.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_ldo_regulator.h"
#include <string.h>

static const char *TAG = "SD_CARD_MANAGER";
static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;
static esp_ldo_channel_handle_t s_ldo_handle = NULL;

#define MOUNT_POINT "/sdcard"

/**
 * Pines para Guition ESP32-P4 (Slot 0 - High Speed Pads)
 */
#define SD_D0_GPIO  39
#define SD_D1_GPIO  40
#define SD_D2_GPIO  41
#define SD_D3_GPIO  42
#define SD_CLK_GPIO 43
#define SD_CMD_GPIO 44

esp_err_t sd_card_manager_init(void)
{
    ESP_LOGI(TAG, "Inicializando Tarjeta SD (SDMMC Slot 0, 4-bit)...");

    // 1. Activar el LDO4 interno (ESP_LDO_V04) que alimenta el dominio de la SD
    // Según esquemático de Guition, LDO4 es el que alimenta VDD_SDIO
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id = 4, // LDO_VO4
        .voltage_mv = 3300,
    };
    
    ESP_LOGI(TAG, "Activando LDO4 interno a 3300mV...");
    esp_err_t ret = esp_ldo_acquire_channel(&ldo_cfg, &s_ldo_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al activar LDO4: %s", esp_err_to_name(ret));
        // Continuamos de todos modos por si ya estuviera activo o fuera otro el problema
    } else {
        ESP_LOGI(TAG, "LDO4 activado correctamente.");
    }

    // Esperar a que el voltaje se estabilice (aumentado para mayor seguridad)
    vTaskDelay(pdMS_TO_TICKS(500));

    // 2. Configurar Host (SDMMC Slot 0)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_PROBING; 

    // 3. Configurar Slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4; // Restaurar a 4-bit ya que el hardware parece responder
    slot_config.clk = SD_CLK_GPIO;
    slot_config.cmd = SD_CMD_GPIO;
    slot_config.d0 = SD_D0_GPIO;
    slot_config.d1 = SD_D1_GPIO;
    slot_config.d2 = SD_D2_GPIO;
    slot_config.d3 = SD_D3_GPIO;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    ESP_LOGI(TAG, "Configuración de pines: CLK=%d, CMD=%d, D0-D3=[%d,%d,%d,%d] (Modo 4-bit)", 
             SD_CLK_GPIO, SD_CMD_GPIO, SD_D0_GPIO, SD_D1_GPIO, SD_D2_GPIO, SD_D3_GPIO);

    // 4. Montar sistema de archivos
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true, // Si es nueva o está corrupta, formatea
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Intentando montaje con LDO4 habilitado...");
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al montar SD: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Tarjeta SD montada exitosamente en %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);
    s_mounted = true;
    
    return ESP_OK;
}

bool sd_card_manager_is_mounted(void)
{
    return s_mounted;
}
