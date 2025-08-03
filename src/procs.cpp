#include "procs.h"
#include "../UI/ui.h"
#include <Preferences.h>
#include "MyWebServer.h"

Preferences tabPrefs;

void restore_active_tab()
{
    tabPrefs.begin("tabmem", true); // read-only
    int tabidx = tabPrefs.getInt("tabidx", 0);
    tabPrefs.end();
    // Zet focus naar opgeslagen tab
    if (ui_TabView1) {
        lv_tabview_set_act(ui_TabView1, tabidx, LV_ANIM_OFF);
    }
}

void save_active_tab(int idx)
{
    tabPrefs.begin("tabmem", false);
    tabPrefs.putInt("tabidx", idx);
    tabPrefs.end();
    Serial.println("Saved active tab: " + String(idx));
    
}

void TabWissel(lv_event_t * e)
{
    static int last_saved_idx = -1;
    if (lv_event_get_code(e) == 33) {
        int idx = lv_tabview_get_tab_act(ui_TabView1);
        if (idx != last_saved_idx) {
            save_active_tab(idx);
            Serial.print("TabChangeProc: tab opgeslagen: ");
            Serial.println(idx);
            last_saved_idx = idx;
        }
    }
}

void update_labels(float aanvoer, float afvoer, float temperature, float pressure) {
    lv_label_set_text(ui_Aanvoer, (String("WP Aanvoer: ") + String(aanvoer,2)).c_str());
    lv_label_set_text(ui_Afvoer, (String("WP Afvoer: ") + String(afvoer,2)).c_str());
    lv_label_set_text(ui_Woonkamer, (String("Woonkamer: ") + String(temperature,2)).c_str());
    lv_label_set_text(ui_Luchtdruk, (String("Luchtdruk: ") + String(pressure)).c_str());
    lv_label_set_text(ui_Tijdstempel, getCurrentTime().c_str());
}


