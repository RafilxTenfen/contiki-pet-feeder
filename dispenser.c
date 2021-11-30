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
#include "dev/leds.h"

static uint16_t udp_port = 1884;
static uint16_t keep_alive = 5;
static uint16_t broker_address[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};
static struct   etimer time_poll;
// static uint16_t tick_process = 0;
static char     pub_test[20];
static char     device_id[17];
static char     topic_hw[25];
static char     *topics_mqtt[] = {"/config",
                                  "/dispensar",
                                  "/topic_1"};
// static char     *will_topic = "/6lowpan_node/offline";
// static char     *will_message = "O dispositivo esta offline";
// This topics will run so much faster than others

mqtt_sn_con_t mqtt_sn_connection;

#define numberOfConfigs 3


void mqtt_sn_callback(char *topic, char *message){
  printf("\nMessage received:");
  printf("\nTopic:%s Message:%s", topic, message);
  debug_os("DISPENSER ID: %d, Receive MSG: %s, Topic: %s", node_id, message, topic);

  leds_on(LEDS_ALL);

  if (strcmp(topic, "/config") != 0 && strcmp(topic, "/dispensar") != 0) {
    debug_os("IS NOT CONFIG OR DISPENSAR");
    return;
  }

  char * idStr = strtok(message, ",");
  printf("\nMSG conf ID: %s", idStr);
  int currentNodeId = (int) node_id;
  int id = atoi(idStr);

  if (id != currentNodeId) {
    return;
  }
  printf("\nIt is my ID: %d == %d", id, currentNodeId);

  if (strcmp(topic, "/config") != 0) {
    printf("\n\nReceived Conf %s", message);
    return;
  }
  if (strcmp(topic, "/dispensar") != 0) {
    printf("\n\nIt`s going to dispense food %s", message);
    debug_os("GO DISPENSER ID: %d, Receive MSG: %s, Topic: %s", node_id, message, topic);
    return;
  }
}

void init_broker(void) {
  char *all_topics[ss(topics_mqtt)+1];
  sprintf(device_id,"%02X%02X%02X%02X%02X%02X%02X%02X",
          linkaddr_node_addr.u8[0],linkaddr_node_addr.u8[1],
          linkaddr_node_addr.u8[2],linkaddr_node_addr.u8[3],
          linkaddr_node_addr.u8[4],linkaddr_node_addr.u8[5],
          linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  sprintf(topic_hw,"Hello addr:%02X%02X",linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  debug_os("DISPENSER Initializing init_broker the MQTT_SN_DEMO, %s", topic_hw);

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

  // mqtt_sn_sub(topic_hw, 0);
  mqtt_sn_sub_send("/config", 0);
  mqtt_sn_sub_send("/dispensar", 0);
}

/*---------------------------------------------------------------------------*/
PROCESS(init_system_process, "[Contiki-OS] Initializing OS");
AUTOSTART_PROCESSES(&init_system_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(init_system_process, ev, data) {
  PROCESS_BEGIN();

  debug_os("Initializing the MQTT_SN_DEMO");

  init_broker();

  etimer_set(&time_poll, CLOCK_SECOND * 1);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&time_poll));
    sprintf(pub_test,"%s",topic_hw);
    mqtt_sn_pub("/topic_1",pub_test,true,0);
      leds_on(LEDS_ALL);
    // debug_os("State MQTT - :%s",mqtt_sn_check_status_string());
    etimer_set(&time_poll, CLOCK_SECOND * 100);
  }
  PROCESS_END();
}
