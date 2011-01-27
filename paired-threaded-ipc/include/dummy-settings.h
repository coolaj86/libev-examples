#ifndef DUMMY_SETTINGS_STRUCT_H
#define DUMMY_SETTINGS_STRUCT_H

#define DUMMYD_SOCK "/tmp/dummyd-%d.sock"

#pragma pack(1)
typedef struct
{  
  int  delay;     // how long (ms) to fake processing time
  int  variance;  // percentage of variance on fake processing time
  int  init_time;
} DUMMY_SETTINGS;
#pragma pack()

void dummy_settings_set_presets(DUMMY_SETTINGS*);
void dummy_settings_print(DUMMY_SETTINGS*);

#endif

