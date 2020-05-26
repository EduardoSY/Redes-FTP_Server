/*
REDES Y SISTEMAS DISTRIBUIDOS
Universidad de La Laguna
Curso 19/20
Autor: Eduardo Da Silva Yanes
*/

#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>

const int MAX_BUFF=1000;

class ClientConnection {
public:
    ClientConnection(int s);
    ~ClientConnection();

    void WaitForRequests();
    void stop();


private:
   bool ok; // This variable is a flag that avois that the
	     // server listens if initialization errors occured.

    FILE *fd;	 // C file descriptor. We use it to buffer the
		 // control connection of the socket and it allows to
		 // manage it as a C file using fprintf, fscanf, etc.

    char command[MAX_BUFF];  // Buffer para almacenar el comando.
    char arg[MAX_BUFF];      // Buffer para almacenar los argumentos.

    int data_socket;         // Descriptor de socket para la conexion de datos;
    int control_socket;      // Descriptor de socket para al conexión de control;

    bool parar;

    bool pass;
};

#endif
