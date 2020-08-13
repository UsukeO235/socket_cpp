#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include <stdio.h>

#include "socket.hpp"
#include <iostream>
/*
int main()
{
	int client_socket;
	struct sockaddr_in server_socket_addr; //server internet socket address
	unsigned short server_port = 50000;

	std::memset( &server_socket_addr, 0, sizeof(server_socket_addr) );
	server_socket_addr.sin_family = AF_INET;

	// 127.0.0.1またはNICに割り当てられたIPアドレス
	if( inet_aton("127.0.0.1", &server_socket_addr.sin_addr) == 0 )
	{
		printf( "inet_aton() failed\n" );
		return -1;
	}

	server_socket_addr.sin_port = htons( server_port );

	if( (client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		printf( "socket() failed\n" );
		return -1;
	}

	if( connect(client_socket, (struct sockaddr*) &server_socket_addr, sizeof(server_socket_addr)) < 0 )
	{
		printf( "connect() failed\n" );
		close( client_socket );
		return -1;
	}

	printf( "connect to %s\n", inet_ntoa(server_socket_addr.sin_addr) );

	close( client_socket );

	return 0;
}
*/
int main()
{
	try
	{
		c_wrapper::socket::Socket< c_wrapper::socket::socket_type::STREAM > sock( c_wrapper::socket::protocol_family::INET );
		sock.connect( "127.0.0.1", 50000 );
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	

	return 0;
}