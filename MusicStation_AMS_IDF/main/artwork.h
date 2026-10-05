#ifndef ARTWORK_H
#define ARTWORK_H

#include "esp_err.h"

esp_err_t artwork_init(void);

esp_err_t artwork_search(
    const char *artist,
    const char *title
);

void artwork_update(
    const char *artist,
    const char *title
);

#endif