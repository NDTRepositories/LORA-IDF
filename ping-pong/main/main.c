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

#define tag "SSD1306"

static float symbol_error;
static float SAMPLES = 50;

static volatile Module_Data module_data;

#include <stdio.h>

// void main() {
//   char buff[100];
//   int a, b;

//   sprintf(buff, "a = %d, b = %d", 10, 20);

//   sscanf(buff, "a = %d, b = %d", &a, &b);

//   printf("a = %d, b = %d\n", a, b);  // distributes a = 10, b = 20
// }

#if CONFIG_PRIMARY

#define TIMEOUT 100
void task_primary(void *pvParameters)
// void task_primary()
{   
	while(1)
{   
	Data_Bkres data_bkres;
    data_bkres.Temp_Data = 1;
	data_bkres.Do_Data = 2;
	data_bkres.Ph_Data = 3;
	data_bkres.EC_Data = 4;
	data_bkres.time_stamp = 5;

	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) 
	{
		int send_len = sprintf((char *)buf,"TEMP = %.2lf, DO = %.2lf, PH = %.2lf, EC = %.2lf, TIME =%.2lf ",data_bkres.Temp_Data,data_bkres.Do_Data,data_bkres.Ph_Data,data_bkres.EC_Data,data_bkres.time_stamp);

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
					symbol_error = (float)lora_packet_lost() /  SAMPLES;
					if (symbol_error  > 1)
					{
						symbol_error = 0;
					}
				    module_data.rssi= lora_packet_rssi();
					module_data.count_lost_packet=lora_packet_lost();
					module_data.SER=symbol_error;
					module_data.snr=lora_packet_snr();
					
					xSemaphoreGive(mutex);
				ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
				ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
				ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
				ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
				ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
				ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" MillSecs", diffTick * portTICK_PERIOD_MS);
				waiting = false;
			}
			TickType_t currentTick = xTaskGetTickCount();
			TickType_t diffTick = currentTick - startTick;
			ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);
			if (diffTick > TIMEOUT)
			 {
				ESP_LOGW(pcTaskGetName(NULL), "Response timeout");
				waiting = false;
			}
			vTaskDelay(1); // Avoid WatchDog alerts
		} // end waiting
		vTaskDelay(pdMS_TO_TICKS(5000));
	} // end while
	}
}
#endif // CONFIG_PRIMARY



#if CONFIG_SECONDARY
void task_secondary(void *pvParameters)
// void task_secondary()
{  
	
	while(1)
	{
 	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) 
	{
		lora_receive(); // put into receive mode
		if(lora_received()) 
		{
        int rxLen = lora_receive_packet(buf, sizeof(buf));
					
					symbol_error = (float)lora_packet_lost() / SAMPLES;
                    if (symbol_error  > 1)
					{
						symbol_error = 0;
					}
					module_data.rssi = lora_packet_rssi();
					module_data.count_lost_packet = lora_packet_lost();
					module_data.SER = symbol_error;
					module_data.snr = lora_packet_snr(); 

			xSemaphoreGive(mutex);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
			ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
			ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbm", lora_packet_rssi());
			ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
			ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
			vTaskDelay(1);
			lora_send_packet(buf, rxLen);
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", rxLen);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	  }
	}
}
	
#endif // CONFIG_SECONDARY

void task_oled(void *pvParameters)
{ 

   while (1){

   
  if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
  {         

		    ESP_LOGI(pcTaskGetName(NULL), "Start");
    		char str[19];
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
  taskYIELD();
}

  
}
void app_main()
{  
    
   SSD1306_Init();
   mutex = xSemaphoreCreateBinary();

	if (lora_init() == 0)
	{
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");


				vTaskDelay(1);

	}


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

     
	xTaskCreatePinnedToCore(&task_primary, "PRIMARY", 1024*16, NULL,tskIDLE_PRIORITY + 2,NULL,0);
		// void task_primary();
#endif

#if CONFIG_SECONDARY

	// task_secondary();

	xTaskCreatePinnedToCore(&task_secondary, "SECONDARY", 1024*16, NULL,tskIDLE_PRIORITY + 2,NULL,0);
#endif

    xTaskCreatePinnedToCore(&task_oled, "OLED", 1024*2, NULL, tskIDLE_PRIORITY + 1, NULL,1);

  
}