//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//****************************************************************************

#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include <unistd.h>


#include <pthread.h>

#include <list>

#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"


int define_socket_TCP(int port) {

  struct sockaddr_in sin;
  int s_socket;
  s_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (s_socket<0)
    errexit("Could't create socket: %s\n", strerror(errno));  //Si hay error, salimos.

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;   //Configuramos el sin
  sin.sin_port = htons(port);

    if (bind(s_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0)  //Unimos el socket con sin
        errexit("Could't bind to port: %s\n", strerror(errno));

    if (listen(s_socket,5) < 0)
        errexit("Error in listen function: %s\n", strerror(errno));

    return s_socket;
}

// This function is executed when the thread is executed.
void* run_client_connection(void *c) {
    ClientConnection *connection = (ClientConnection *)c;

    connection->WaitForRequests();

    return NULL;
}

FTPServer::FTPServer(int port) {
    this->port = port;
}

// Parada del servidor.
void FTPServer::stop() {
    close(msock);
    shutdown(msock, SHUT_RDWR);

}

// Starting of the server
void FTPServer::run() {
    struct sockaddr_in fsin;
    int ssock;
    socklen_t alen;
    msock = define_socket_TCP(port);
    while (1) {
      pthread_t thread;
      ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
      if(ssock < 0)
          errexit("Fallo en el accept: %s\n", strerror(errno));

      ClientConnection *connection = new ClientConnection(ssock);
      connection_list.push_back(connection);  //Añadido a la lista por si es necesario
      pthread_create(&thread, NULL, run_client_connection, (void*)connection);  //Creamos un hilo para cada conexión que además añade cada usuario a una lista

    }

}
