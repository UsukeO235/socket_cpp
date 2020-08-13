/*
MIT License

Copyright (c) 2020 UsukeO235

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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