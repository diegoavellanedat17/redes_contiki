#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "contiki.h"
#include "net/rime/rime.h"

// Estructura del nodo, se tiene un id, apuntadores al sibling y se child
struct node {
    int id;
    struct node *sibling;
    struct node *child;
};

// Esta corresponde la esructura de la lista, tiene las mismas caracteristicas
// de los nodos pero tiene un apuntador al siguiente elemento de la lista
struct LinkedList{
    int id;
    struct node *sibling;
    struct node *child;
    struct LinkedList *next;
};

//Se define el item de la linked list
typedef struct LinkedList *item; //Define node as pointer of id type struct LinkedList
//Se define el elemento nodo
typedef struct node node;



//Definicion de las funciones que se encuentran despues del main
node * new_node(int);
node * add_sibling(node *, int);
node * add_child(node *, int);
void print_childs(node * n, item list_backtrace, item list_visited);
item createitem();
void print_list(item head);
item tail(item head);
void remove_last_item(item head);
bool is_node_in_list(item head,node * n);
item add_node_list(item list_pointer, node *n);
node * from_item_to_node(item list_node);
int search_forwarder(node * n, item list_backtrace,item list_visited, int id_node);
void serialize(node * n, item list_backtrace,item list_visited,char cadena_serializada[]);
void deserialize(node *n,char cadena_serializada[],item list_backtrace);
