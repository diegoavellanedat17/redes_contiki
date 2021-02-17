#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>


#define INF_NEG -9999
// El maximo en la tabla de padres candidatos podemos considerar un remove o un update
#define MAX_PARENTS 30
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
// Asignacion de estructura
struct beacon b;
struct node n;
// llenar el beacon con la informacion
void fill_beacon_msg (struct beacon *b,linkaddr_t id,signed int rssi_c);
