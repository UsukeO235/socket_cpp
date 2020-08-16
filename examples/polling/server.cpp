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
			c_wrapper::socket::Socket< c_wrapper::socket::socket_type::STREAM > new_sock;
			new_sock = sock.accept();

			char s[256] = {};
			std::cout << new_sock.receive( s, 256 ) << " bytes received. ";
			std::cout << "Received message: " << s << std::endl;
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