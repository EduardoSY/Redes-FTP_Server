all: ftp_server

ftp_server: ClientConnection.cpp FTPServer.cpp ftp_server.cpp
	g++ -g -std=c++11  ClientConnection.cpp FTPServer.cpp ftp_server.cpp -o ftp_server -lpthread

clean:
	$(RM) ftp_server *~
