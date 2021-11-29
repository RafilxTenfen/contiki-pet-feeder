/**
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.

 *******************************************************************************
 * @file main_core.c
 * @author Ânderson Ignacio da Silva
 * @date 19 Ago 2016
 * @brief Main code to test MQTT-SN on Contiki-OS
 * @see http://www.aignacio.com
 * @license This project is licensed by APACHE 2.0.
 */

#include "contiki.h"
#include "lib/random.h"
#include "clock.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "mqtt_sn.h"
#include "dev/leds.h"
#include "net/rime/rime.h"
#include "net/ip/uip.h"
#include "sys/node-id.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct Config {
  int id;
  int dispensedTimes;
  int seccondsToDispense;
  int seccondsToDispenseDecrement;
  int gramsAvailable;
  time_t lastTimeDispensed;
  int configuredPortionGrams;
  int sizeGrams;
  char* animal;
} Config;

char* getJsonConfig(struct Config config) {
  char *postmsg = malloc (sizeof (char) * 10000);
  sprintf(postmsg, "{\n \"id\": \"%d\",\n \"dispensedTimes\": %d,\n \"gramsAvailable\": %d,\n \"lastTimeDispensed\": \"%ld\",\n \"configuredPortionGrams\": %d,\n \"sizeGrams\": %d,\n \"animal\": \"%s\"\n}", config.id,
    config.dispensedTimes, config.gramsAvailable, config.lastTimeDispensed,
    config.configuredPortionGrams, config.sizeGrams, config.animal);
  return (char *) postmsg;
};

char* getMessageConfig(struct Config config) {
  char *msg = malloc (sizeof (char) * 400);
  sprintf(msg, "%d,%d,%d,%ld,%d,%d,%s", config.id,
    config.dispensedTimes, config.gramsAvailable, config.lastTimeDispensed,
    config.configuredPortionGrams, config.sizeGrams, config.animal);
  return (char *) msg;
};

void sendCurl(struct Config config) {
  char *postthis = getJsonConfig(config);
  char *command = malloc (sizeof (char) * 10000);
  sprintf(command, "curl --header \"Content-Type: application/json\" \
                                                                             --request POST \
                                                                             --data '%s' \
                                                                             https://animalfeeder-api-2wmdhpbuoq-uc.a.run.app", postthis);
  debug_os("\ncurl command: %s", command);
  // system(command);
}

// void sendCurl(struct Config config) {
//   CURL *curl;
//   CURLcode res;
//   struct curl_slist *slist1;

//   /* In windows, this will init the winsock stuff */
//   curl_global_init(CURL_GLOBAL_ALL);

//   slist1 = NULL;
//   slist1 = curl_slist_append(slist1, "Content-Type: application/json");
//   curl = curl_easy_init();
//   if (curl) {
//     char *postthis = getJsonConfig(config);
//     printf("\n Sending POST: %s \n", postthis);
//     curl_easy_setopt(curl, CURLOPT_URL, "https://animalfeeder-api-2wmdhpbuoq-uc.a.run.app");
//     curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
//     curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strlen(postthis));
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
//     curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.74.0");
//     curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
//     curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

//     res = curl_easy_perform(curl);
//     if (res != CURLE_OK) {
//       fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//     }

//     curl_easy_cleanup(curl);
//     curl_slist_free_all(slist1);
//   }
// }

static uint16_t udp_port = 1884;
static uint16_t keep_alive = 5;
static uint16_t broker_address[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};
static struct   etimer periodic_timer;
// static uint16_t tick_process = 0;
static char     pub_test[20];
static char     device_id[17];
static char     topic_hw[25];
static char     *topics_mqtt[] = {"/config",
                                  "/dispensar"};
// static char     *will_topic = "/6lowpan_node/offline";
// static char     *will_message = "O dispositivo esta offline";
// This topics will run so much faster than others

mqtt_sn_con_t mqtt_sn_connection;

#define numberOfConfigs 3


void* createConfig() {
  struct Config* configs;
  configs = malloc(sizeof(struct Config) * numberOfConfigs);
  long now = 1638116931;
  struct Config dog = {
    id: 3,
    dispensedTimes: 0,
    seccondsToDispense: 4,
    seccondsToDispenseDecrement: 4,
    gramsAvailable: 2500,
    lastTimeDispensed: now,
    configuredPortionGrams: 400,
    sizeGrams: 3000,
    animal: "Dog",
  };
  configs[0] = dog;

  struct Config cat = {
    id: 4,
    dispensedTimes: 0,
    seccondsToDispense: 10,
    seccondsToDispenseDecrement: 10,
    gramsAvailable: 1500,
    lastTimeDispensed: now,
    configuredPortionGrams: 250,
    sizeGrams: 3000,
    animal: "Cat",
  };
  configs[1] = cat;

  struct Config cow = {
    id: 5,
    dispensedTimes: 0,
    seccondsToDispense: 15,
    seccondsToDispenseDecrement: 15,
    gramsAvailable: 6000,
    lastTimeDispensed: now,
    configuredPortionGrams: 800,
    sizeGrams: 10000,
    animal: "Cow",
  };
  configs[2] = cow;
  return configs;
}

Config* configs;

void mqtt_sn_callback(char *topic, char *message){
  printf("\nMessage received:");
  printf("\nTopic:%s Message:%s",topic,message);
}

void init_broker(void) {
  debug_os("Initializing init_broker the MQTT_SN_DEMO");

  char *all_topics[ss(topics_mqtt)+1];
  sprintf(device_id,"%02X%02X%02X%02X%02X%02X%02X%02X",
          linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1],
          linkaddr_node_addr.u8[2],linkaddr_node_addr.u8[3],
          linkaddr_node_addr.u8[4],linkaddr_node_addr.u8[5],
          linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  sprintf(topic_hw,"Hello addr:%02X%02X",linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);

  mqtt_sn_connection.client_id     = device_id;
  mqtt_sn_connection.udp_port      = udp_port;
  mqtt_sn_connection.ipv6_broker   = broker_address;
  mqtt_sn_connection.keep_alive    = keep_alive;
  //mqtt_sn_connection.will_topic    = will_topic;   // Configure as 0x00 if you don't want to use
  //mqtt_sn_connection.will_message  = will_message; // Configure as 0x00 if you don't want to use
  mqtt_sn_connection.will_topic    = 0x00;
  mqtt_sn_connection.will_message  = 0x00;

  mqtt_sn_init();   // Inicializa alocação de eventos e a principal PROCESS_THREAD do MQTT-SN

  size_t i;
  for(i=0;i<ss(topics_mqtt);i++)
    all_topics[i] = topics_mqtt[i];
  all_topics[i] = topic_hw;

  mqtt_sn_create_sck(mqtt_sn_connection,
                     all_topics,
                     ss(all_topics),
                     mqtt_sn_callback);

  mqtt_sn_sub(topic_hw, 0);
  mqtt_sn_sub("/config", 0);
  mqtt_sn_sub("/dispensar", 0);
}

/*---------------------------------------------------------------------------*/
PROCESS(init_system_process, "[Contiki-OS] Initializing Sync node");
AUTOSTART_PROCESSES(&init_system_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_system_process, ev, data) {
  PROCESS_BEGIN();

  debug_os("Initializing PROCESS_THREAD the MQTT_SN_DEMO");
  configs = createConfig();
  init_broker();
  int now = clock_seconds();
  debug_os("Node ID: %d, Device ID: %s, Secconds: %d", node_id, device_id, now);

  etimer_set(&periodic_timer, 10*CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  int configSend = 0;
  while(1) {
    PROCESS_WAIT_EVENT(etimer_expired(&periodic_timer));
    if (configSend == 0) {
      int j;
      for (j = 0; j < 3; j++) {
        Config currentConfig = configs[j];
        char *configMsg = getMessageConfig(currentConfig);
        debug_os("Sync send Config: %s", currentConfig.animal);
        // config.dispensedTimes, config.gramsAvailable, config.lastTimeDispensed,
        // config.configuredPortionGrams, config.sizeGrams, config.animal);
        mqtt_sn_pub("/config", configMsg, true, 0);
      }
      configSend += 1;
    }

    // sprintf(pub_test,"%s",topic_hw);
    // mqtt_sn_pub("/topic_1",pub_test,true,0);
    int i = 0;
    for (i = 0; i < 3; i++) {
      Config currentConfig = configs[i];
      if (currentConfig.seccondsToDispenseDecrement == 0) {
        if (currentConfig.gramsAvailable < currentConfig.configuredPortionGrams) {
          debug_os("\n\nConfig id: %d doesn't have sufficient grams available %d to configured portion %d",
            currentConfig.id, currentConfig.gramsAvailable, currentConfig.configuredPortionGrams);
          currentConfig.seccondsToDispenseDecrement = currentConfig.seccondsToDispense;
          configs[i] = currentConfig;
          continue;
        }
        currentConfig.dispensedTimes += 1;
        currentConfig.seccondsToDispenseDecrement = currentConfig.seccondsToDispense;
        currentConfig.lastTimeDispensed += currentConfig.seccondsToDispense;
        currentConfig.gramsAvailable -= currentConfig.configuredPortionGrams;
        configs[i] = currentConfig;
        sendCurl(currentConfig);
        char *dispenserMsg = getMessageConfig(currentConfig);
        debug_os("Sync send to dispense: %s", dispenserMsg);
        mqtt_sn_pub("/dispensar", dispenserMsg, true, 0);
        continue;
      }
      currentConfig.seccondsToDispenseDecrement -= 1;
      configs[i] = currentConfig;
    }

    etimer_set(&periodic_timer, CLOCK_SECOND * 1);
  }

  // while (1) {


  //   printf("\nPassed 1 second");
  //   sleep(1);

  // }
  PROCESS_END();
}
