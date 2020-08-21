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

#include "socket.hpp"
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <signal.h>

volatile sig_atomic_t running = true;

void handler( int )
{
	running = false;
}

int main()
{
	try
	{
		c_wrapper::socket::Socket< c_wrapper::socket::socket_type::STREAM > producer( c_wrapper::socket::protocol_family::INET );
		std::vector<c_wrapper::socket::Socket< c_wrapper::socket::socket_type::STREAM >> sockets;
		sockets.reserve(4);

		c_wrapper::socket::Poller poller(5);

		struct sigaction action = {};
		action.sa_handler = handler;
		sigaction( SIGINT, &action, NULL );

		producer.bind( 50000 );
		producer.listen( 5 );

		poller.append( producer, c_wrapper::socket::poll_event::IN );

		while( running )
		{
			if( poller.poll() )
			{
				for( auto itr = sockets.begin(); itr != sockets.end(); ++ itr )
				{
					if( poller.is_event_detected( *itr ) )
					{
						std::cout << "Event detected on comm socket: " << static_cast<int>(poller.event_detected(*itr)) << std::endl;
					}
				}

				if( poller.is_event_detected( producer ) )
				{
					std::cout << static_cast<int>(poller.event_detected(producer)) << std::endl;
					
					if( sockets.size() < 4 )
					{
						std::cout << "New socket created" << std::endl;
						sockets.push_back( producer.accept() );
						poller.append( sockets.back(), c_wrapper::socket::poll_event::IN | c_wrapper::socket::poll_event::HUNG_UP );
					}
				}
			}
			else
			{
				std::cout << "poll() returned false" << std::endl;
			}	
		}

		std::cout << "Aborted" << std::endl;
	}
	catch( const std::exception& e )
	{
		std::cerr << e.what() << '\n';
	}
	

	return 0;
}