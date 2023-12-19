#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

typedef struct
{
    int rssi;
    float snr;
    int count_lost_packet;
    float SER;   //symbol error
}Module_Data;

typedef struct
{ 
   float Temp_Data;
   float Do_Data;
   float Ph_Data;
   float EC_Data;
   double time_stamp;
}Data_Bkres;

#endif