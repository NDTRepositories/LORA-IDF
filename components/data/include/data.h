#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

typedef struct
{
    int rssi;
    int snr;
    int count_lost_packet;
    float SER;   //symbol error
}Module_Data;

#endif