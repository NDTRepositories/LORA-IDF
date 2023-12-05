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
#include "freertos/semphr.h"
#include "esp_log.h"

#if CONFIG_DS1307
#include "ds1307.h"
#endif

#if CONFIG_DS3231
#include "ds3231.h"
#endif

#include "lora.h"
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "data.h"

#define tag "SSD1306"

static int count_lost_packet;
static float symbol_error;
float SAMPLES = 30;


volatile char TIME_STAMP[50];
volatile Module_Data module_data;

SemaphoreHandle_t dataMutex;


#if CONFIG_DS1307


void ds1307(void *pvParameters)
{
        if (xSemaphoreTake(dataMutex, portMAX_DELAY))
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

			while (1)
				{
					ds1307_get_time(&dev, &time);

                    xSemaphoreGive(dataMutex);

					printf("%04d-%02d-%02d %02d:%02d:%02d\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
						time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);


                 	vTaskDelay(pdMS_TO_TICKS(500));
				}
        }

}
#endif



#if CONFIG_DS3231

void ds3231(void *pvParameters)
{

        if (xSemaphoreTake(dataMutex, portMAX_DELAY))
        {
        i2c_dev_t dev;
        memset(&dev, 0, sizeof(i2c_dev_t));

        ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, CONFIG_DS3231_I2C_SDA, CONFIG_DS3231_I2C_SCL));

            // setup datetime: 2016-10-09 13:50:10
               struct tm time =
				{
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


            xSemaphoreGive(dataMutex);

			printf("%04d-%02d-%02d %02d:%02d:%02d, %.2f deg Cel\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
							time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, temp);

				}

        }
}
#endif



#if CONFIG_PRIMARY

#define TIMEOUT 100


void task_primary(void *pvParameters)
{

        if (xSemaphoreTake(dataMutex, portMAX_DELAY))
        {


		ESP_LOGI(pcTaskGetName(NULL), "Start");
		uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
		while(1) {
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
			count_lost_packet =0;
			symbol_error = 0 ;
			TickType_t startTick = xTaskGetTickCount();
			while(waiting) {
				lora_receive(); // put into receive mode
				if(lora_received())
				{
					int rxLen = lora_receive_packet(buf, sizeof(buf));
					TickType_t currentTick = xTaskGetTickCount();
					TickType_t diffTick = currentTick - startTick;


					if(lora_packet_lost()  >=  1 )
					{
						if (count_lost_packet > 30)
						{
							count_lost_packet = 1;
						}

						count_lost_packet++;

					}

					symbol_error = count_lost_packet /  SAMPLES;

				    module_data.rssi= lora_packet_rssi();
					module_data.count_lost_packet=count_lost_packet;
					module_data.SER=symbol_error;
					module_data.snr=lora_packet_snr();


					ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
					ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
					ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbw", lora_packet_rssi());
					ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
					ESP_LOGI(pcTaskGetName(NULL), "count lost packet :%d", count_lost_packet);
					ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
					ESP_LOGI(pcTaskGetName(NULL), "Response time is %"PRIu32" MillSecs", diffTick * portTICK_PERIOD_MS);

					printf(" \n ");
					waiting = false;
				}
				TickType_t currentTick = xTaskGetTickCount();
				TickType_t diffTick = currentTick - startTick;
				ESP_LOGD(pcTaskGetName(NULL), "diffTick=%"PRIu32, diffTick);


                xSemaphoreGive(dataMutex);

				if (diffTick > TIMEOUT) {
					ESP_LOGW(pcTaskGetName(NULL), "Response timeout");
					printf(" \n ");
					waiting = false;
				}



				vTaskDelay(1); // Avoid WatchDog alerts
			} // end waiting
			vTaskDelay(pdMS_TO_TICKS(5000));
        }


	} // end while



}
#endif // CONFIG_PRIMARY



#if CONFIG_SECONDARY
void task_secondary(void *pvParameters)
{

        if (xSemaphoreTake(dataMutex, portMAX_DELAY))
        {
			ESP_LOGI(pcTaskGetName(NULL), "Start");
			uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
			while(1) {
				lora_receive(); // put into receive mode
				if(lora_received())
				{


						if(lora_packet_lost()  >=  1 )
						{
							if (count_lost_packet > 30)
							{
								count_lost_packet = 1;
							}

							count_lost_packet++;

						}

						symbol_error = count_lost_packet / SAMPLES;

					int rxLen = lora_receive_packet(buf, sizeof(buf));

					module_data.rssi = lora_packet_rssi();
					module_data.count_lost_packet = count_lost_packet;
					module_data.SER = symbol_error;
					module_data.snr = lora_packet_snr();



					ESP_LOGI(pcTaskGetName(NULL), "lora packet snr :%.2lf dbm", lora_packet_snr());
					ESP_LOGI(pcTaskGetName(NULL), "lora packet rssi :%d dbw", lora_packet_rssi());
					ESP_LOGI(pcTaskGetName(NULL), "lora packet lost  :%d", lora_packet_lost());
					ESP_LOGI(pcTaskGetName(NULL), "count lost packet :%d", count_lost_packet);
					ESP_LOGI(pcTaskGetName(NULL), "symbol_error : %.2lf ", symbol_error);
					ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);


					printf("\n ");
					for (int i=0;i<rxLen;i++)
					{
						if (isupper(buf[i])) {
							buf[i] = tolower(buf[i]);
						} else {
							buf[i] = toupper(buf[i]);
						}
					}
					vTaskDelay(1);
					lora_send_packet(buf, rxLen);
                    xSemaphoreGive(dataMutex);
					ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent back...", rxLen);
					printf("\n");
				}
				vTaskDelay(1); // Avoid WatchDog alerts
			}

        }
}


#endif // CONFIG_SECONDARY


void oled_display()
{
     if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
 	SSD1306_t dev;
	int center, top, bottom;
	// char lineChar[20];

	ESP_ERROR_CHECK(i2cdev_init());


#if CONFIG_I2C_INTERFACE
	ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
	ESP_LOGI(tag, "INTERFACE is SPI");
	ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
	ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
	ESP_LOGI(tag, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
	ESP_LOGI(tag, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

#if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(tag, "Flip upside down");
#endif

#if CONFIG_SSD1306_128x64
	ESP_LOGI(tag, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
#endif // CONFIG_SSD1306_128x64
#if CONFIG_SSD1306_128x32
	ESP_LOGI(tag, "Panel is 128x32");
	ssd1306_init(&dev, 128, 32);
#endif // CONFIG_SSD1306_128x32

	// ssd1306_clear_screen(&dev, false);
	// ssd1306_contrast(&dev, 0xff);
	// ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);
	// vTaskDelay(3000 / portTICK_PERIOD_MS);

#if CONFIG_SSD1306_128x64
	top = 2;
	center = 3;
	bottom = 8;
	ssd1306_display_text(&dev, 0, &module_data.rssi, 14, false);
	ssd1306_display_text(&dev, 1, &module_data.snr, 16, false);
	ssd1306_display_text(&dev, 2, &module_data.count_lost_packet,16, false);
	ssd1306_display_text(&dev, 3, &module_data.SER, 13, false);
	// ssd1306_display_text(&dev, 4, &TIME_STAMP, 14, false);
    xSemaphoreGive(dataMutex);
	ssd1306_display_text(&dev, 5, "ABCDEFGHIJKLMNOP", 16, true);
	ssd1306_display_text(&dev, 6, "abcdefghijklmnop",16, true);
	ssd1306_display_text(&dev, 7, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x64

#if CONFIG_SSD1306_128x32
	top = 1;
	center = 1;
	bottom = 4;
	ssd1306_display_text(&dev, 0, "SSD1306 128x32", 14, false);
	ssd1306_display_text(&dev, 1, "Hello World!!", 13, false);
	//ssd1306_clear_line(&dev, 2, true);
	//ssd1306_clear_line(&dev, 3, true);
	ssd1306_display_text(&dev, 2, "SSD1306 128x32", 14, true);
	ssd1306_display_text(&dev, 3, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x32
	// Restart module
	esp_restart();

    }

}

void app_main()
{

dataMutex = xSemaphoreCreateMutex();

#if CONFIG_DS3231

    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(&ds3231, "ds3231", 1024, NULL, 5, NULL);

#endif


#if CONFIG_DS1307

   ESP_ERROR_CHECK(i2cdev_init());
   xTaskCreate(&ds1307, "ds1307",1024 , NULL, 5, NULL);

#endif

	if (lora_init() == 0)
	{
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
			while(1)
			{
				vTaskDelay(1);
			}
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

    xTaskCreate(&task_primary, "PRIMARY", 2048, NULL,7, NULL);
	//	xTaskCreate(&task_primary, "PRIMARY", 1024*3, NULL, 5, NULL);
#endif

#if CONFIG_SECONDARY

	xTaskCreate(&task_secondary, "SECONDARY", 2048, NULL, 7, NULL);

	//	xTaskCreate(&task_secondary, "SECONDARY", 1024*3, NULL, 5, NULL);
#endif

    // xTaskCreate(&oled_display, "oled_display", 2048, NULL, 5, NULL );


}
