/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "time.h"
#include "sys/time.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "lora.h"
#include "ssd1306.h"
#include "font.h"
#include "data.h"

SemaphoreHandle_t mutex;


#if CONFIG_DS1307
#include "ds1307.h"
#endif

#if CONFIG_DS3231
#include "ds3231.h"
#endif


#define tag "SSD1306"

static int count_lost_packet;
static float symbol_error;
float SAMPLES = 30;


volatile char TIME_STAMP[50];

static volatile Module_Data module_data;



#if CONFIG_DS1307


void ds1307(void *pvParameters)
{

			i2c_dev_t dev;
			memset(&dev, 0, sizeof(i2c_dev_t));

			ESP_ERROR_CHECK(ds1307_init_desc(&dev, 0, CONFIG_DS1307_I2C_SDA, CONFIG_DS1307_I2C_SCL));

			// setup datetime: 2018-04-11 00:52:10
					struct tm time =
					{
						.tm_year = 118, //since 1900 (2018 - 1900)
						.tm_mon  = 3,  // 0-based
						.tm_mday = 11,
						.tm_hour = 0,
						.tm_min  = 52,
						.tm_sec  = 10
					};
			ESP_ERROR_CHECK(ds1307_set_time(&dev, &time));


			ds1307_get_time(&dev, &time);



			printf("%04d-%02d-%02d %02d:%02d:%02d\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
						time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);


                 	vTaskDelay(pdMS_TO_TICKS(500));

         

		vTaskDelete(NULL);

}
#endif



#if CONFIG_DS3231

void task_ds3231(void *pvParameters)
{
    while(1){

	 if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, CONFIG_DS3231_I2C_SDA, CONFIG_DS3231_I2C_SCL));

    // setup datetime: 2016-10-09 13:50:10
    struct tm time = {
        .tm_year = 116, //since 1900 (2016 - 1900)
        .tm_mon  = 9,  // 0-based
        .tm_mday = 9,
        .tm_hour = 13,
        .tm_min  = 50,
        .tm_sec  = 10
    };
    ESP_ERROR_CHECK(ds3231_set_time(&dev, &time));

    while (1)
    {
        float temp;

        vTaskDelay(pdMS_TO_TICKS(250));

        if (ds3231_get_temp_float(&dev, &temp) != ESP_OK)
        {
            printf("Could not get temperature\n");
            continue;
        }

        if (ds3231_get_time(&dev, &time) != ESP_OK)
        {
            printf("Could not get time\n");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        printf("%04d-%02d-%02d %02d:%02d:%02d, %.2f deg Cel\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
            time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, temp);
    }


	}

	}

}



#endif



#if CONFIG_PRIMARY

#define TIMEOUT 100
void task_primary(void *pvParameters)
// void task_primary()
{   
	while(1)
{
    if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
	
	{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) 
	{
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf,"Hello World!! %"PRIu32, nowTick);

#if 0
		// Maximum Payload size of SX1276/77/78/79 is 255
		memset(&buf[send_len], 0x20, 255-send_len);
		send_len = 255;
#endif
		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent:[%.*s]", send_len, send_len, buf);
		bool waiting = true;
		TickType_t startTick = xTaskGetTickCount();
		while(waiting) {
			lora_receive(); // put into receive mode
			if(lora_received()) {
				int rxLen = lora_receive_packet(buf, sizeof(buf));
				TickType_t currentTick = xTaskGetTickCount();
				TickType_t diffTick = currentTick - startTick;
					if(lora_packet_lost()  >  30 )
					{
						
						count_lost_packet++;

					}
					symbol_error = (float)lora_packet_lost() /  SAMPLES;
					if (symbol_error  > 1)
					{
						symbol_error = 0;
					}
				    module_data.rssi= lora_packet_rssi();
					module_data.count_lost_packet=count_lost_packet;
					module_data.SER=symbol_error;
					module_data.snr=lora_packet_snr();
					xSemaphoreGive(mutex);
				ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
				ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
				ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
				ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
				ESP_LOGI(pcTaskGetName(NULL), "count lost packet sample :%d", count_lost_packet);
				ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
				ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" MillSecs", diffTick * portTICK_PERIOD_MS);
				waiting = false;
			}
			TickType_t currentTick = xTaskGetTickCount();
			TickType_t diffTick = currentTick - startTick;
			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);
			if (diffTick > TIMEOUT) {
				ESP_LOGW(pcTaskGetName(NULL), "Response timeout");
				waiting = false;
			}
			vTaskDelay(1); // Avoid WatchDog alerts
		} // end waiting
		vTaskDelay(pdMS_TO_TICKS(5000));
	} // end while
	}
}
}
#endif // CONFIG_PRIMARY



#if CONFIG_SECONDARY
void task_secondary(void *pvParameters)
// void task_secondary()
{  
	
	while(1)
	 {
    if(xSemaphoreTake(mutex, portMAX_DELAY) == pdPASS)
	{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if(lora_received()) {
        int rxLen = lora_receive_packet(buf, sizeof(buf));
					
						
						if (lora_packet_lost() > 30)
						{
						   count_lost_packet ++;
						}
							
						
					symbol_error = (float)lora_packet_lost() / SAMPLES;
                    if (symbol_error  > 1)
					{
						symbol_error = 0;
					}
					module_data.rssi = lora_packet_rssi();
					module_data.count_lost_packet = count_lost_packet;
					module_data.SER = symbol_error;
					module_data.snr = lora_packet_snr(); 
			xSemaphoreGive(mutex);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
			ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
			ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
			ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
			ESP_LOGI(pcTaskGetName(NULL), "count lost packet sample :%d", count_lost_packet);
			ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
			for (int i=0;i<rxLen;i++) {
				if (isupper(buf[i])) {
					buf[i] = tolower(buf[i]);
				} else {
					buf[i] = toupper(buf[i]);
				}
			}
			vTaskDelay(1);
			lora_send_packet(buf, rxLen);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", rxLen);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	}
	}
}
	
}
#endif // CONFIG_SECONDARY

void task_oled(void *pvParameters)
{ 

   
  if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
  {         

		    ESP_LOGI(pcTaskGetName(NULL), "Start");
    		char str[15];
			SSD1306_Clear();
			SSD1306_GotoXY(0, 0);
			sprintf(str, "RSSI:%.2d", module_data.rssi);
			SSD1306_Puts(str, &Font_7x10, 1);
			SSD1306_GotoXY(0, 10);
			sprintf(str, "LOST :%.2d", module_data.count_lost_packet);
			SSD1306_Puts(str, &Font_7x10, 1);
			SSD1306_GotoXY(0, 20);
			sprintf(str, "SER :%.2f", module_data.SER);
			SSD1306_Puts(str, &Font_7x10, 1);
			SSD1306_GotoXY(0, 30);
			sprintf(str, "SNR :%.2f", module_data.snr);
			SSD1306_Puts(str, &Font_7x10, 1);
			SSD1306_GotoXY(0, 40);
			SSD1306_UpdateScreen();

  }


    vTaskDelete(NULL);
}
void app_main()
{  
    
   SSD1306_Init();
   mutex = xSemaphoreCreateMutex();

	if (lora_init() == 0)
	{
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");


				vTaskDelay(1);

	}
    // Get current time
    time_t now;
    time(&now);
    printf("Current time: %s", ctime(&now));
    
    // Get current system time in microseconds
    uint64_t current_time_us = esp_timer_get_time();
    printf("Current system time (microseconds): %lld\n", current_time_us);
    
    // Delay for 1 second using vTaskDelay
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Get current system time using gettimeofday
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("Current system time (seconds): %lld\n", tv.tv_sec);
    
    // Get the amount of free heap memory
    size_t free_heap = esp_get_free_heap_size();
    printf("Free heap memory: %d bytes\n", free_heap);
    
    // Restart the ESP32 system


#if CONFIG_169MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
	lora_set_frequency(169e6); // 169MHz
#elif CONFIG_433MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
	lora_set_frequency(433e6); // 433MHz
#elif CONFIG_470MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
	lora_set_frequency(470e6); // 470MHz
#elif CONFIG_866MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
	lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %d MHz", CONFIG_OTHER_FREQUENCY);
	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
	lora_set_frequency(frequency);
#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 7;

#if CONFIF_EXTENDED
	cr = CONFIG_CODING_RATE
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

  

#if CONFIG_PRIMARY

     
	xTaskCreatePinnedToCore(&task_primary, "PRIMARY", 1024*16, NULL, 15, NULL,	0);
		// void task_primary();
#endif

#if CONFIG_SECONDARY

	// task_secondary();

	xTaskCreatePinnedToCore(&task_secondary, "SECONDARY", 1024*16, NULL, 15, NULL,	0);
#endif

    xTaskCreatePinnedToCore(&task_oled, "OLED", 1024*16, NULL, 10, NULL,1);

  
}
