#include "ble_obd.h"
#include "main.h"
#include "ui.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "OBD_DPF";

// Target BLE device name
#define TARGET_NAME "vLinker MC-IOS"

// Forward declaration
static int gap_cb(struct ble_gap_event *event, void *arg);

// =====================================================
// SEND OBD COMMAND (UNCHANGED)
// =====================================================
void ble_write_obd(const char *cmd) {

    if (obd_char_handle == 0 || conn_handle == 0) return;

    uint8_t data[32];
    int len = snprintf((char*)data, sizeof(data), "%s\r", cmd);

    ESP_LOGI(TAG, "TX: %s", cmd);

    ble_gattc_write_flat(conn_handle, obd_char_handle, data, len, NULL, NULL);
}

// =====================================================
// START SCAN (UNCHANGED)
// =====================================================
static void start_scan(void) {

    ESP_LOGI(TAG, "Starting scan...");

    struct ble_gap_disc_params disc_params = {0};
    disc_params.filter_duplicates = 1;

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 0, &disc_params, gap_cb, NULL);
}

// =====================================================
// CHARACTERISTIC DISCOVERY CALLBACK (UNCHANGED)
// =====================================================
static int chr_cb(uint16_t conn_id,
                  const struct ble_gatt_error *error,
                  const struct ble_gatt_chr *chr,
                  void *arg) {

    if (error->status == 0 && chr != NULL) {

        if ((chr->properties & BLE_GATT_CHR_PROP_WRITE) &&
            (chr->properties & BLE_GATT_CHR_PROP_NOTIFY)) {

            obd_char_handle = chr->val_handle;

            ESP_LOGI(TAG, "OBD handle found: %d", obd_char_handle);

            // Enable notifications
            uint16_t notify_en = 0x0001;
            ble_gattc_write_flat(conn_id, obd_char_handle + 1,
                                 &notify_en, 2, NULL, NULL);

            state = STATE_INIT_OBD;
        }
    }

    return 0;
}

// =====================================================
// GAP EVENT HANDLER (UNCHANGED)
// =====================================================
static int gap_cb(struct ble_gap_event *event, void *arg) {

    struct ble_hs_adv_fields fields;

    switch (event->type) {

        // -----------------------------
        // DEVICE DISCOVERY
        // -----------------------------
        case BLE_GAP_EVENT_DISC:

            ble_hs_adv_parse_fields(&fields,
                                   event->disc.data,
                                   event->disc.length_data);

            if (fields.name_len > 0 &&
                strncmp((char *)fields.name,
                        TARGET_NAME,
                        fields.name_len) == 0) {

                ESP_LOGI(TAG, "Device found! Connecting...");
                ble_gap_disc_cancel();

                struct ble_gap_conn_params conn_p = {
                    .scan_itvl=128,
                    .scan_window=64,
                    .itvl_min=24,
                    .itvl_max=40,
                    .supervision_timeout=256
                };

                ble_gap_connect(BLE_OWN_ADDR_PUBLIC,
                                &event->disc.addr,
                                30000,
                                &conn_p,
                                gap_cb,
                                NULL);
            }
            break;

        // -----------------------------
        // CONNECTED
        // -----------------------------
        case BLE_GAP_EVENT_CONNECT:

            if (event->connect.status == 0) {

                ESP_LOGI(TAG, "Connected!");

                conn_handle = event->connect.conn_handle;
                state = STATE_CONNECTED;

                ble_gattc_disc_all_chrs(conn_handle,
                                        1,
                                        0xFFFF,
                                        chr_cb,
                                        NULL);
            } else {
                start_scan();
            }
            break;

        // -----------------------------
        // DATA RECEIVED (OBD PARSER)
        // -----------------------------
        case BLE_GAP_EVENT_NOTIFY_RX: {

            char rx[64] = {0};

            os_mbuf_copydata(event->notify_rx.om,
                             0,
                             OS_MBUF_PKTLEN(event->notify_rx.om),
                             rx);

            ESP_LOGI(TAG, "RX: %s", rx);

            unsigned int a, b, c;
            char *p;

            // Distance
            if ((p = strstr(rx, "62 32 77")) &&
                sscanf(p + 9, "%x %x %x", &a, &b, &c) == 3) {

                val_dist = (a << 16) | (b << 8) | c;
                data_ready = true;
            }

            // Soot
            else if ((p = strstr(rx, "62 32 75")) &&
                     sscanf(p + 9, "%x", &a) == 1) {

                val_soot = (int)a;
                data_ready = true;
            }

            // Coolant temp
            else if ((p = strstr(rx, "62 00 05")) &&
                     sscanf(p + 9, "%x", &a) == 1) {

                val_temp = a - 40;
                data_ready = true;
            }

            // Differential pressure
            else if ((p = strstr(rx, "62 30 35")) &&
                     sscanf(p + 9, "%x", &a) == 1) {

                int raw_val = (int)a;

                if (raw_val <= 20) {
                    val_diff = 0;
                } else {
                    val_diff = raw_val - 20;
                }

                data_ready = true;
            }

            // Regeneration status
            else if ((p = strstr(rx, "62 32 74")) &&
                     sscanf(p + 9, "%x", &a) == 1) {

                val_regen = a;
                data_ready = true;
            }

            break;
        }

        // -----------------------------
        // DISCONNECTED
        // -----------------------------
        case BLE_GAP_EVENT_DISCONNECT:

            ESP_LOGI(TAG, "Disconnected! Reason: %d",
                     event->disconnect.reason);

            state = STATE_INIT_OBD;
            conn_handle = 0;

            if (lbl_status) {
                lv_label_set_text(lbl_status, "SEARCHING OBD...");
            }

            start_scan();
            break;
    }

    return 0;
}

// =====================================================
// BLE INIT (NEW WRAPPER FROM ORIGINAL MAIN)
// =====================================================
void ble_init(void) {

    nimble_port_init();

    ble_hs_cfg.sync_cb = start_scan;

    xTaskCreatePinnedToCore(
        (void*)nimble_port_run,
        "nimble",
        4096,
        NULL,
        10,
        NULL,
        0
    );
}