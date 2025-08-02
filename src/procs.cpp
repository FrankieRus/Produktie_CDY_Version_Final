#include "procs.h"
#include "../UI/ui.h"
#include <Preferences.h>

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


