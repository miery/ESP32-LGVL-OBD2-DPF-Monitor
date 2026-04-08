#include "ui.h"
#include "led.h"

lv_obj_t *lbl_status;
lv_obj_t *lbl_val_soot;
lv_obj_t *lbl_val_dist;
lv_obj_t *lbl_val_temp;
lv_obj_t *lbl_val_diff;

// Create single metric box (unchanged logic)
static void create_metric_box(
    lv_obj_t *parent,
    const char* title,
    const char* unit,
    int col,
    int row,
    lv_obj_t **val_label
) {
    lv_obj_t * obj = lv_obj_create(parent);

    lv_obj_set_grid_cell(obj,
        LV_GRID_ALIGN_STRETCH, col, 1,
        LV_GRID_ALIGN_STRETCH, row, 1);

    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_width(obj, 0, 0); 
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * l_title = lv_label_create(obj);
    lv_label_set_text(l_title, title);
    lv_obj_set_style_text_font(l_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(l_title, lv_color_white(), 0);
    lv_obj_align(l_title, LV_ALIGN_TOP_MID, 0, 0);

    *val_label = lv_label_create(obj);
    lv_label_set_text(*val_label, "---");
    lv_obj_set_style_text_font(*val_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(*val_label, lv_color_white(), 0);
    
    lv_obj_set_width(*val_label, 150); 
    lv_obj_set_style_text_align(*val_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(*val_label, LV_ALIGN_CENTER, 0, 5);

    lv_obj_t * l_unit = lv_label_create(obj);
    lv_label_set_text(l_unit, unit);
    lv_obj_set_style_text_font(l_unit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l_unit, lv_color_white(), 0);
    lv_obj_align(l_unit, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void set_regeneration_active(int status) {

    static int last_status = -1;
    if (status == last_status) return;
    last_status = status;

    if (status > 0) {
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00FF00), 0);
        lv_label_set_text(lbl_status, "REGEN ACTIVE!");
        led_set_red();
    } else {
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x0000FF), 0);
        lv_label_set_text(lbl_status, "ASTRA J: CONNECTED");
        led_set_idle();
    }
}

void ui_init(void) {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lbl_status = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_status, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_22, 0);

    lv_obj_set_style_bg_opa(lbl_status, LV_OPA_COVER, 0);
	lv_obj_set_width(lbl_status, LV_PCT(100));
	lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER, 0);
	lv_label_set_long_mode(lbl_status, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_bg_color(lbl_status, lv_color_black(), 0);
    lv_obj_set_style_pad_all(lbl_status, 8, 0);

    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 35);

    lv_label_set_text(lbl_status, "SEARCHING OBD...");

    lv_obj_t *main_cont = lv_obj_create(lv_scr_act());
	
	lv_obj_set_size(main_cont, 466 - 30, 466 - 110);
	lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 85);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_layout(main_cont, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(main_cont, col_dsc, row_dsc);
	
	lv_obj_set_style_bg_opa(main_cont, 0, 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);

    create_metric_box(main_cont, "SOOT", "%", 0, 0, &lbl_val_soot);
    create_metric_box(main_cont, "DISTANCE", "km", 1, 0, &lbl_val_dist);
    create_metric_box(main_cont, "ECT", "°C", 0, 1, &lbl_val_temp);
    create_metric_box(main_cont, "DELTA P", "kPa", 1, 1, &lbl_val_diff);
}