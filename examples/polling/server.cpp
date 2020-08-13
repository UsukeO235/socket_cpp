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

#if 0
int main()
{
	int server_socket;
	int client_sockets[2] = {-1, -1};
	struct sockaddr_in server_socket_addr;
	struct sockaddr_in client_socket_addr;
	unsigned short port;
	unsigned int client_socket_length;

	/*
	if((port = (unsigned short)atoi( "" )) == 0)
	{
		return -1;
	}
	*/

	if((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		printf( "socket() failed\n" );
		return -1;
	}

	// 切断後すぐにソケットを再利用可能にするための設定
	int yes = 1;
	if( setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)(&yes), sizeof(yes)) < 0 )
	{
		close( server_socket );
		return -1;
	}

	std::memset( &server_socket_addr, 0, sizeof(server_socket_addr) );
	server_socket_addr.sin_family      = AF_INET;
	server_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_socket_addr.sin_port        = htons(50000);

	if(bind(server_socket, (struct sockaddr *) &server_socket_addr, sizeof(server_socket_addr) ) < 0 )
	{
		printf( "bind() failed\n" );
		return -1;
	}

	if(listen(server_socket, 5) < 0)
	{
		printf( "listen() failed\n" );
		return -1;
	}

	// http://www.eyes-software.co.jp/news/archives/7
	int maxfd;
	fd_set rfds;
	struct timeval tv;

	while(1)
	{
		FD_ZERO( &rfds );
        FD_SET( server_socket, &rfds );
		maxfd = server_socket;

		for( unsigned int i = 0; i < 2; i ++ )
		{
			if( client_sockets[i] != -1 )  // 接続が確立されるまでは実行されないようにする
			{
				FD_SET( client_sockets[i], &rfds );
				if( client_sockets[i] > maxfd )
				{
					maxfd = client_sockets[i];
				}
			}
		}

		// タイムアウト時間を10sec+500000μsec に指定する。
		tv.tv_sec = 10;
		tv.tv_usec = 500000;

		int count = select( maxfd+1, &rfds, NULL, NULL, &tv );

		if( count < 0 )  // シグナル受信
		{
			printf( "Signal received\n" );
			close( server_socket );
			return 0;
		}
		else if( count == 0 )  // タイムアウト
		{
			printf( "Timeout\n" );
			continue;
		}
		else
		{
			if( FD_ISSET( server_socket, &rfds ) )
			{
                // 接続されたならクライアントからの接続を確立する
                for( unsigned int i = 0; i < 2; i++ )
				{
                    if( client_sockets[i] == -1 )
					{
						if(( client_sockets[i] = accept(server_socket, (struct sockaddr *)&client_socket_addr, &client_socket_length )) < 0 )
						{
							close( server_socket );
							printf( "accept() failed\n" );
							return -1;
						}

						printf( "accept from %s\n", inet_ntoa(client_socket_addr.sin_addr) );
					}
				}
			}

			for( unsigned int i = 0; i < 2; i ++ )
			{
				if( FD_ISSET( client_sockets[i], &rfds ) )
				{
					// 受信

				}
			}

		}
		
	}
	

	return 0;
}
#else

int main()
{
	try
	{
		c_wrapper::socket::Socket< c_wrapper::socket::socket_type::STREAM > sock( c_wrapper::socket::protocol_family::INET );

		sock.bind( 50000 );
		sock.listen( 5 );

		c_wrapper::socket::Poller poller(2);

		poller.append( sock );
		poller.poll( -1 );
		
		if( poller.is_event_detected( sock ) )
		{
			if( sock.accept() )
			{
				std::cout << "Succeeded !" << std::endl;
			}
		}
		else
		{
			std::cerr << "No event detected" << '\n';
		}
		
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	

	return 0;
}

#endif