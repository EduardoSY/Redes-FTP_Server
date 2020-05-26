/*
REDES Y SISTEMAS DISTRIBUIDOS
Universidad de La Laguna
2º de grado de Ingeniería Informática
Curso 19/20
Autor: Eduardo Da Silva Yanes
----------------------------------
This class processes an FTP transactions.
*/

#include <unistd.h> //Incluido para la funcion Close

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>

#include "common.h"
#include "ClientConnection.h"

ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);
  char buffer[MAX_BUFF];
  control_socket = s;

  fd = fdopen(s, "a+");
  if (fd == NULL){
    std::cout << "Connection closed" << std::endl;
    fclose(fd);
    close(control_socket);
    ok = false;
    return ;
  }
  ok = true;
  data_socket = -1;
  pass=false;
};

ClientConnection::~ClientConnection() {
  fclose(fd);
  close(control_socket);
}

int connect_TCP( uint32_t address,  uint16_t  port) {
  struct sockaddr_in sin;
  int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_tcp<0)
  errexit("Error when creating socket: %s\n", strerror(errno));

  memset(&sin, 0, sizeof(sin));

  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address;

  if (connect(sock_tcp, (struct sockaddr*)&sin, sizeof(sin)) < 0)
  errexit("Error when connecting to %s: %s\n", address, strerror(errno));

  return sock_tcp;
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}


#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.
void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }
  fprintf(fd, "220 Service ready\n");

  while(!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
      printf("--->Comando: USER\n");
      fscanf(fd, "%s", arg);
      if (!strcmp(arg,"prueba"))
        fprintf(fd, "331 User name okay, need password.\n");
      else
        fprintf(fd, "332 Need account for login.\n");
    }

    else if (COMMAND("PWD")) {
      printf("--->Comando: PWD\n");
      char path[MAX_BUFF];
      if (getcwd(path, sizeof(path)) != NULL)
        fprintf(fd, "257 \"%s\" \n", path);
    }

    else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      printf("--->Comando: PASS ");
      for(int i=0;i<strlen(arg);i++){
        printf("*");
      }

      printf("\n");
      if (!strcmp(arg,"prueba"))
        fprintf(fd, "230 User logged in, proceed.\n");  //RFC 959 code
      else
        fprintf(fd, "530 Not logged in.\n");
    }

    else if (COMMAND("PORT")) {
      printf("--->Comando: PORT\n");
      pass = false;
      unsigned int ip[4];
      unsigned int port[2];

      fscanf(fd, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port[0], &port[1]);  //IP + funcion matematica para sacar puerto basandose en 2 numeros.
      uint32_t ip_addr_c = ip[3]<<24 | ip[2]<<16 | ip[1]<<8 | ip[0];  //Rodamos la IP bit a bit
      uint16_t port_c = port[0] << 8 | port[1]; //Mismo paso anterior con el puerto

      data_socket = connect_TCP(ip_addr_c,port_c);  //Nos conectamos al puerto del cliente
      fprintf(fd, "200 OK\n");
    }

    else if (COMMAND("PASV")) {
      pass = true;
      struct sockaddr_in sin, sa;
      socklen_t sa_len = sizeof(sa);
      int sock_d;
      sock_d = socket(AF_INET, SOCK_STREAM, 0);
      if (sock_d<0)
        errexit("Could't create socket: %s\n", strerror(errno));

      memset(&sin, 0, sizeof(sin));
      sin.sin_family = AF_INET;
      sin.sin_addr.s_addr =INADDR_ANY;;
      sin.sin_port = 0;

      if (bind(sock_d, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errexit("Could't bind to the port: %s\n", strerror(errno));
      if (listen(sock_d,5) < 0)
        errexit("Error in listen function: %s\n", strerror(errno));

      getsockname(sock_d, (struct sockaddr *)&sa, &sa_len);

      fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n", (unsigned int)(sin.sin_addr.s_addr & 0xff), (unsigned int)((sin.sin_addr.s_addr >> 8) & 0xff), (unsigned int)((sin.sin_addr.s_addr >> 16) & 0xff), (unsigned int)((sin.sin_addr.s_addr >> 24) & 0xff), (unsigned int)(sa.sin_port & 0xff),(unsigned int)(sa.sin_port >> 8));

      data_socket = sock_d;
    }

    else if (COMMAND("CWD")) {
      fscanf(fd, "%s", arg);
      printf("--->Comando: CWD %s\n", arg);

      char path[MAX_BUFF];

      if (getcwd(path, sizeof(path)) != NULL){

        strcat(path,"/");
        strcat(path,arg);

        if (chdir(path) < 0)
          fprintf(fd, "550 Failed to change directory.\n");
        else
          fprintf (fd, "250 Directory successfully changed.\n");
      }
    }

    else if (COMMAND("STOR") ) {

      fscanf(fd, "%s", arg);
      printf("---> Command: STOR %s\n", arg);
      FILE* file = fopen(arg,"wb");

      if (!file){
        fprintf(fd, "450 Requested file action not taken.\n");
        close(data_socket);
      }

      else{
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);

        struct sockaddr_in sa;
        socklen_t sa_len = sizeof(sa);
        char buffer[MAX_BUFF];
        int n;

        if (pass)
            data_socket = accept(data_socket,(struct sockaddr *)&sa, &sa_len);

        do{
          n = recv(data_socket, buffer, MAX_BUFF, 0);
          fwrite(buffer, sizeof(char) , n, file);
        } while (n == MAX_BUFF);  //Al ser menor que el maximo deducimos que es el ultimo paquete

        fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
        fclose(file);
        close(data_socket);
      }
    }

    else if (COMMAND("SYST")) {
      printf("--->Comando: SYST\n");
      fprintf(fd, "215 UNIX system type.\n");
    }

    else if (COMMAND("TYPE")) {

      fscanf(fd, "%s", arg);
      printf("--->Comando: TYPE %s\n", arg);
      if (!strcmp(arg,"A"))
        fprintf(fd, "200 Switching to ASCII mode.\n");
      else if (!strcmp(arg,"I"))
        fprintf(fd, "200 Switching to Binary mode.\n");
      else if (!strcmp(arg,"L"))
        fprintf(fd, "200 Switching to Tenex mode.\n");
      else
        fprintf(fd, "501 Syntax error in parameters or arguments.\n");
    }

    else if (COMMAND("RETR")) {
      fscanf(fd, "%s", arg);
      printf("---> Command: RETR %s\n", arg);
      FILE* file = fopen(arg,"rb");
      if (!file){
        fprintf(fd, "450 Requested file action not taken.\n");
        close(data_socket);
      }
      else{
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        struct sockaddr_in sa;
        socklen_t sa_len = sizeof(sa);
        char buffer[MAX_BUFF];
        int n;
        if (pass)
          data_socket = accept(data_socket,(struct sockaddr *)&sa, &sa_len);
        do{
          n = fread(buffer, sizeof(char), MAX_BUFF, file);
          send(data_socket, buffer, n, 0);
        } while (n == MAX_BUFF);  //Una vez no se cumple esta condicion el paquete es el ultimo

        fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
        fclose(file);
        close(data_socket);
      }
    }

    else if (COMMAND("QUIT")) {
      printf("---> Command: QUIT");
      fprintf(fd, "221 Service closing control connection.\n");
      stop();
    }

    else if (COMMAND("LIST")) { //Al escribir un ls se manda LIST
      printf("---> Command: LIST\n");
      fprintf(fd, "125 Data connection already open; transfer starting.\n");

      struct sockaddr_in sa;
      socklen_t sa_len = sizeof(sa);
      char buffer[MAX_BUFF];
      std::string list;
      std::string ls = "ls";

      FILE* file = popen(ls.c_str(), "r");

      if (!file){
        fprintf(fd, "450 Requested file action not taken.\n");
        close(data_socket);
      }

      else{

        if (pass)
        data_socket = accept(data_socket,(struct sockaddr *)&sa, &sa_len);

        while (!feof(file))
        if (fgets(buffer, MAX_BUFF, file) != NULL)
          list.append(buffer);

        send(data_socket, list.c_str(), list.size(), 0);

        fprintf(fd, "250 Requested file action okay, completed.\n");
        pclose(file);
        close(data_socket);
      }
    }

    else  {
      fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
      printf("El comando : %s %s no está implementado\n", command, arg);


    }

  }

  fclose(fd);


  return;

};
