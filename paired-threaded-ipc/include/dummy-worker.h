#ifndef _DUMMY_WORKER_H_
#define _DUMMY_WORKER_H_

#include "dummy-settings.h"

int
worker_init(DUMMY_SETTINGS* new_settings);

void
worker_run(void* data);

void
worker_clean();

void
worker_set(DUMMY_SETTINGS* new_settings);

#endif
