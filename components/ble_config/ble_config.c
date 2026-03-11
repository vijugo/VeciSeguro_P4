#include "ble_config.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_CONFIG";

// UUIDs para Servicios y Características
// 128-bit UUIDs para evitar colisiones
static const ble_uuid128_t svc_uuid =
    BLE_UUID128_INIT(0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04);

static const ble_uuid128_t char_wifi_ssid_uuid =
    BLE_UUID128_INIT(0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xef, 0x05, 0x06, 0x07, 0x08);

static const ble_uuid128_t char_wifi_pass_uuid =
    BLE_UUID128_INIT(0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xef, 0x09, 0x0a, 0x0b, 0x0c);

static const ble_uuid128_t char_modem_apn_uuid =
    BLE_UUID128_INIT(0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x7a, 0x8b, 0x9c, 0xad, 0xbe, 0xef, 0x0d, 0x0e, 0x0f, 0x10);

static char wifi_ssid[33] = "Mi_WiFi";
static char wifi_pass[64] = "Password123";
static char modem_apn[64] = "internet.com";

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &char_wifi_ssid_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = &char_wifi_pass_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                .uuid = &char_modem_apn_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                0, // No más características
            }
        },
    },
    {
        0, // No más servicios
    },
};

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid = ctxt->chr->uuid;
    char uuid_str[BLE_UUID_STR_LEN];
    ble_uuid_to_str(uuid, uuid_str);
    
    ESP_LOGI(TAG, "GATT Access: op=%d, uuid=%s", ctxt->op, uuid_str);

    if (ble_uuid_cmp(uuid, &char_wifi_ssid_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGI(TAG, "Leyendo WiFi SSID: %s", wifi_ssid);
            int rc = os_mbuf_append(ctxt->om, wifi_ssid, strlen(wifi_ssid));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len >= sizeof(wifi_ssid)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            int rc = ble_hs_mbuf_to_flat(ctxt->om, wifi_ssid, sizeof(wifi_ssid) - 1, &len);
            wifi_ssid[len] = '\0';
            ESP_LOGI(TAG, "¡NUEVO VALOR RECIBIDO! WiFi SSID: %s", wifi_ssid);
            wifi_manager_connect(wifi_ssid, wifi_pass);
            return rc;
        }
    }

    if (ble_uuid_cmp(uuid, &char_wifi_pass_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGI(TAG, "Leyendo WiFi Pass: %s", wifi_pass);
            int rc = os_mbuf_append(ctxt->om, wifi_pass, strlen(wifi_pass));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len >= sizeof(wifi_pass)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            int rc = ble_hs_mbuf_to_flat(ctxt->om, wifi_pass, sizeof(wifi_pass) - 1, &len);
            wifi_pass[len] = '\0';
            ESP_LOGI(TAG, "¡NUEVO VALOR RECIBIDO! WiFi Pass: %s", wifi_pass);
            wifi_manager_connect(wifi_ssid, wifi_pass);
            return rc;
        }
    }

    if (ble_uuid_cmp(uuid, &char_modem_apn_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            ESP_LOGI(TAG, "Leyendo APN: %s", modem_apn);
            int rc = os_mbuf_append(ctxt->om, modem_apn, strlen(modem_apn));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len >= sizeof(modem_apn)) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            int rc = ble_hs_mbuf_to_flat(ctxt->om, modem_apn, sizeof(modem_apn) - 1, &len);
            modem_apn[len] = '\0';
            ESP_LOGI(TAG, "¡NUEVO VALOR RECIBIDO! APN: %s", modem_apn);
            return rc;
        }
    }

    ESP_LOGW(TAG, "GATT Access: UUID no manejado");
    return BLE_ATT_ERR_UNLIKELY;
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "--- EVENTO: BLE Conectado! status=%d ---", event->connect.status);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "--- EVENTO: BLE Desconectado! razon=%d ---", event->disconnect.reason);
        // Reiniciar advertising
        struct ble_gap_adv_params adv_params;
        memset(&adv_params, 0, sizeof adv_params);
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
        ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                          &adv_params, ble_gap_event, NULL);
        ESP_LOGI(TAG, "BLE Advertising reiniciado por desconexión");
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "--- EVENTO: Advertising completo ---");
        break;
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "--- EVENTO: MTU actualizado: %d ---", event->mtu.value);
        break;
    }
    return 0;
}

static void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE Host Sincronizado Correctamente");

    // Configurar publicidad (advertising)
    struct ble_hs_adv_fields fields;
    const char *name;

    memset(&fields, 0, sizeof fields);
    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    // Agregar el UUID del servicio al anuncio para que las apps lo identifiquen mejor
    fields.uuids128 = &svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                      &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error al iniciar publicidad: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE Advertising Activo: '%s'", name);
    }
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Iniciado");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];
    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "Registrado servicio: %s handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        break;
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG, "Registrada característica: %s def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;
    default:
        break;
    }
}

void ble_config_init(void)
{
    ESP_LOGI(TAG, "Inicializando BLE NimBLE...");

    nimble_port_init();

    // Nombre del dispositivo
    ble_svc_gap_device_name_set("Veciseguro-Config");
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Registrar servicios
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error en count_cfg: %d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error en add_svcs: %d", rc);
        return;
    }

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb; // <--- Verificación de registro

    nimble_port_freertos_init(ble_host_task);
}
