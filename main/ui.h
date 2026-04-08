#pragma once

#include "lvgl.h"

extern lv_obj_t *lbl_status;
extern lv_obj_t *lbl_val_soot;
extern lv_obj_t *lbl_val_dist;
extern lv_obj_t *lbl_val_temp;
extern lv_obj_t *lbl_val_diff;

void ui_init(void);
void set_regeneration_active(int status);