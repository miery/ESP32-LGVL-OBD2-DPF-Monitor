#include "obd_logic.h"
#include "main.h"
#include "ble_obd.h"
#include "ui.h"

static const char* init_cmds[] = {
    "ATZ","ATE0","ATL0","ATSP6","ATSH7E0","ATCAF1"
};

static const char* obd_queries[] = {
    "223275","220005","223035","223277","223274"
};

static int init_step = 0;
static int query_idx = 0;

void obd_process(uint32_t now, uint32_t *last_obd_ms) {

    if (state == STATE_INIT_OBD) {

        if (now - *last_obd_ms > 1500) {
            ble_write_obd(init_cmds[init_step]);
            init_step++;

            if (init_step >= sizeof(init_cmds)/sizeof(init_cmds[0])) {
                state = STATE_READY;
				lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x0000FF), 0);
                lv_label_set_text(lbl_status, "ASTRA J: CONNECTED");
            }

            *last_obd_ms = now;
        }
    }

    else if (state == STATE_READY) {

        if (now - *last_obd_ms > 800) {
            ble_write_obd(obd_queries[query_idx]);
            query_idx = (query_idx + 1) % 5;
            *last_obd_ms = now;
        }

        if (data_ready) {
            lv_label_set_text_fmt(lbl_val_soot, "%d", val_soot);
            lv_label_set_text_fmt(lbl_val_temp, "%d", val_temp);
            lv_label_set_text_fmt(lbl_val_diff, "%d", val_diff);
            lv_label_set_text_fmt(lbl_val_dist, "%d", val_dist);

            set_regeneration_active(val_regen > 0);
            data_ready = false;
        }
    }
}