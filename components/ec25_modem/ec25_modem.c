#include "ec25_modem.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "EC25_MODEM";

#define BUF_SIZE 1024

static void ec25_rx_task(void *arg)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(EC25_UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Módem RX: %s", (char *)data);
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void ec25_send_cmd(const char *cmd)
{
    ESP_LOGI(TAG, "Enviando comando: %s", cmd);
    uart_write_bytes(EC25_UART_NUM, cmd, strlen(cmd));
    uart_write_bytes(EC25_UART_NUM, "\r\n", 2);
}

void ec25_modem_init(void)
{
    ESP_LOGI(TAG, "Inicializando EC25...");

    // 1. Configurar PIN de encendido (PWRKEY)
    gpio_reset_pin(EC25_PWRKEY_PIN);
    gpio_set_direction(EC25_PWRKEY_PIN, GPIO_MODE_OUTPUT);

    // Secuencia de encendido
    ESP_LOGI(TAG, "Encendiendo módem...");
    gpio_set_level(EC25_PWRKEY_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(EC25_PWRKEY_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(600)); 
    gpio_set_level(EC25_PWRKEY_PIN, 1);
    
    // Esperar a que inicie
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 2. Configurar UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(EC25_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(EC25_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(EC25_UART_NUM, EC25_TX_PIN, EC25_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Iniciar tarea de lectura
    xTaskCreate(ec25_rx_task, "ec25_rx_task", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG, "UART1 Configurado. Tarea de lectura iniciada.");
    
    // Probar conexión básica
    ec25_send_cmd("AT");
    vTaskDelay(pdMS_TO_TICKS(500));
    ec25_send_cmd("ATI"); // Información del fabricante/modelo
}
