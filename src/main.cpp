#include "lvgl.h"
#include <Wire.h>
#include <string.h>

#define SFR02_ADDRESS 0x70

#define MAX_HISTORY_SIZE 10

static lv_obj_t* label_distance;
static lv_timer_t* measure_timer = NULL;

static int last_measured_distance = -1;
static int distance_history[MAX_HISTORY_SIZE];
static int history_index = 0;
static lv_obj_t* history_list_label;

static void update_history_display(void) {
    char history_buf[MAX_HISTORY_SIZE * 15 + 50];
    strcpy(history_buf, "Mesures enregistrees :\n");

    for (int i = 0; i < history_index; i++) {
        char temp_buf[20];
        snprintf(temp_buf, sizeof(temp_buf), "%d cm\n", distance_history[i]);
        strcat(history_buf, temp_buf);
    }
    lv_label_set_text(history_list_label, history_buf);
}

static void save_distance_event_cb(lv_event_t * e) {
    if (last_measured_distance != -1) {
        if (history_index < MAX_HISTORY_SIZE) {
            distance_history[history_index++] = last_measured_distance;
        } else {
            for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
                distance_history[i] = distance_history[i+1];
            }
            distance_history[MAX_HISTORY_SIZE - 1] = last_measured_distance;
        }
        update_history_display();
    }
}

static void clear_history_event_cb(lv_event_t * e) {
    history_index = 0;
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        distance_history[i] = 0;
    }
    update_history_display();
}

static void measure_distance(lv_timer_t * timer)
{
    Wire.beginTransmission(SFR02_ADDRESS);
    Wire.write(0x00);
    Wire.write(0x51);
    Wire.endTransmission();

    delay(70);

    Wire.beginTransmission(SFR02_ADDRESS);
    Wire.write(0x02);
    Wire.endTransmission(false);

    Wire.requestFrom(SFR02_ADDRESS, 2);
    if (Wire.available() == 2) {
        uint8_t highByte = Wire.read();
        uint8_t lowByte = Wire.read();
        int distance_cm = (highByte << 8) | lowByte;

        last_measured_distance = distance_cm;

        char buf[32];
        snprintf(buf, sizeof(buf), "Distance : %d cm", distance_cm);
        lv_label_set_text(label_distance, buf);
    } else {
        lv_label_set_text(label_distance, "Erreur I2C !");
        last_measured_distance = -1;
    }
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        if (obj == lv_obj_get_child(lv_obj_get_parent(label_distance), 0)) {
            measure_distance(NULL);
        }
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        bool toggled = lv_obj_has_state(obj, LV_STATE_CHECKED);

        if(toggled) {
            if(measure_timer == NULL) {
                measure_timer = lv_timer_create(measure_distance, 500, NULL);
            }
        } else {
            if(measure_timer) {
                lv_timer_del(measure_timer);
                measure_timer = NULL;
            }
        }
    }
}

void testLvgl()
{
    lv_obj_t * btn1 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -165, -40);
    lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_t * label = lv_label_create(btn1);
    lv_label_set_text(label, "Mesurer");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, -150, 30);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);
    label = lv_label_create(btn2);
    lv_label_set_text(label, "Mesurer CTN");
    lv_obj_center(label);

    label_distance = lv_label_create(lv_scr_act());
    lv_label_set_text(label_distance,"Distance : ---");
    lv_obj_align(label_distance, LV_ALIGN_CENTER, 0, 0);

    history_list_label = lv_label_create(lv_scr_act());
    lv_label_set_text(history_list_label, "Mesures enregistrees:\n");
    lv_obj_set_width(history_list_label, LV_SIZE_CONTENT);
    lv_obj_set_height(history_list_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(history_list_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(history_list_label, LV_ALIGN_TOP_RIGHT, -10, 5);
    update_history_display();

    lv_obj_t * btn_save = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_save, save_distance_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -10, -50);
    lv_obj_t * label_save = lv_label_create(btn_save);
    lv_label_set_text(label_save, "Enregistrer");
    lv_obj_center(label_save);

    lv_obj_t * btn_clear = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_clear, clear_history_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_clear, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t * label_clear = lv_label_create(btn_clear);
    lv_label_set_text(label_clear, "Supprimer");
    lv_obj_center(label_clear);
}

#ifdef ARDUINO

#include "lvglDrivers.h"

void mySetup()
{
    Serial.begin(115200);
    Wire.begin();
    testLvgl();
}

void loop() {

}

void myTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}

#else

#include "lvgl.h"
#include "app_hal.h"
#include <cstdio>

int main(void)
{
    printf("LVGL Simulator\n");
    fflush(stdout);

    lv_init();
    hal_setup();

    testLvgl();

    hal_loop();
    return 0;
}

#endif