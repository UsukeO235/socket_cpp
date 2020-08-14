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
#ifndef SOCKET_H_
#define SOCKET_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>

#include <memory>
#include <unordered_map>
#include <exception>
#include <stdexcept>

namespace c_wrapper
{

namespace socket
{

enum socket_type
{
	STREAM,  // TCP
	DGRAM,  // UDP
};

enum protocol_family
{
	UNIX,  // PF_UNIX
	INET,  // PF_INET
};

enum blocking_mode
{
	BLOCKING,
	NON_BLOCKING,
};

class Poller;

template< socket_type type >
class Socket;

template<>
class Socket< socket_type::STREAM >
{
	private:
	int socket_ = -1;
	int client_socket_ = -1;

	bool established_ = false;

	blocking_mode mode_;

	struct sockaddr_in server_socket_addr_;
	struct sockaddr_in client_socket_addr_;

	friend Poller;

	int get() const
	{
		if( established_ )
		{
		    return (client_socket_==-1)? socket_ : client_socket_;
		}
		else
		{
		    return socket_;
		}
	}

	public:
	Socket() = default;

	Socket( const protocol_family protocol )
	{
		if( protocol == protocol_family::INET )
		{
			socket_ = ::socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
		}
		else
		{
			socket_ = ::socket( PF_UNIX, SOCK_STREAM, IPPROTO_TCP );
		}

		if( socket_ < 0 )
		{
			std::runtime_error( "Could not create socket" );
		}

		// 切断後すぐにソケットを再利用可能にするための設定
		int yes = 1;
		if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)(&yes), sizeof(yes)) < 0 )
		{
			close( socket_ );
			throw std::runtime_error( "setsockopt() failed" );
		}
	}

	~Socket() noexcept
	{
		close( socket_ );
		close( client_socket_ );
	}

	void change_mode( const blocking_mode mode )
	{
		if( mode == blocking_mode::NON_BLOCKING )
		{
			// ソケットをノンブロッキングモードに設定
			u_long val = 1;
			if( !established_ )
			{
				if( ioctl( socket_, FIONBIO, &val ) < 0 )
				{
					close( socket_ );
					throw std::runtime_error( "ioctl() fialed" );
				}
			}
			else
			{
				if( client_socket_ >= 0)
				{
					if( ioctl( client_socket_, FIONBIO, &val ) < 0 )
					{
						close( socket_ );
						close( client_socket_ );
						throw std::runtime_error( "ioctl() fialed" );
					}
				}
				else
				{
					if( ioctl( socket_, FIONBIO, &val ) < 0 )
					{
						close( socket_ );
						throw std::runtime_error( "ioctl() fialed" );
					}
				}
			}
		}
		else
		{
			close( socket_ );
			close( client_socket_ );
			throw std::runtime_error( "Not implemented" );
		}

		mode_ = mode;
	}

	void bind( const uint16_t port )
	{
		std::memset( &server_socket_addr_, 0, sizeof(server_socket_addr_) );
		server_socket_addr_.sin_family      = AF_INET;
		server_socket_addr_.sin_addr.s_addr = htonl( INADDR_ANY );  // 後で接続元を制限すること
		server_socket_addr_.sin_port        = htons( port );
		
		if( ::bind(socket_, (struct sockaddr*)(&server_socket_addr_), sizeof(server_socket_addr_) ) < 0 ) 
		{
			// 既に接続が確立しているポートでは bind が失敗する( Address already in use )
			close( socket_ );
			throw std::runtime_error( "bind() failed" );
		}
	}

	void listen( const int backlog )
	{
		if( ::listen(socket_, backlog) < 0 )
		{
			close( socket_ );
			throw std::runtime_error( "listen() failed" );
		}
	}

	Socket accept()
	{
		/*
		* -- non-blocking modeのときにacceptが失敗する可能性について --
		* (1) accept失敗で例外を投げることの是非
		* non-blocking modeにおいてacceptが失敗するのは例外ではないと考える
		* 従って例外を投げるべきではない
		* (2) accept()のインターフェース設計
		* blocking mode、non-blocking modeともに同じインターフェースであるのが好ましい
		* new_socket = accept()でクライアントと接続されたソケットを受け取りたい
		* non-blocking modeでは以下のようにすればよい
		* try{
		*     poller.poll( 0 );  // タイムアウト値は0
		*     if( poller.is_event_detected( socket ) ){  // socketはnon-blocking modeに設定されている
		*         new_socket = socket.accept()
		*     }
		*     // new_socketを使ってクライアントと通信できる
		* } catch(...){
		*     // socketに対するイベントが発生しているにも関わらずacceptが失敗した
		*     // これは例外的な状況と考える
		*     // acceptが完了する前にクライアント側がソケットを閉じるとacceptに失敗する可能性はある
		*     // 要調査
		* }
		* 
		*/
	
		unsigned int client_socket_length;
		client_socket_ = ::accept( socket_, (struct sockaddr *)&client_socket_addr_, &client_socket_length );
		if( client_socket_<0 )
		{
			close( socket_ );
			throw std::runtime_error( "accept() failed" );
		}

		established_ = true;
		return *this;
	}

	bool connect( const char* ip_address, const uint16_t port )
	{
		if( established_ )
		{
			return true;
		}

		std::memset( &server_socket_addr_, 0, sizeof(server_socket_addr_) );
		server_socket_addr_.sin_family = AF_INET;
		server_socket_addr_.sin_port = htons( port );

		if( inet_aton( ip_address, &server_socket_addr_.sin_addr ) == 0 )
		{
			close( socket_ );
			throw std::runtime_error( "Invalid IP address" );
		}

		if( ::connect(socket_, (struct sockaddr*) &server_socket_addr_, sizeof(server_socket_addr_)) < 0 )
		{
			if( mode_ == blocking_mode::BLOCKING )
			{
				close( socket_ );
				throw std::runtime_error( "connect() failed" );
			}
			return false;
		}

		// 切断後すぐにソケットを再利用可能にするための設定
		int yes = 1;
		if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)(&yes), sizeof(yes)) < 0 )
		{
			close( socket_ );
			throw std::runtime_error( "setsockopt() failed" );
		}

		established_ = true;
		client_socket_ = socket_;
		return true;
	}

};

enum poll_event
{
	ERROR,
	HUNG_UP,
	IN,
	OUT,
};

class Poller
{
	private:
	const std::size_t max_num_of_fds_;
	std::unique_ptr< pollfd[] > fds_;
	std::unique_ptr< pollfd[] > results_;

	unsigned int index_ = 0;
	std::unordered_map< int, unsigned int > map_;  // fd, index
	
	public:
	Poller( const std::size_t N )
	: max_num_of_fds_( N ) 
	, fds_( new pollfd[N] )
	, results_( new pollfd[N] )
	{
		std::memset( &(fds_[0]), 0, sizeof(pollfd)*N );
		std::memset( &(results_[0]), 0, sizeof(pollfd)*N );
	}

	void append( Socket<socket_type::STREAM>& socket )
	{
		if( index_ >= max_num_of_fds_ )
		{
			throw std::runtime_error( "Could not append fd to fds" );
		}

		fds_[index_].fd = socket.get();
		fds_[index_].events = POLLERR | POLLHUP | POLLIN | POLLOUT;

		map_[socket.get()] = index_;
		index_ ++;
	}

	void append( Socket<socket_type::DGRAM>& socket )
	{
		throw std::runtime_error( "Not implemented" );
		/*
		if( ss_.size()+ds_.size() >= N )
		{
			throw std::runtime_error( "Could not append fd to fds" );
		}

		fds_[ss_.size()+ds_.size()].fd = socket.get();
		fds_[ss_.size()+ds_.size()].events = POLLIN | POLLERR;

		ds_.push_back( &socket );
		*/
	}

	void remove( Socket<socket_type::STREAM>& socket )
	{
		auto itr = map_.find( socket.get() );
		if( itr == map_.end() )  // キーが存在しない
		{
			throw std::runtime_error( "Specified socket not registered" );
		}

		// fds_の中から指定されたソケットを削除し、fds_を前方に詰める
		// (1) fds_ = {5, x, 3}, index_==3, itr->second==1
		// (2) fds_ = {5, 3}, index_==2, itr==NULL
		std::memmove( fds_.get(), fds_.get()+(itr->second)+1, sizeof(pollfd)*(index_-(itr->second)-1) );
		map_.erase( itr );
		index_ --;
	}

	bool poll( const int timeout=-1 )
	{
		int ret = ::poll( fds_.get(), index_, timeout );

		std::memset( &(results_[0]), 0, sizeof(pollfd)*max_num_of_fds_ );
		std::memcpy( results_.get(), fds_.get(), sizeof(pollfd)*max_num_of_fds_ );
		std::memset( &(fds_[0]), 0, sizeof(pollfd)*max_num_of_fds_ );
		index_ = 0;

		if( ret <= 0 )
		{
			return false;
		}
		return true;
	}

	bool is_event_detected( Socket<socket_type::STREAM>& socket )
	{
		auto itr = map_.find( socket.get() );
		if( itr == map_.end() )  // キーが存在しない
		{
			throw std::runtime_error( "Specified socket not registered" );
		}

		if( results_[ itr->second ].revents == 0 )  // イベントの発生なし
		{
			return false;
		}

		//map_.erase( itr );
		return true;
	}

	
};

}
}

#endif