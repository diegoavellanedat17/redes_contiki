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
#include "lib/list.h"
#include "lib/memb.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>
// Librerias propias
#include <construccion_arbol.h>

//LIberia del radio
//#include cc2420.h
/*---------------------------------------------------------------------------*/

PROCESS(send_beacon, "Envia Beacons Periodicamente");
AUTOSTART_PROCESSES(&send_beacon);

/*---------------------------------------------------------------------------*/
/* En MEMB definimos un espacio de memoria en el cual se almacenará la lista  */
MEMB(possible_parent_memb, struct possible_parent, MAX_PARENTS);
// Se define de tipo lista
LIST(possible_parents_list);
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

  struct beacon b_recv;
  // Imprimir el Beacon Recibido apunta a donde esta guardado el packet
  void *msg= packetbuf_dataptr();

  b_recv=*((struct beacon*)msg);// lo convierto tipo beacon y luego cojo su info y la guardo
  signed int b_rec_rssi_c =b_recv.rssi_c;
  linkaddr_t b_rec_id = b_recv.id;
  // Imprimo lo que recibo y luego lo meto en una lista
  printf("broadcast message received from %d with rssi_c = %d\n",b_rec_id.u8[0],b_rec_rssi_c);
  // Guardar los datos en una lista
  /* Mirar si ya conocemos este padre candidato Recorriendo la lista */

  //Defino que la variable p será un padre posible
  struct possible_parent *p;

  for(p = list_head(possible_parents_list); p != NULL; p = list_item_next(p)) {

    /* La idea es que si encuentra un paren con ese mismo id le cambie el rssi en la lista */

    if(linkaddr_cmp(&p->id, &b_rec_id )) {
      printf("Encontre al nodo en la lista Tenia rssi %d\n",p->rssi_c);
      // Hare un update del RSSI
      p->rssi_c=b_rec_rssi_c;
      printf("Nuevo rssi %d\n",p->rssi_c);
      break;
    }
  }
  // Si N es NULL quiere decir que Recorrio toda la lista y no encontro a nadie con el mismo ID
  /* If n is NULL, this neighbor was not found in our list, and we
     allocate a new struct neighbor from the neighbors_memb memory
     pool. */
  if(p == NULL) {
    p = memb_alloc(&possible_parent_memb);
    /* If we could not allocate a new neighbor entry, we give up. We
       could have reused an old neighbor entry, but we do not do this
       for now. */
    if(p == NULL) {
      return;
    }

    linkaddr_copy(&p->id, &b_rec_id);// Guardar en p el nuevo id para la entrada
    p->rssi_c =b_rec_rssi_c;
    list_add(possible_parents_list, p);
    printf("Guardado Tamano lista %d\n",list_length(possible_parents_list));

  }

}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
//Proceso send beacon
PROCESS_THREAD(send_beacon, ev, data)
{
  // Las variables van antes del process PROCESS_BEGIN
  // estas instrucciones corren una vez cuando el proceso inicia
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)


  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  if (linkaddr_node_addr.u8[0]==1) {
    n.rssi_c=0;
  }
  else{
    n.rssi_c = INF_NEG ;
  }

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    fill_beacon_msg(&b,linkaddr_node_addr,n.rssi_c);

    packetbuf_copyfrom(&b, sizeof(struct beacon));
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
  }

  PROCESS_END();

}
