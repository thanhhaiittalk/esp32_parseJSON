					/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "cJSON.h"

void sd_card_init();
int check_update(const char * const monitor);

void sd_card_init()
{
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
	// Internal pull-ups are not sufficient. However, enabling internal pull-ups
	// does make a difference some boards, so we do that here.

	gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
	gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
	gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
	gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
	gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	        .format_if_mount_failed = false,
	        .max_files = 5,
	        .allocation_unit_size = 16 * 1024
	    };
	// Use settings defined above to initialize SD card and mount FAT filesystem.
	    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
	    // Please check its source code and implement error recovery when developing
	    // production applications.
	    sdmmc_card_t* card;
	    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

	    if (ret != ESP_OK) {
	        if (ret == ESP_FAIL) {
	            printf( "Failed to mount filesystem.\n");
	        } else {
	            printf("Failed to initialize the card (%d).\n",ret);
	        }
	    }
	    // Card has been initialized, print its properties
	    sdmmc_card_print_info(stdout, card);
}

int check_update(const char * const json_string)
{
    const cJSON *update = NULL;
    const cJSON *artifacts = NULL;

    int status = 0;
    cJSON *muse_json = cJSON_Parse(json_string);
    if (muse_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        status = 0;
        goto end;
    }

    update = cJSON_GetObjectItemCaseSensitive(muse_json, "update");
    if (cJSON_IsString(update) && (update->valuestring != NULL))
    {
        printf("Checking update \"%s\"\n", update->valuestring);
    }

//
end:
    cJSON_Delete(muse_json);
    return status;
}

void app_main(void)
{
	char * json_string;
	char line[64];
	ESP_ERROR_CHECK(nvs_flash_init());
	sd_card_init();
	printf("sdcard\n");
	FILE * json_file = fopen("/sdcard/hcm.txt","r");
	if (json_file==NULL){
			printf("Failed to open file for reading\n");
	}
	fseek(json_file,0L,SEEK_END);
	long size = ftell(json_file);
	printf("size: %ld \n",size);
	fseek(json_file,0L,SEEK_SET);
	json_string =(char*) calloc(size,sizeof(char*));
	while(fgets(line,sizeof(line),json_file)!=NULL){
		//printf("%s == ",line);
		strcat(json_string,line);
	}
	printf(json_string);
	int status = check_update(json_string);
	printf("status = %d\n",status);
	free(json_string);
}
