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
#include <pthread.h>
#include "bgp.h"
#define BACKLOG 10     // how many pending connections queue will hold
#define str_size 3




//Global Variable
const char *table[] = {"connect","disconnect","advertise","withdraw","routes","best","peers","quit"};
int (*jump_table[])(int,char*,char*) = {mp3_connect,disconnect,advertise,withdraw,routes,best_ip,peers,quit};
volatile int peer_id = 0;
volatile int rout_table_id = 0;
volatile int id;
volatile int server_id = 0;
volatile int client_id = 0;
int host_port;
fd_set active_fd_set;
//int new_fd;
//int socketfd_c;
bgp_t* AS;
pb_t* PT;
int advertise_count = 0;
pthread_mutex_t index_lock;


int tcp_connect(int socketfd, const char * address,unsigned int port)
{

    struct addrinfo hints, *servinfo;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char buffer[20];
    sprintf(buffer, "%d", port);
    getaddrinfo(address, buffer, &hints, &servinfo);
   if(connect(socketfd,servinfo->ai_addr,servinfo->ai_addrlen) < 0)
      perror("ERROR on connection");
    
    return 0;
}

int tcp_server(int socketfd, unsigned int port)
{

  struct sockaddr_in serveraddr;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  //printf("FD is %d\n",socketfd);
  if (bind(socketfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
      perror("ERROR on binding");
  if(listen(socketfd, BACKLOG) < 0 )
      perror("listen");
  return 0;
}

void *both_side(void* socketfd)
{
    bgp_t* buffer;
    fd_set read_fd_set;
    struct timeval* tv;
    tv->tv_sec = 2;
    tv->tv_usec = 50000;
    buffer = malloc(sizeof(bgp_t));
    int i, j, k, m; 
    
    FD_ZERO(&active_fd_set);
    FD_SET((int)socketfd, &active_fd_set);

    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    while(1)
    {  
      
       pthread_mutex_lock(&index_lock);
       read_fd_set = active_fd_set;
       pthread_mutex_unlock(&index_lock);
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, tv) < 0)
        {
          perror ("select");
          exit (EXIT_FAILURE);
        }
        
     /*
       if((new_fd[server_id] = accept((int)socketfd, (struct sockaddr *)&their_addr, &addr_size)) < 0)
          perror("accept");
       if(recv(new_fd[server_id],buffer,sizeof(bgp_t),0) == -1) 
           perror("recv");
       printf("Message recved\n");
     */
      for (i = 0; i < FD_SETSIZE; ++i)
       {      
         if (FD_ISSET(i, &read_fd_set))
          {
            
            if (i == (int)socketfd)
              {
                /* Connection request on original socket. */

                int new;
                new = accept ((int)socketfd,(struct sockaddr *)&their_addr, &addr_size);
                if (new < 0)
                  {
                    perror ("accept");
                    exit (EXIT_FAILURE);
                  }
                //printf("i is %d\n",i);

                FD_SET(new, &active_fd_set);
              }
            else
              {
                /* Data arriving on an already-connected socket. */

                if(recv(i,buffer,sizeof(bgp_t),0) == -1) 
                   perror("recv");
              // printf("Recvd sth\n");
                if(buffer->reply == 1)
                  {
			pthread_mutex_lock(&index_lock);
          		AS->AS_NUM = AS->number;
                      
          		AS->peer_type = buffer->peer_type;
          		AS->reply = 0;
                  //      printf("check peer: %d\n",4 - AS->peer_type);
                        AS->port = -1;
                        pthread_mutex_unlock(&index_lock);
                        check_valid(4 - AS->peer_type);
          		if(send(i,AS,sizeof(bgp_t),0) == -1)
            		  perror("Send");
                        pthread_mutex_lock(&index_lock);
          	//	printf("Message sent back\n");
                        pthread_mutex_unlock(&index_lock);
                  }
                if(buffer->AS_NUM != 0)
                {
                   update_table(buffer,i);
                }
               /* inform "qualified" neighbors about the update*/
               for (j = 0; j < FD_SETSIZE; ++j)
                 {
                    if(FD_ISSET(j, &active_fd_set) && j != i && j != (int)socketfd) 
                     {
                         for(k = 0; k < peer_id; k++)
                          {
                            if(PT[k].socket == j)
                              {
                                // Check if the previous connection has a smaller type number
                                  int temp = 4 - PT[k].peer_type;
                                  printf("peer_type print: %d\n",AS->peer_type);
                                 // printf("neighbor's peer_type print: %d\n",PT[k].peer_type); 
                                  if(AS->peer_type == -1 || temp == 1 || AS->peer_type == 3)
                                   {
                                     pthread_mutex_lock(&index_lock);
                                     AS->reply = 0;
                                     AS->AS_NUM = -1;
                                     AS->peer_type = 4 - PT[k].peer_type;
                                      AS->port = -1;
                                     pthread_mutex_unlock(&index_lock);
                                    printf("Notify neighbors to update\n");
                                     check_valid(AS->peer_type);
                                     if(send(j,AS,sizeof(bgp_t),0) == -1)
            		                perror("Send");
                                   }
                                 break; 
                              }
                          }
                     }
                 }
               }
           }
        }
    }
    
    printf("pthread is gone\n");
    pthread_exit(NULL);
}

//Has to compate ip prefix, only update different entries
void update_table(bgp_t* buffer,int i)
{
       int j,k,m,n,g;
       int flag1 = 0;
       int flag2 = 1;
       int flag3 = 0;
       int counter = 0;
        pthread_mutex_lock(&index_lock);
       if(buffer->AS_NUM != -1)
        {   
          PT[peer_id].AS_num = buffer->AS_NUM;
          PT[peer_id].peer_type = buffer->peer_type;
          PT[peer_id].socket = i;
          if(buffer->port != -1)
          {
              PT[peer_id].port = buffer->port;
           //   printf("Host name is %s\n", buffer->host);
              strcpy(PT[peer_id].host , buffer->host);
          }
          peer_id++;
        }

       for(j = 0; j < 50; j++)
        {
           flag1 = 0;
           flag2 = 1;
           flag3 = 0;
           if((buffer->rout[j]).valid == 1)
            {
               for(m = 0; m < advertise_count; m++)
                 {
                     if(strcmp((AS->rout[m]).prefix,(buffer->rout[j]).prefix) == 0)
                      {
                        flag1 = 1;                 //Update AS lists throught the route  
                        break;
                      }
                 }
               for(m = 0; m < rout_table_id; m++)
                 {
                     if(strcmp((AS->rout[m]).prefix,(buffer->rout[j]).prefix) == 0)
                      {
                        flag3 = 1;                 //Update AS lists throught the route  
                        break;
                      }
                 }
              for(m = 0; m < rout_table_id; m++)
               {
                 flag2 = 1;
                 if((AS->rout[m]).AS_count == (buffer->rout[j]).AS_count + 1)
                   {
                     for(g= 0; g < (buffer->rout[j]).AS_count; g++)
                      {
                        if( (AS->rout[m]).AS_num[g] != (buffer->rout[j]).AS_num[g])
                            {
                                flag2 = 0;
                                break;
                            }
                      }
                     if(flag2 == 1)
                     {
                        break;
                     }
                   }
                else
                {
                    flag2 = 0;
                }
              }
               //(AS->rout[rout_table_id]).valid = 1;
           //   printf("%d %d %d %d\n",rout_table_id,flag1,flag2,flag3);
              if((flag1 == 0 && flag2 == 0) || flag3 == 0)        
                {   
                   strcpy((AS->rout[rout_table_id]).prefix , (buffer->rout[j]).prefix); //Update the prefix destination
                   for(k = 0; k < (buffer->rout[j]).AS_count; k++)
                   {
                      (AS->rout[rout_table_id]).AS_num[k] = (buffer->rout[j]).AS_num[k];                 //Update AS lists throught the route
                   }
                   (AS->rout[rout_table_id]).AS_num[(buffer->rout[j]).AS_count] = AS->number;   //Add its own AS number to the route
                   (AS->rout[rout_table_id]).AS_count = (buffer->rout[j]).AS_count + 1;         //Update the AS num counter
                   (AS->rout[rout_table_id]).type = buffer->peer_type;                     //Update the latest type
                   rout_table_id++; 
               }
            }
        }
      
       pthread_mutex_unlock(&index_lock);
}



int mp3_connect(int socketfd, char* type, char* address)
{
   int i,j;
   int socketfd_c = socket(AF_INET, SOCK_STREAM, 0);
   char* token,*host,*port;
   char* s1 = ":";
   char* s2 = "\0";
   token = strtok(address,s1);
   host = token;
   token = strtok(NULL,s2);
   port = token;
   pthread_mutex_lock(&index_lock);
   gethostname(AS->host,19);
  // printf("AS host: %s\n",AS->host);
   AS->port = host_port;
 //  printf("AS port: %d\n",host_port);
   AS->AS_NUM = AS->number;
   AS->peer_type = atoi(type);
   AS->reply = 1;
   pthread_mutex_unlock(&index_lock);
/*
   char buffer[100];
   sprintf(buffer, "%d", AS->number);
   strcat(buffer," ");
   strcat(buffer,type);
   strcat(buffer," ");
   for(j = 0; j < 100; j++)
    {
      if((AS->rout[j]).valid == 1)
       {
         //printf("Valid prefix: %s\n",AS->rout[j].prefix);
         strcat(buffer,AS->rout[j].prefix);
         strcat(buffer," ");
       }
    }
*/
 //  printf("Socket for connection is %d\n",socketfd_c);
   tcp_connect(socketfd_c, host,atoi(port));
   pthread_mutex_lock(&index_lock);
   FD_SET(socketfd_c, &active_fd_set);
   
   /*
   for (i = 0; i < FD_SETSIZE; ++i)
    {
      if(FD_ISSET(i, &active_fd_set))
        printf("current fd is %d\n",i);
    }
 */
   check_valid(AS->peer_type);
   PT[peer_id].port = atoi(port);
   strcpy(PT[peer_id].host,host);
   pthread_mutex_unlock(&index_lock);
   send(socketfd_c,AS,sizeof(bgp_t),0);
 //  printf("Start sending data...\n");
   pthread_mutex_lock(&index_lock);

   AS->AS_NUM = -1;
   AS->peer_type = -1;
   pthread_mutex_unlock(&index_lock);
   return 0;
}
int disconnect(int socketfd, char* type, char* address)
{ 
   int i;
   char* port,*token;
   char* s1 = ":";
   char* s2 = "\0";
   token = strtok(address,s1);
   token = strtok(NULL,s2);
   port = token; 
   int temp = AS->reply;
   // close(PT[i].socket);
    pthread_mutex_lock(&index_lock);
    PT[i].peer_type == -1;
    PT[i].AS_num == -1;
    AS->reply = temp;
    pthread_mutex_unlock(&index_lock);
   return 0;
}
int advertise(int socketfd, char* type, char* address)
{
   pthread_mutex_lock(&index_lock);
   AS->reply = 0;
   strcpy((AS->rout[rout_table_id]).prefix,address);                         //Need to modify when append case happened
   (AS->rout[rout_table_id]).AS_num[0] = AS->number;
   (AS->rout[rout_table_id]).type = 0;
   (AS->rout[rout_table_id]).AS_count++;
   //printf("prefix is %s\n",address);
   (AS->rout[rout_table_id]).valid = 1;
   rout_table_id++;
   advertise_count++;
   pthread_mutex_unlock(&index_lock);
   return 0;
}
int withdraw(int socketfd, char* type, char* address)
{
   return 0;
}
int routes(int socketfd, char* type, char* address)
{
    AS_table_peer_print();
   return 0;
}
int best_ip(int socketfd, char* type, char* address)
{
   return 0;
}
int peers(int socketfd, char* type, char* address)
{
   int j;
   for(j = 0; j < peer_id; j++)
    {    
       if(PT[j].peer_type == 1)
        {
           printf("Provider ");
           printf("%s",PT[j].host);
           printf(":%d\n",PT[j].port);
        }
       else if(PT[j].peer_type == 2)
        {
           printf("Peer ");
           printf("%s",PT[j].host);
           printf(":%d\n",PT[j].port);
        }
       else if(PT[j].peer_type == 3)
        {
           printf("Customer ");
           printf("%s",PT[j].host);
           printf(":%d\n",PT[j].port);
        }
    }
   
   return 0;
}
int quit(int socketfd, char* type, char* address)
{
   return 0;
}

void AS_table_peer_print()
{
   char* token;
   char* s = "\n";
   int j,k;
   printf("BEGIN ROUTE LIST\n");
   for(j = 0; j < rout_table_id; j++)
    {
       printf("%s ",strtok((AS->rout[j]).prefix,s));
       for(k = (AS->rout[j]).AS_count - 2; k >= 0; k--)
        {
           printf("%d ",(AS->rout[j]).AS_num[k]);
        }
       printf("\n");
    }
  printf("END ROUTE LIST\n");
}



void check_valid(int type)
{
     int j;
   
    for(j = 0; j < rout_table_id; j++)                                                
    {
       if(type == 1 || (AS->rout[j]).type == 0)
        {
      //     printf("valid id: %d\n",j);
            (AS->rout[j]).valid = 1;
        } 
       else if((AS->rout[j]).type == 3)
        {
            (AS->rout[j]).valid = 1;
        }
       else
        {
            (AS->rout[j]).valid = 0;
        }
      
    }
   
}

int main(int argc, char **argv)
{
  if(argc != 3)
   {
      printf("Argument number incorrect!\n");
      return 0;
   }

  // initialize variables
  PT = malloc(50*sizeof(pb_t));
  AS = malloc(sizeof(bgp_t));
  AS->AS_NUM = -1;
  AS->peer_type = -1;
  host_port = atoi(argv[2]);
;
  pthread_t tid_s,tid_c;
  char buffer[10];
  char inputs[50];
  const char s[2] = " ";
  char *token,*type,*address;
  int result,j;
  int i = 0;
  int function_num = 7;
  
  int socketfd_s = socket(AF_INET, SOCK_STREAM, 0);
  //printf("Server socket is %d\n",socketfd_s);
  for(j = 0; j < 50; j++)
   {
      (AS->rout[j]).valid = 0;
      (AS->rout[j]).AS_count = 0;
   }

   
  // printf("server fd is %d, client fd is %d\n",socketfd_s,socketfd_c);
  tcp_server(socketfd_s, atoi(argv[2]));
  //AS->socketfd = socketfd;
  AS->number = atoi(argv[1]);
  pthread_create(&tid_s, NULL, both_side, (void*)socketfd_s);
  while(1)
{
  fgets(inputs,sizeof(inputs),stdin);
  //printf("the type is %s\n",(AS->peer[0]).peer_type);
  for(i = 0; i < 8; i++)
  { 
    if(!strncmp(inputs,table[i],str_size))
       {
           function_num = i;  
           break;
       }
  }

  token = strtok(inputs,s);
  for(i = 0; i < 8; i++)
   {
      if(!strcmp(token,table[i]))
       {
           function_num = i;
           break;
       }
   }
  switch(function_num)
  {
	  case 0:
	  token = strtok(NULL,s);
	  type = token;
          if(!strncmp(type,"customer",8))
          {
             type = "1";
          }
          else if(!strncmp(type,"peer",4))
          {
             type = "2";
          }
          else if(!strncmp(type,"provider",8))
          {
             type = "3";
          }
          else
          {
             printf("You made a typo!\n");
             return 0;
          }
	  token = strtok(NULL,s);
	  address = token;
	  break;
	  
	  case 1:
	  token = strtok(NULL,s);
	  address = token;
	  break;
	  
	  case 2:
	  token = strtok(NULL,s);
	  address = token;
          
	  break;
	  
	  case 3:
	  token = strtok(NULL,s);
	  address = token;
	  break;
	  
	  case 5:
	  token = strtok(NULL,s);
	  address = token;
	  default:
          break;
  }
  //printf("function_num is %d\n",function_num);
  result = jump_table[function_num](0,type,token);
  function_num = 7;
  //pthread_join(tid,NULL);
}
  return 0;
}
