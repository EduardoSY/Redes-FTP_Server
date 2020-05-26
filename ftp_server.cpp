/*
REDES Y SISTEMAS DISTRIBUIDOS
Universidad de La Laguna
Curso 19/20
Autor: Eduardo Da Silva Yanes
*/

#include <iostream>
#include <signal.h>
#include <string.h>

#include "FTPServer.h"

FTPServer *server;



extern "C" void sighandler(int signal, siginfo_t *info, void *ptr){

    std::cout << "\nSaliendo..." << std::endl;
    server->stop();
    exit(-1);
}


void exit_handler(){

    server->stop();
}


int main(int argc, char **argv){

            struct sigaction action;

            action.sa_sigaction = sighandler;
            action.sa_flags = SA_SIGINFO;
            sigaction(SIGINT, &action , NULL);

            server = new FTPServer(2121);
            std::cout<<"Server corriendo en puerto 2121\n";
            atexit(exit_handler);
            server->run();

}
