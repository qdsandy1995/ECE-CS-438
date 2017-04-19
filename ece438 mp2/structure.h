#ifndef STRUCTURE_H_
#define STRUCTURE_H_


#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>


#define MAX_LEN 1400

struct mp2_socket
{
	int s;
}dssocket;

typedef struct Packets {
    uint16_t seq;
    uint16_t size;
    char data[MAX_LEN];
} Packet;

typedef struct Messages {
  uint16_t ack;
  //uint8_t window_size;
  uint8_t state; //0 for normal, 1 for duplicate, 2 for pkt loss
} Message;

void pkt_init(Packet* pkt);
void mes_init(Message* mes);

#endif
