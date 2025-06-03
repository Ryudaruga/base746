#include "lvgl.h"
#include <Wire.h>

#define SFR02_ADDRESS 0x70
static lv_obj_t* label_distance;
static void event_handler(lv_event_t * e)
{
  
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Mesure");
        
        lv_timer_handler();  // Important pour que LVGL tourne

        // 1. Envoyer commande de mesure en cm (0x51)
        Wire.beginTransmission(SFR02_ADDRESS);
        Wire.write(0x00);  // Registre de commande
        Wire.write(0x51);  // Commande de mesure en cm
        Wire.endTransmission();

        delay(70);  // Attendre la fin de mesure

        // 2. Lire les 2 octets du résultat
        Wire.beginTransmission(SFR02_ADDRESS);
        Wire.write(0x02);  // Registre de lecture
        Wire.endTransmission(false);  // repeated start

        Wire.requestFrom(SFR02_ADDRESS, 2);
        if (Wire.available() == 2) {
        uint8_t highByte = Wire.read();
        uint8_t lowByte = Wire.read();
        int distance_cm = (highByte << 8) | lowByte;

        // Mettre à jour l'affichage à l'écran
        char buf[32];
        snprintf(buf, sizeof(buf), "Distance : %d cm", distance_cm);
        lv_label_set_text(label_distance, buf);
        } 
        else {
            lv_label_set_text(label_distance, "Erreur I2C !");
        }

        delay(500);  // Mesure toutes les 500 ms
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Maint");

        lv_timer_handler();  // Important pour que LVGL tourne

        // 1. Envoyer commande de mesure en cm (0x51)
        Wire.beginTransmission(SFR02_ADDRESS);
        Wire.write(0x00);  // Registre de commande
        Wire.write(0x51);  // Commande de mesure en cm
        Wire.endTransmission();

        delay(70);  // Attendre la fin de mesure

        // 2. Lire les 2 octets du résultat
        Wire.beginTransmission(SFR02_ADDRESS);
        Wire.write(0x02);  // Registre de lecture
        Wire.endTransmission(false);  // repeated start

        Wire.requestFrom(SFR02_ADDRESS, 2);
        if (Wire.available() == 2) {
        uint8_t highByte = Wire.read();
        uint8_t lowByte = Wire.read();
        int distance_cm = (highByte << 8) | lowByte;

        // Mettre à jour l'affichage à l'écran
        char buf[32];
        snprintf(buf, sizeof(buf), "Distance : %d cm", distance_cm);
        lv_label_set_text(label_distance, buf);
        } 
        else {
            lv_label_set_text(label_distance, "Erreur I2C !");
        }

        delay(500);  // Mesure toutes les 500 ms
    }
}


void testLvgl()
{
  // Initialisations générales
  lv_obj_t * label;

  lv_obj_t * btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn1, LV_ALIGN_CENTER, -165, -40);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

  label = lv_label_create(btn1);
  lv_label_set_text(label, "Mesurer");
  lv_obj_center(label);

  lv_obj_t * btn2 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_align(btn2, LV_ALIGN_CENTER, -150, 30);
  lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(btn2, LV_SIZE_CONTENT);

  label = lv_label_create(btn2);
  lv_label_set_text(label, "Mesurer CTN");
  lv_obj_center(label);

  label_distance = lv_label_create(lv_scr_act());
  lv_label_set_text(label_distance,"Distance : ---");
  lv_obj_align(label_distance,LV_ALIGN_CENTER,0,0);
}

#ifdef ARDUINO

#include "lvglDrivers.h"

// à décommenter pour tester la démo
// #include "demos/lv_demos.h"

void mySetup()
{
  // à décommenter pour tester la démo
  // lv_demo_widgets();

  // Initialisations générales
  Serial.begin(115200);
  Wire.begin();
  testLvgl();
}

void loop() {

}

void myTask(void *pvParameters)
{
  // Init
  TickType_t xLastWakeTime;
  // Lecture du nombre de ticks quand la tâche débute
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    // Loop

    // Endort la tâche pendant le temps restant par rapport au réveil,
    // ici 200ms, donc la tâche s'effectue toutes les 200ms
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200)); // toutes les 200 ms
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
