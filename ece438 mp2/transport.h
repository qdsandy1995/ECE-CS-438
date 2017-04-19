/* Interface for building the MP2 transport */

/* You may define this however you want */
struct mp2_socket;

/* Initialize a socket */
struct mp2_socket * mp2_socket_init();

/* Connect to a client address and port.
   Port here is specified in HOST byte order.
   Must be called on an initialized socket.

   Returns 0 on success, -1 on error */
int mp2_connect(struct mp2_socket * socket, const char * address,
  unsigned int port);

/* Listen on a port and accept a connection.
   Port here is specified in HOST byte order.
   Must be called on an initialized socket.

   Returns 0 on success, -1 on error */
int mp2_accept(struct mp2_socket * socket, unsigned int port);

/* Send data to a remote socket. Should send
   all of the data, potentially breaking it up into packets.
   Should block for flow control, but some buffering is OK. */
void mp2_send(struct mp2_socket * socket, const void * data, unsigned int len);

/* Receive data from a remote socket. Should wait for data to be available,
   then return _up to_ len bytes. Returns the number of bytes received.
   Should return 0 if the remote end has closed the socket. */
unsigned int mp2_recv(struct mp2_socket * socket, void * buffer,
  unsigned int size);

/* Closes a socket and lets the remote end know the connection is finished */
void mp2_close(struct mp2_socket * socket);


