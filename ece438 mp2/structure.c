

#include <stdio.h>
#include <stdlib.h>

#include "structure.h"

void pkt_init(Packet* pkt)
{
   pkt->seq = 0;
   pkt->size = 0;
   int i;
   for(i = 0; i < MAX_LEN; i++)
      pkt->data[i] = 0;
}

void mes_init(Message* mes)
{
  mes->ack = 0;
  //mes->window_size = rwnd;
  mes->state = 0;
}


