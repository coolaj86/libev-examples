#include <stdio.h> // printf

#include "dummy-settings.h"

void dummy_settings_set_presets(DUMMY_SETTINGS* dummy_settings)
{
  //set ALL the default values
  dummy_settings->delay = 100;
  dummy_settings->variance = 10;
  dummy_settings->init_time = 10;
}

void dummy_settings_print(DUMMY_SETTINGS* dummy_settings)
{
  printf("delay = %dms\n", dummy_settings->delay);
  printf("variance = %d%%\n", dummy_settings->variance);
  printf("init_time = %d%%\n", dummy_settings->init_time);
}
