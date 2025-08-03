#ifndef PROCS_H
#define PROCS_H

#include <lvgl.h>

void restore_active_tab();
void save_active_tab(int idx);
void TabWissel(lv_event_t * e);
void update_labels(float aanvoer, float afvoer, float temperature, float pressure); // Nieuwe procedure voor label updates

#endif // PROCS_H
