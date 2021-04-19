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
#include <string.h>
#include <stdio.h>
#include "cfs/cfs.h"
// Librerias propias
#include <construccion_arbol.h>
#include <rt.h>



//LIberia del radio
//#include cc2420.h
/*---------------------------------------------------------------------------*/
// Crear el proceso de seleccion de padre
PROCESS(select_parent, "Selecciona un Padre");
// Crear proceso enviar un beacon
PROCESS(send_beacon, "Envia Beacons Periodicamente");
// Crear Procesos de Unicast
PROCESS(unicast_msg, "Mensajes de Unicast");
// Construir la tabla de enrutamiento del nodo
PROCESS(build_RT, "Construir la tabla de enrutamiento ");

AUTOSTART_PROCESSES(&send_beacon,&select_parent,&unicast_msg,&build_RT);
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* En MEMB definimos un espacio de memoria en el cual se almacenará la lista  */
MEMB(possible_parent_memb, struct possible_parent, MAX_PARENTS);
// Se define de tipo lista
LIST(possible_parents_list);
/*---------------------------------------------------------------------------*/
bool me_created=false;

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

  // Sacar el RSSI del enlace y aca ya tengo el rssi total
  uint16_t rssi_link=packetbuf_attr(PACKETBUF_ATTR_RSSI);
  signed int rssi_total=b_rec_rssi_c+rssi_link;
  // Imprimo lo que recibo y luego lo meto en una lista
  //printf("broadcast message received from %d with rssi_c = %d\n",b_rec_id.u8[0],b_rec_rssi_c);

  // Si soy el nodo root no tengo que hacer ninguna lista
  if (linkaddr_node_addr.u8[0]!=1) {

  //Defino que la variable p será un padre posible
  struct possible_parent *p;

  // Guardar los datos en una lista
  /* Mirar si ya conocemos este padre candidato Recorriendo la lista */

  for(p = list_head(possible_parents_list); p != NULL; p = list_item_next(p)) {

    /* La idea es que si encuentra un paren con ese mismo id le cambie el rssi en la lista */

    if(linkaddr_cmp(&p->id, &b_rec_id )) {
      printf("Encontre al nodo en la lista Tenia rssi %d\n",p->rssi_c);
      // Hare un update del RSSI
      p->rssi_c=rssi_total;
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
  }

    printf("Proceso de Seleccion de padre \n");
    process_post(&select_parent,PROCESS_EVENT_CONTINUE,&(possible_parents_list));
  }

}

//Para boradcast
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{

  struct unicast_message msg_recv;
  // Imprimir el Beacon Recibido apunta a donde esta guardado el packet
  void *msg= packetbuf_dataptr();
  // ya tengo el mensaje que debo retransmitir
  msg_recv=*((struct unicast_message*)msg);//

  printf("unicast received payload %s  %d.%d\n",
	msg_recv.msg_cadena,msg_recv.id.u8[0], msg_recv.id.u8[1]);
   // Aca se usarà el add child y esas cosas
   //Enviar a un proceso en donde se vaya contruyendo la tabla de enrutamiento.
   process_post(&build_RT,PROCESS_EVENT_CONTINUE,&msg_recv);

}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: \n",
    dest->u8[0], dest->u8[1]);
}


//Para unicast
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

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
    //printf("broadcast message sent with %d\n",n.rssi_c);
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
      //Recorrer la tabla de rssi totales y escoger el menor

      // Recorrer la lista y ver los valores de RSSI
      struct possible_parent *p;
      struct possible_parent *p_header;
      // Asigno el rssi seleccionado a la cabeza de la lista

      signed int rssi_selected;
      p_header=list_head(possible_parents_list);
      rssi_selected=p_header->rssi_c;


      for(p = list_head(possible_parents_list); p != NULL; p = list_item_next(p)) {

        /* Recorro la lista competa y voy imprimiendo los valores de rssi */
        if(p->rssi_c >= rssi_selected ){
          selected_parent=p;
          rssi_selected=p->rssi_c;
        }

      }

      printf("Padre seleccionado %d\n", selected_parent->id.u8[0] );
      // Actualizar el rssi que voy a empezar a divulgar
      n.rssi_c=selected_parent->rssi_c;
      printf("#L %d 0\n", n.preferred_parent.u8[0]);
      linkaddr_copy(&n.preferred_parent,&selected_parent->id);
      printf("#L %d 1\n", n.preferred_parent.u8[0]);

    }

  }

  PROCESS_END();
}

//Proceso para enviar unicast

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_msg, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      /*        */
      /* step 3 */
      /*        */
      /* reading from cfs */
      if (linkaddr_node_addr.u8[0]!=1) {

        char message[32];
        char *filename = "msg_file";
        char buf[100];
        int fd_read, fd_write;
        int n_cfs;
        


        fd_read = cfs_open(filename, CFS_READ);
        if(fd_read!=-1) {
            cfs_read(fd_read, buf, sizeof(message));
            printf("paso 2: se leyo : %s\n", buf);
            // lo que lee del archivo, llena el unicast para emviarlo
            //msg_to_fill=strdup(buf);

            cfs_close(fd_read);

            fill_unicast_msg(&u_msg,linkaddr_node_addr,buf);
            packetbuf_copyfrom(&u_msg, sizeof(struct unicast_message));
            unicast_send(&uc, &n.preferred_parent);

          }

        else {
            // Aqui solo entra la primera vez, con el objetivo de guardar el propio nodo con )
            //printf("ERROR: could not read from memory in step 3.\n");
            //Creo una variable donde se enviara el unicast por primera vez
            char  buf_link[10];
            sprintf(buf_link, "%d", linkaddr_node_addr.u8[0]);
            printf("buf_link es %s\n",buf_link );

            //aca se supone que arma el mensaje completo
            fd_write = cfs_open(filename, CFS_WRITE);

            if(fd_write != -1) {
              printf("En esta se genera el archivo pero no se envia\n");
              n_cfs= cfs_write(fd_write, buf_link, sizeof(buf_link));
              cfs_close(fd_write);
            }
            else {
              printf("No hemos podido escribir en el unicast .\n");
            }

          }


        }

    }


    PROCESS_END();

}

/*---------------------------------------------------------------------------*/
//Codigo del proceso Contruir la tabla de enrutamiento
PROCESS_THREAD(build_RT,ev,data)
{

  PROCESS_BEGIN();
  //Lo que esta aca corre una sola vez en toda la vida
  static node *me_node; //= new_node(5);
  me_node= new_node(linkaddr_node_addr.u8[0]);

  //printf("Esto corre una sola vez \n");
  while(1) {
    // este evento corre cuando le llega un evento a este proceso
    PROCESS_YIELD();// cede el procesador hasta que llegue un evento
      if(ev== PROCESS_EVENT_CONTINUE){

        struct unicast_message *msg_recv= data;
        printf("unicast received payload UN rt %s %d.%d\n",
        msg_recv->msg_cadena,msg_recv->id.u8[0], msg_recv->id.u8[1]);
        char mensaje_cadena[]="";
        strcpy(mensaje_cadena,msg_recv->msg_cadena);

        char message[32];
        char *filename = "msg_file";
        char buf[100]="";
        int fd_read,fd_write;
        int n_cfs;

        fd_read = cfs_open(filename, CFS_READ);

        if(fd_read!=-1){
            cfs_read(fd_read, buf, sizeof(message));
            cfs_close(fd_read);
          }

        else {

          char  buf_link[10];
          sprintf(buf_link, "%d", linkaddr_node_addr.u8[0]);
          //printf("buf_link es %s\n",buf_link );
          //strcat(buf_link, ",)");

          fd_write = cfs_open(filename, CFS_WRITE);

          if(fd_write != -1) {
            printf("En esta se genera el archivo pero no se envia\n");
            n_cfs= cfs_write(fd_write, buf_link, sizeof(buf_link));
            cfs_close(fd_write);
          }

          else
              printf("No hemos podido escribir en el unicast .\n");
          }

          if (strcmp(buf,"") != 0){
            printf("El de este current node es : %s\n", buf);
            //me_node= new_node(linkaddr_node_addr.u8[0]);
            item list_backtrace=NULL;
            item list_visited=NULL;
            me_node= deserialize(me_node,buf,list_backtrace);
            me_node=deserialize(me_node,mensaje_cadena,list_backtrace);
            char cadena_to_save[100];
            serialize(me_node,list_backtrace,list_visited,cadena_to_save);
            printf("La cadena a guardar es %s\n", cadena_to_save);


          }


        //add_child(me_node,msg_recv->id.u8[0]);
        //item list_backtrace=NULL;
        //item list_visited=NULL;
        //print_childs(me_node,list_backtrace,list_visited);

        // Manejo de archivos en contiki

        // char message[32];
        // char buf[100];
        // strcpy(message,"#1.writing routing t");
        // strcpy(buf,message);
        // printf("step 1: %s\n", buf );

        /* step 2 */
        /*        */
        /* writing to cfs */
        //char *filename = "msg_file";
        //int fd_write;
        //int n;

        // fd_write = cfs_open(filename, CFS_WRITE);
        // if(fd_write != -1) {
        //   n = cfs_write(fd_write, message, sizeof(message));
        //   cfs_close(fd_write);
        //   printf("step 2: successfully written to cfs. wrote %i bytes\n", n);
        // } else {
        //   printf("ERROR: could not write to memory in step 2.\n");
        // }




    }

  }

  PROCESS_END();
}
