#include "lvgl.h"          // Bibliothèque graphique LVGL
#include <Wire.h>          // Communication I2C
#include <string.h>        // Fonctions pour manipuler les chaînes de caractères

#define SFR02_ADDRESS 0x70 // Adresse I2C du capteur SFR02
#define MAX_HISTORY_SIZE 10 // Nombre maximal de mesures sauvegardées

// Déclarations des objets LVGL utilisés pour l'interface
static lv_obj_t* label_distance;
static lv_obj_t* label_valeur1;
static lv_obj_t* label_valeur2;
static lv_obj_t* label_surface;
static lv_timer_t* measure_timer = NULL; // Timer pour les mesures continues

// Variables pour stockage des mesures
static int last_measured_distance = -1;
static int distance_history[MAX_HISTORY_SIZE];
static int history_index = 0;
static lv_obj_t* history_list_label;

// Met à jour l'affichage de l'historique des mesures
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

// Callback : enregistre la dernière distance mesurée dans l'historique
static void save_distance_event_cb(lv_event_t * e) {
    if (last_measured_distance != -1) {
        if (history_index < MAX_HISTORY_SIZE) {
            distance_history[history_index++] = last_measured_distance;
        } else {
            // Décalage des valeurs pour faire de la place
            for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
                distance_history[i] = distance_history[i+1];
            }
            distance_history[MAX_HISTORY_SIZE - 1] = last_measured_distance;
        }
        update_history_display(); // Rafraîchit l'affichage
    }
}

int val1 = 0;
int val2 = 0;
float surface;

// Callback : assigne la valeur mesurée à val1
static void valeur1(lv_event_t * e){
    char buf[32];
    val1 = last_measured_distance;
    snprintf(buf, sizeof(buf), "valeur1 : %d cm", val1);
    lv_label_set_text(label_valeur1, buf);

    // Calcule la surface si val2 est déjà défini
    if(val2 != 0){
        surface = ((val1/100.0f)*(val2/100.0f));
        snprintf(buf, sizeof(buf), "surface : %.2f m2", surface);
        lv_label_set_text(label_surface, buf);
    }
}

// Callback : assigne la valeur mesurée à val2
static void valeur2(lv_event_t * e){
    char buf[32];
    val2 = last_measured_distance;
    snprintf(buf, sizeof(buf), "valeur2 : %d cm", val2);
    lv_label_set_text(label_valeur2, buf);

    // Calcule la surface si val1 est déjà défini
    if(val1 != 0){
        surface = ((val1/100.0f)*(val2/100.0f));
        snprintf(buf, sizeof(buf), "surface : %.2f m2", surface);
        lv_label_set_text(label_surface, buf);
    }
}

// Callback : réinitialise l'historique des mesures
static void clear_history_event_cb(lv_event_t * e) {
    history_index = 0;
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        distance_history[i] = 0;
    }
    update_history_display();
}

// Fonction de mesure de la distance via le capteur SFR02
static void measure_distance(lv_timer_t * timer)
{
    // Envoi de la commande de mesure (0x51)
    Wire.beginTransmission(SFR02_ADDRESS);
    Wire.write(0x00);
    Wire.write(0x51);
    Wire.endTransmission();

    delay(70); // Attente du résultat de la mesure

    // Lecture du résultat (2 octets)
    Wire.beginTransmission(SFR02_ADDRESS);
    Wire.write(0x02); // Registre de lecture
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
        // Erreur de communication I2C
        lv_label_set_text(label_distance, "Erreur I2C !");
        last_measured_distance = -1;
    }
}

// Gère les événements sur les boutons (clic, état changé, etc.)
static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        // Si bouton de mesure cliqué -> effectuer une mesure
        if (obj == lv_obj_get_child(lv_obj_get_parent(label_distance), 0)) {
            measure_distance(NULL);
        }
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        // Démarrage/arrêt de la mesure continue
        bool toggled = lv_obj_has_state(obj, LV_STATE_CHECKED);

        if(toggled) {
            if(measure_timer == NULL) {
                measure_timer = lv_timer_create(measure_distance, 500, NULL); // toutes les 500ms
            }
        } else {
            if(measure_timer) {
                lv_timer_del(measure_timer);
                measure_timer = NULL;
            }
        }
    }
}

// Fonction principale de création de l'interface utilisateur
void testLvgl()
{
    // Bouton "Mesurer"
    lv_obj_t * btn1 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -165, -40);
    lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_t * label = lv_label_create(btn1);
    lv_label_set_text(label, "Mesurer");
    lv_obj_center(label);

    // Bouton "Mesurer CTN" (continu)
    lv_obj_t * btn2 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, -150, 30);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE); // Toggle
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);
    label = lv_label_create(btn2);
    lv_label_set_text(label, "Mesurer CTN");
    lv_obj_center(label);

    // Affichage de la distance
    label_distance = lv_label_create(lv_scr_act());
    lv_label_set_text(label_distance,"Distance : ---");
    lv_obj_align(label_distance, LV_ALIGN_CENTER, 0, 0);

    // Affichage de l'historique
    history_list_label = lv_label_create(lv_scr_act());
    lv_label_set_text(history_list_label, "Mesures enregistrees:\n");
    lv_obj_set_width(history_list_label, LV_SIZE_CONTENT);
    lv_obj_set_height(history_list_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(history_list_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(history_list_label, LV_ALIGN_TOP_RIGHT, -10, 5);
    update_history_display();

    // Bouton "Enregistrer"
    lv_obj_t * btn_save = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_save, save_distance_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -10, -50);
    lv_obj_t * label_save = lv_label_create(btn_save);
    lv_label_set_text(label_save, "Enregistrer");
    lv_obj_center(label_save);

    // Bouton "Supprimer" l'historique
    lv_obj_t * btn_clear = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_clear, clear_history_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_clear, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t * label_clear = lv_label_create(btn_clear);
    lv_label_set_text(label_clear, "Supprimer");
    lv_obj_center(label_clear);

    // Boutons pour val1 et val2 (pour calculer la surface)
    lv_obj_t * btn_valeur1 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_valeur1, valeur1, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_valeur1, LV_ALIGN_CENTER, -10, 60);
    lv_obj_t * label_val = lv_label_create(btn_valeur1);
    lv_label_set_text(label_val, "valeur 1");
    lv_obj_center(label_val);

    lv_obj_t * btn_valeur2 = lv_button_create(lv_scr_act());
    lv_obj_add_event_cb(btn_valeur2, valeur2, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_valeur2, LV_ALIGN_CENTER, -10, 100);
    lv_obj_t * label_val2 = lv_label_create(btn_valeur2);
    lv_label_set_text(label_val2, "valeur 2");
    lv_obj_center(label_val2);

    // Affichage de la surface calculée
    label_surface = lv_label_create(lv_scr_act());
    lv_label_set_text(label_surface,"Surface : ---");
    lv_obj_align(label_surface, LV_ALIGN_CENTER, 0, -100);

    // Affichage des valeurs 1 et 2
    label_valeur1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_valeur1,"Valeur1 : ---");
    lv_obj_align(label_valeur1, LV_ALIGN_CENTER, -150, -70);

    label_valeur2 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_valeur2,"Valeur2 : ---");
    lv_obj_align(label_valeur2, LV_ALIGN_CENTER, -150, -100);
}

#ifdef ARDUINO

#include "lvglDrivers.h"

// Setup Arduino
void mySetup()
{
    Serial.begin(115200);
    Wire.begin();      // Initialisation I2C
    testLvgl();        // Création de l'interface
}

void loop() {
    // Vide ici, boucle principale gérée ailleurs ou par LVGL
}

void myTask(void *pvParameters)
{
    // Tâche périodique (non utilisée ici)
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

// Main pour simulateur PC
int main(void)
{
    printf("LVGL Simulator\n");
    fflush(stdout);

    lv_init();      // Initialisation LVGL
    hal_setup();    // Initialisation matérielle simulée

    testLvgl();     // Création de l'interface

    hal_loop();     // Boucle principale
    return 0;
}

#endif
