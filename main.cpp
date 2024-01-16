#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// for wifi
#include <pico/cyw43_arch.h>
#include <lwip/apps/lwiperf.h>
#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <messages/utils.h>

#define TEMP 15
const uint MAX_TIMINGS = 85;
// qeuues
QueueHandle_t buffer;

static TaskHandle_t  wifiTask = NULL,readTask = NULL;

typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

void read_from_dht(dht_reading *result);

void init() {
    // for wifi
    printf("Start init WiFi\n");
    printf("Stop init WiFi\n");
}


static void readtemp(void *pvParameters) {
   
   dht_reading sample_buf[256];
   for (;;) {

        dht_reading reading;
        read_from_dht(&reading);
        float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
        for(int i=0;i<256;i++)
        sample_buf[i]=reading;
        xQueueSend(buffer,sample_buf,256);
        printf("Humidity = %.1f%%, Temperature = %.1fC\n",
               reading.humidity, reading.temp_celsius, fahrenheit);    

      vTaskDelay(1000);
      
   }
}

static void wifi(void *pvParameters) {
    printf("Start wifi task\n");
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return;
    }
    printf("Done init arch\n");

    //xTaskCreate(readtemp,"Read Temp",512,NULL,tskIDLE_PRIORITY,&readTask);

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect.\n");
        exit(1);
    }
    else
    {
        printf("Connected.\n");
    }
    
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    struct sockaddr_in listen_addr;
    bzero(&listen_addr, sizeof(listen_addr));
    listen_addr.sin_len = sizeof(struct sockaddr_in);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(8080);
    
    if (server_sock < 0)
    {
        printf("Unable to create socket: error %d", errno);
        return;
    }

    if (bind(server_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        printf("Unable to bind socket: error %d\n", errno);
        return;
    }

    if (listen(server_sock, 3) < 0)
    {
        printf("Unable to listen on socket: error %d\n", errno);
        return;
    }

    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), ntohs(listen_addr.sin_port));

    while (true)
    {
        struct sockaddr_storage remote_addr;
        socklen_t len = sizeof(remote_addr);
        printf("Wait conncection!\n");
        int conn_sock = accept(server_sock, (struct sockaddr *)&remote_addr, &len);
        printf("Got conncection!\n");
        dht_reading inter_buff[256];
        while (conn_sock>=0)
        {
            xQueueReceive(buffer,&inter_buff,portMAX_DELAY);
            send(conn_sock,&inter_buff,sizeof(dht_reading),0);
        }
        
    }
}

int main() {
    // start uart
    stdio_init_all();
    gpio_init(TEMP);
    sleep_ms(5000);
    printf("Begging free heap size %d\n", xPortGetFreeHeapSize());

    // init
    init();

    // create queues

    // create tasks
    xTaskCreate(wifi, "WiFi", 256, NULL, 31, &wifiTask);
   
    // create semaphore

    vTaskStartScheduler();

    return 0;
}

void read_from_dht(dht_reading *result) {
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(TEMP, GPIO_OUT);
    gpio_put(TEMP, 0);
    sleep_ms(20);
    gpio_set_dir(TEMP, GPIO_IN);

    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint count = 0;
        while (gpio_get(TEMP) == last) {
            count++;
            sleep_us(1);
            if (count == 255) break;
        }
        last = gpio_get(TEMP);
        if (count == 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (count > 16) data[j / 8] |= 1;
            j++;
        }
    }
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        result->humidity = (float) ((data[0] << 8) + data[1]) / 10;
        if (result->humidity > 100) {
            result->humidity = data[0];
        }
        result->temp_celsius = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (result->temp_celsius > 125) {
            result->temp_celsius = data[2];
        }
        if (data[2] & 0x80) {
            result->temp_celsius = -result->temp_celsius;
        }
    } 
}