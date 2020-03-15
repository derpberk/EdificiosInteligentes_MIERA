#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"



#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_system.h"


#define GPIO_LED   2
#define GPIO_LED_PIN_SEL  1ULL<<GPIO_LED 
#define ALARM_ON gpio_set_level(GPIO_LED, 0);  printf("alarma encendida \r\n");
#define ALARM_OFF gpio_set_level(GPIO_LED, 1); printf("alarma apagada \r\n");

static const char *TAG = "MQTT_EXAMPLE";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/* Global variables */
int mode=0;
int alarm_state=0;
int alarm_flag=0;

void blink_alarm(int rep, int ms){
	int i;
	for (i=0; i<rep; i++){
		gpio_set_level(GPIO_LED, 0);
		vTaskDelay(ms / portTICK_RATE_MS);
		gpio_set_level(GPIO_LED, 1);
		vTaskDelay(ms / portTICK_RATE_MS);
	}
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            //msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            //msg_id = esp_mqtt_client_subscribe(client, "/alarma", 0);
            //ESP_LOGI(TAG, "Suscrito al topic alarma");

            //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "0000000004/buzzer/state", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "0000000004/buzzer/mode", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
	    const char* topic_alarma = "0000000004/buzzer/state";
	    int res= strncmp(topic_alarma, event->topic,23);
	    if (res==0){
		    printf("Se ha recibido un mensaje del topic state \r\n");
		    char dato = *(event->data);
		    if ((int)dato == 48) {
			printf("Se ha recibido un 0 \r\n");
			alarm_flag=0;
			alarm_state=0;
		    }
		    if ((int)dato == 49) {
			printf("Se ha recibido un 1 \r\n");
			alarm_flag=1;
			alarm_state=1;
		    }

	    }
	    const char* topic_modo = "0000000004/buzzer/mode";
	    res= strncmp(topic_modo, event->topic,22);
	    if (res==0){
		    printf("Se ha recibido un mensaje del topic mode \r\n");
		    char dato = *(event->data);
		    if ((int)dato == 48) {
			printf("Se ha recibido un 0 \r\n");
			mode=0;
		    }
		    if ((int)dato == 49) {
			printf("Se ha recibido un 1 \r\n");
			mode=1;
		    }

	    }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    return client;
}

void * thread_func(void * p)
{
    printf("In thread_func\n");
    return NULL;
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    /** ConfiguraciÃn del pin de salidad **/
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    blink_alarm(8,200);

    nvs_flash_init();
    wifi_init();
    esp_mqtt_client_handle_t client = mqtt_app_start();


    printf("Se ha pasado la sentencia de comienzo de mqtt\r\n");
    vTaskDelay(2000 / portTICK_RATE_MS); // Waiting for broker connection
    int contador=0;
    int contador2=0;
    ALARM_ON
    while(1){
            if (mode==0){
		    /** In this mode, the alarm blink continiusly until 0 is recieved **/
		    if (alarm_state==1){
			    if (contador%2 == 0){
				    ALARM_ON
			    }
			    else {
				    ALARM_OFF
			    }
		    }
		    else {
			    ALARM_OFF
		    } 
            }
            else if(mode==1){
		    /** In this mode, the alarm blink only 4 times   **/
		    if (alarm_flag==1){
			    contador2=0;
			    alarm_flag=0;
		    }
		    if (contador2<8){
			    if (contador2%2 == 0){
				    ALARM_ON
			    }
			    else {
				    ALARM_OFF
			    }
			    contador2++;
		    }
		    else {
			    ALARM_OFF
		    } 
            }

	    if (contador == 7){
		    contador=0;
		    printf("Se va a enviar un msg a public_topic\r\n");
		    esp_mqtt_client_publish(client, "public_topic", "0000000004_buzzer_mode_state", 10, 1, 0);
	    }
	    else {
		    contador++;
	    }

            vTaskDelay(125 / portTICK_RATE_MS); // Waiting for broker connection
    }
}
