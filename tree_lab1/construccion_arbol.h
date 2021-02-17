#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>


#define INF_NEG -9999;

////////////////////////////////////
// Estructura del mensaje de beacon
struct beacon{
  linkaddr_t id;
  signed int rssi_c;
};

struct node{
  linkaddr_t preferred_parent;
  signed int rssi_c;// El que va a divulgar
};
///////////////////////////
// Asignacion de estructura
struct beacon b;
struct node n;
// llenar el beacon con la informacion
void fill_beacon_msg (struct beacon *b,linkaddr_t id,signed int rssi_c);
