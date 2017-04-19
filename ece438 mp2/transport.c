#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include "transport.h"
#include "structure.h"
#define PK_LEN 1400              //One packet is 1KB
#define MAX_PKT_NUM 10240        //Support 10MB data
#define MAXBUFLEN 100
#define MAX_DATA 10485760       //10MB
#define RTT 40000               //40ms

//#define TEST_PORT "12000"
//Global Variable
Packet* pkt;
 uint16_t pkt_seq;
 uint16_t exp_seq = 0;
 int total_size = 0;
//struct timeval t[2*MAX_PKT_NUM];
//struct timeval t1,t2;

struct mp2_socket * mp2_socket_init()
{
    struct mp2_socket* dssocket;
    //socket->s = malloc(sizeof(int));
    dssocket = malloc(sizeof(struct mp2_socket));
    dssocket->s = socket(AF_INET, SOCK_DGRAM, 0);
    if(dssocket->s == -1)
    {
       perror("Socket was not created\n");
    }
    return dssocket;
}

int mp2_connect(struct mp2_socket * dssocket, const char * address,unsigned int port)
{

    struct addrinfo hints, *servinfo;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
  //  servinfo = malloc(sizeof(struct addrinfo));
    char buffer[20];
    sprintf(buffer, "%d", port);
    getaddrinfo(address, buffer, &hints, &servinfo);
   if(connect(dssocket->s,servinfo->ai_addr,servinfo->ai_addrlen) < 0)
      perror("ERROR on connection");
    return 0;
}

int mp2_accept(struct mp2_socket * dssocket, unsigned int port)
{

  /*Configure settings in address struct*/
  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  int socketfd = dssocket->s;
  //printf("FD is %d\n",socketfd);
  if (bind(socketfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
      perror("ERROR on binding");
  return 0;
/*
    //Add handshake
    while(1)
    {
        if(recv(dssocket->s,test,1,0) == 1)
        {
            printf("Handshake succeeds");
            return 0;
        }
    }

*/
}

void mp2_send(struct mp2_socket * dssocket, const void * data, unsigned int len)
{
    printf("len is %d\n",len);
    if(len <= 0 )
    {
        return;
    }
    //char pkt_data[PK_LEN];

    int i,j,k = 0;
    Message* mes;
    mes = malloc(sizeof(Message));
    mes_init(mes);
    int pkt_num = len/PK_LEN;
    int last_pkt_len = len%PK_LEN;
    Packet* pkt;
    pkt = malloc((pkt_num+1)*sizeof(Packet));

    for(i = 0; i < pkt_num+1; i++)
    {
       pkt_init(&pkt[i]);
    }

    
    for(j = 0; j < pkt_num; j++)
    {
    memcpy(pkt[j].data,(char*)data,PK_LEN);
    pkt[j].seq = pkt_seq;
    pkt[j].size = PK_LEN + 4;
    printf("Full data index is %d,strlen is %zu,sequence is %d\n",j,strlen(pkt[j].data),pkt[j].seq);
    pkt_seq++;
    
    }
    if(last_pkt_len != 0)
    {
      memcpy(pkt[j].data,(char*)data+j*PK_LEN,last_pkt_len);
      pkt[j].seq = pkt_seq; 
      pkt[j].size = last_pkt_len + 4;
      pkt_seq++;
      printf("Partial data index is %d,strlen is %zu,sequence is %d\n",j,strlen(pkt[j].data),pkt[j].seq);
      j++;
    }
    
    for(i = 0; i < j; i++)
    {    
      //  gettimeofday(&t[i],NULL);
         printf("Last Packet sequence: %d\n",pkt[i].seq);
         send(dssocket->s,&pkt[i],pkt[i].size,0);
   //     if(recvfrom(dssocket->s,mes,sizeof(Message),0,NULL,NULL) < 0)
   //     {
   //        perror("Recvfrom Failed");
   //     }
     //   gettimeofday(&t[i+1],NULL);
     /*
        if(mes->ack != pkt[i].seq)
        {
           for(k = 0; k <= i+1; k++)
          {
             if(t[i+1].tv_usec - t[i].tv_usec > RTT)
              {
                printf("Has to go back\n");
                if(recvfrom(dssocket->s,mes,sizeof(Message),0,NULL,NULL) < 0)
                {
                   perror("Server is GG\n");
                }
                i = mes->ack;      // Go back to N
              }
          }
        }
     */
    }
    time_t timer;
    char buf[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    puts(buf);
}

unsigned int mp2_recv(struct mp2_socket * dssocket, void * buffer,unsigned int size)
{
    time_t timer;
    char buf[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    puts(buf);

    int counter = 0;
    unsigned int byte_len = 0;
    int recv_size;
    struct sockaddr_in remote;
    int len = sizeof(remote);
    char temp[102400];
    Packet *result;
    result = malloc(size*sizeof(Packet));
    pkt_init(result);
    Message* mes;
    mes = malloc(sizeof(Message));
    mes_init(mes);
    printf("Size is %d\n",size);
    if(size < PK_LEN)
      {
         recv_size = size;
      }
      else
      { 
         recv_size = PK_LEN;
      }
    //total_size += size; 
    while(1)
    {             
      while(1){
      counter++;
      if(recvfrom(dssocket->s,result,recv_size + 10,0,(struct sockaddr *) &remote, &len) < 0)
      {
         perror("Send failed:");
         break;
      }
    mes->ack = result->seq; 
    //sleep(10000);
   
    if(sendto(dssocket->s,mes,sizeof(Message),0,(struct sockaddr *)&remote,len) < 0)
     {
       perror("Fail sendto");
     }
    //printf("r is %d and e is %d\n",mes->ack,exp_seq);
    
    if(result->seq == exp_seq)
      {
        printf("Yes!\n");
        break;
      }
     printf("No!\n");
     printf("result seq is %d, exp_seq is %d\n",result->seq,exp_seq);
    //if(counter > 10)
       break; 
    }
    exp_seq++;  
    //byte_len += sizeof((result)->data)/sizeof(char);
    printf("data len is %d\n",result->size-4);
    memcpy((char*)buffer,(result)->data,result->size-4);
    //strcpy(temp,(result)->data);
    //strcat((char*)buffer,temp);
    // strcat(temp,(result)->data);
   //  printf("temp len is %zu\n",strlen(temp));
     total_size += size;
     printf("total size is %d\n",total_size);   
    // printf("bbbuffer is %s\n", (char*)buffer);
   // if(result->size == size)
   //   {
    //   printf("buffer is %s\n", (char*)buffer);
         printf("data transmission done!\n"); 
         break;        
   //   }
   // break;
    }
  //  mp2_close(dssocket);
    return size;

}
void mp2_close(struct mp2_socket * dssocket)
{
    close(dssocket->s);
}

int main()
{
  int i;
  int port = 12000;
  struct mp2_socket* dssocket = mp2_socket_init();

  /* Sender Side*/
/*
  char address[] = "128.174.187.140";
  char data1[] = "N";
  char data2[] = "I";
  mp2_connect(dssocket,address,port);
  mp2_send(dssocket,data1,1);
*/
 // mp2_send(dssocket,data2,1);

  /*Receiver Side*/

  void* buffer;
  buffer = malloc(100000*sizeof(char));
  printf("size of buffer is %zu\n",strlen(buffer));
  mp2_accept(dssocket,port);
  int a = 0;
  a = mp2_recv(dssocket,buffer,86112);
  printf("a is %d\n",a);
  printf("len is %zu\n",strlen((char*)buffer));
  return 0;
}
