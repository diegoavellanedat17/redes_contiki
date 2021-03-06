/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>

#include <stdbool.h>

uint8_t MAX_RSSI_LOCATION = 0;


/*---------------------------------------------------------------------------*/
//Estructura de Beacon
struct beacon{
  // el id correspondiente al nodo actual
  linkaddr_t id;
  // el rssi que tiene para legar al root
  signed int rssi_c;
};
// Declaramos que la estructura b es de tipo beacon
struct beacon b;
signed int rssi_parent= -1000;
const linkaddr_t node_root_id={{1,0}};
/*---------------------------------------------------------------------------*/

//Estructura de Item de la lista
struct table_possible_parent{
  // ID del mensaje recibio
  linkaddr_t id[10];
  // rssi con enlace
  signed int rssi_total[10];
  uint16_t pos_to_save;
};

struct table_possible_parent table_pc;

/*---------------------------------------------------------------------------*/
// Para declarar bien un item de una lista se hace con
struct posible_parent{
  struct posible_parent *next;
  linkaddr_t id;
  signed int rssi_c;

};
/*---------------------------------------------------------------------------*/
//Definir la lista de padres candidatos
LIST(list_possible_parents);
MEMB(parent_candidato, struct posible_parent,15);


/*---------------------------------------------------------------------------*/
// Crear el proceso de enviar beacons
PROCESS(send_beacon, "Enviar Mensajes de Beacon");

// Crear el proceso de seleccion de padre
PROCESS(select_parent, "Selecciona un parent");

AUTOSTART_PROCESSES( &send_beacon,&select_parent);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  // Se define la estructura Beacon de entrada con los mensajes de rssi de camino
  // y el ID del nodo
  struct beacon *b_message;
  b_message = packetbuf_dataptr();
  signed int b_message_rssi_c =b_message->rssi_c;
  linkaddr_t b_message_id = b_message-> id;


  // se debe tomar el rssi del enlace tambien y sumarlo con el rss del camino
  uint16_t rssi_link=packetbuf_attr(PACKETBUF_ATTR_RSSI);
  signed int rssi_total=b_message_rssi_c+rssi_link;

  struct posible_parent *n_posible_parent;
  n_posible_parent->id=b_message_id;
  n_posible_parent->rssi_c= rssi_total;

  printf("Node %d.%d RSSI PATH %d link %d\n" ,b_message_id.u8[0],b_message_id.u8[1],b_message_rssi_c,rssi_total);
  // meter en la lista de padres cantidatos el mensaje de beacon
  list_push(list_possible_parents,n_posible_parent);
  printf("tamaño de la lista = %d\n ",list_length(list_possible_parents));

}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
//Codigo del proceso send beacon
PROCESS_THREAD(send_beacon,ev,data)
{

  static struct etimer et;
// Guardar el nuevo rssi de Camino una vez se conecta
  if(ev ==PROCESS_EVENT_CONTINUE){
    rssi_parent = data;
  }

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();



  broadcast_open(&broadcast, 129, &broadcast_call);


  while(1) {

    b.id=linkaddr_node_addr;
    b.rssi_c=rssi_parent;

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    // esto verifica que id del nodo es el que esta enviando
    if(linkaddr_node_addr.u8[0] == 1 &&
       linkaddr_node_addr.u8[1] == 0) {
         b.rssi_c=0;
         printf("Nodo root %d\n",b.rssi_c);
    }

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    packetbuf_copyfrom(&b,sizeof(struct beacon));// direccion en donde esta ubicado el beacon y el tamaño
    broadcast_send(&broadcast);
    printf("broadcast message sent %d\n", b.rssi_c);
  }

  PROCESS_END();
}


/*---------------------------------------------------------------------------*/
//Codigo del proceso select_parent
PROCESS_THREAD(select_parent,ev,data)
{

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {
    // este evento corre cuando le llega un evento a este proceso
    PROCESS_YIELD();// cede el procesador hasta que llegue un evento
    if(ev== PROCESS_EVENT_CONTINUE){
      // Definir una estructura de lo que llego que apunte al data recibido
      struct table_possible_parent *recv_table= data;
      // Cuando llegue un evento de este tipo se va a correr esto
      // Recorrer la tabla de padres cantidatos y seleccionar un padre
      //printf("Se recibió ID %d y RSSI %d\n",recv_table->id[0],recv_table->rssi_total[0]);
      //Recorrer la tabla de rssi totales y escoger el menor

      uint8_t i;

      for (i = 0; i < 10; ++i)
      {
        if(recv_table->rssi_total[i]!=0 ){

          if(i==0){
            MAX_RSSI_LOCATION=0;
          }
          else{
            if(recv_table->rssi_total[i] >= recv_table->rssi_total[MAX_RSSI_LOCATION]){
              MAX_RSSI_LOCATION=i;
            }
          }
        }
      }
      printf("Nodo PARENT %d.%d \n", recv_table->id[MAX_RSSI_LOCATION].u8[0],recv_table->id[MAX_RSSI_LOCATION].u8[1]);
      //linkaddr_copy(&linkaddr_node_addr,&recv_table->id[MAX_RSSI_LOCATION]);
      // si escojo padre actualizo el camino del rssi y envìo al proceso de enviar beacon
      signed int new_rssi = recv_table->rssi_total[MAX_RSSI_LOCATION];
      // enviar dato en el nuevo proceso
      process_post(&send_beacon,PROCESS_EVENT_CONTINUE,new_rssi);// en new rssi paso la direccion de memoria
    }

  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
