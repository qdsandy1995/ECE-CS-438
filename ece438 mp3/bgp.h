#ifndef BGP_H_
#define BGP_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct peer_table{
   int peer_type;
   int AS_num;
   int socket;
   int port;
   char host[50];
}pb_t;

typedef struct route_table{
   int valid;
   int AS_num[50];
   char prefix[50];
   int type;
   int AS_count;   
}rt_t;

typedef struct BGP{
   int reply;
   int number;
   int AS_NUM;
   char host[50];
   int port;
   int peer_type;
   rt_t rout[50];
}bgp_t;

int tcp_connect(int socketfd, const char * address,unsigned int port);
int tcp_server(int socketfd, unsigned int port);
int mp3_connect(int socketfd, char* type, char* address);
int disconnect(int socketfd, char* type, char* address);
int advertise(int socketfd, char* type, char* address);
int withdraw(int socketfd, char* type, char* address);
int routes(int socketfd, char* type, char* address);
int best_ip(int socketfd, char* type, char* address);
int peers(int socketfd, char* type, char* address);
int quit(int socketfd, char* type, char* address);
void AS_table_peer_print();
void check_valid(int type);
void update_table(bgp_t* buffer,int i);

#endif
