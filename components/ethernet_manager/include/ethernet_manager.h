#ifndef ETHERNET_MANAGER_H
#define ETHERNET_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ethernet_manager_init(void);
bool ethernet_manager_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // ETHERNET_MANAGER_H
