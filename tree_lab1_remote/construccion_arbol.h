#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>
// para setear la TX Power
#include "net/netstack.h"


// Para diferenciar entre skymote y telos b

#define INF_NEG -9999
// El maximo en la tabla de padres candidatos podemos considerar un remove o un update
#define MAX_PARENTS 10
// El maximo de Unicast messages para envío
#define MAX_UNICAST_MSGS 10

//
/* This is the structure of unicast ping messages. */
struct unicast_message {
  char* msg;
  linkaddr_t id;
};
////////////////////////////////////
// Estructura del mensaje de beacon qu voy a enviar
struct beacon{
  linkaddr_t id;
  signed int rssi_c;
};
// Estructura nodo que se va a divulgar
struct node{
  linkaddr_t preferred_parent;// El padre del nodo
  signed int rssi_c;// El que va a divulgar
};
// Estructura posibles padres para la lista
/* This structure holds information about neighbors. */
struct possible_parent {

  struct possible_parent *next;
  linkaddr_t id;
  signed int rssi_c;

};

// Estructura de mensajes para retransmision */
struct u_retransmit_msg {

  struct u_retransmit_msg *next;
  char* msg;
  linkaddr_t id;

};
// Asignacion de estructura
struct beacon b;
static struct node n;
static struct possible_parent *selected_parent;


struct unicast_message u_msg;
//static struct unicast_conn msg_unicast;
// llenar el beacon con la informacion
void fill_beacon_msg (struct beacon *b,linkaddr_t id,signed int rssi_c);
void fill_unicast_msg(struct unicast_message *unicast_msg,linkaddr_t id);
signed int signExtension (uint16_t rssi);
