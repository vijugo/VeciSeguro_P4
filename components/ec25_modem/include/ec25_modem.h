#ifndef EC25_MODEM_H
#define EC25_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/uart.h"
#include "driver/gpio.h"

// TODO: Reemplazar el PIN_PWRKEY genérico por el pin de la placa al que se conectó.
#define EC25_UART_NUM      UART_NUM_2
#define EC25_TX_PIN        26 // Pin TX en header RS485/J5 de Guition
#define EC25_RX_PIN        27 // Pin RX en header RS485/J5 de Guition
#define EC25_PWRKEY_PIN    GPIO_NUM_20 // Pin para encender el módem (PWRKEY)

/**
 * @brief Inicializa el módem EC25 (UART y Power Key).
 */
void ec25_modem_init(void);

/**
 * @brief Envía un comando AT al módem.
 */
void ec25_send_cmd(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif // EC25_MODEM_H
