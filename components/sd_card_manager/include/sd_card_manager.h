#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa y monta la tarjeta SD en /sdcard
 * 
 * @return esp_err_t ESP_OK si tuvo éxito
 */
esp_err_t sd_card_manager_init(void);

/**
 * @brief Verifica si la tarjeta SD está montada
 * 
 * @return true si está montada
 */
bool sd_card_manager_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_MANAGER_H
