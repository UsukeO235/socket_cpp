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
#include <unistd.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace c_wrapper
{

namespace socket
{

class SocketException
: public std::runtime_error
{
	public:
	SocketException( const char* message )
	: std::runtime_error( message ){}
};

class SocketInitializationFailedException
: public SocketException
{
	public:
	SocketInitializationFailedException( const char* message )
	: SocketException( message ){}
};

class SocketAcceptFailedException
: public SocketException
{
	public:
	SocketAcceptFailedException( const char* message )
	: SocketException( message ){}
};

class SocketConnectFailedException
: public SocketException
{
	public:
	SocketConnectFailedException( const char* message )
	: SocketException( message ){}
};

class SocketBindFailedException
: public SocketException
{
	public:
	SocketBindFailedException( const char* message )
	: SocketException( message ){}
};

class SocketListenFailedException
: public SocketException
{
	public:
	SocketListenFailedException( const char* message )
	: SocketException( message ){}
};

class SocketModeChangeFailedException
: public SocketException
{
	public:
	SocketModeChangeFailedException( const char* message )
	: SocketException( message ){}
};

enum class socket_type
: int
{
	STREAM  = SOCK_STREAM,
	DGRAM = SOCK_DGRAM
};

enum class protocol_family
: int
{
	UNIX = AF_UNIX,
	INET = AF_INET
};

enum class blocking_mode
: int
{
	BLOCKING = 0,
	NON_BLOCKING = 1
};

class Poller;

template< socket_type type >
class Socket;

template<>
class Socket< socket_type::STREAM >
{
	private:
	int socket_ = -1;
	bool established_ = false;

	protocol_family protocol_ = protocol_family::INET;
	blocking_mode mode_ = blocking_mode::BLOCKING;

	struct sockaddr_in server_socket_addr_;
	struct sockaddr_in client_socket_addr_;

	friend Poller;

	int get() const
	{
		return socket_;
	}

	public:
	Socket() = default;
	Socket( const Socket& ) = delete;
	Socket& operator=( const Socket& ) = delete;

	Socket( const protocol_family protocol )
	{
		protocol_ = protocol;
		socket_ = ::socket( static_cast<int>(protocol_), SOCK_STREAM, IPPROTO_TCP );

		if( socket_ < 0 )
		{
			throw SocketInitializationFailedException( "Could not create socket" );
		}

		// 切断後すぐにソケットを再利用可能にするための設定
		int yes = 1;
		if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)(&yes), sizeof(yes)) < 0 )
		{
			throw SocketInitializationFailedException( "setsockopt() failed" );
		}
	}

	/**
	 * @brief Construct a new Socket object
	 * 
	 * @param rhs ムーブ元
	 */
	Socket( Socket&& rhs ) noexcept
	: socket_( rhs.socket_ )
	, established_( rhs.established_ )
	, protocol_( rhs.protocol_ )
	, mode_( rhs.mode_ )
	, server_socket_addr_( rhs.server_socket_addr_ )
	, client_socket_addr_( rhs.client_socket_addr_ )
	{
		rhs.socket_ = -1;
		rhs.established_ = false;
	}

	/**
	 * @brief ムーブ代入演算子
	 * 
	 * @param rhs ムーブ元
	 * @return Socket& ソケット(ムーブ先)
	 */
	Socket& operator=( Socket&& rhs ) noexcept
	{
		if( this != &rhs )
		{
			socket_ = rhs.socket_;
			established_ = rhs.established_;
			protocol_ = rhs.protocol_;
			mode_ = rhs.mode_;
			server_socket_addr_ = rhs.server_socket_addr_;
			client_socket_addr_ = rhs.client_socket_addr_;
			
			rhs.socket_ = -1;
			rhs.established_ = false;
		}
		return *this;
	}

	~Socket() noexcept
	{
		if( socket_ != -1 )
		{
			close( socket_ );
		}
	}

	void change_mode( const blocking_mode mode )
	{
		u_long val = static_cast<u_long>(mode);
		if( ioctl( socket_, FIONBIO, &val ) < 0 )
		{
			throw SocketModeChangeFailedException( "ioctl() fialed" );
		}

		mode_ = mode;
	}

	void bind( const uint16_t port )
	{
		std::memset( &server_socket_addr_, 0, sizeof(server_socket_addr_) );
		server_socket_addr_.sin_family      = static_cast<int>(protocol_);
		server_socket_addr_.sin_addr.s_addr = htonl( INADDR_ANY );  // 後で接続元を制限すること
		server_socket_addr_.sin_port        = htons( port );
		
		if( ::bind(socket_, (struct sockaddr*)(&server_socket_addr_), sizeof(server_socket_addr_) ) < 0 ) 
		{
			// 既に接続が確立しているポートでは bind が失敗する( Address already in use )
			throw SocketBindFailedException( "bind() failed" );
		}
	}

	void listen( const int backlog )
	{
		if( ::listen(socket_, backlog) < 0 )
		{
			throw SocketListenFailedException( "listen() failed" );
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
		Socket< socket_type::STREAM > s;
		s.socket_ = ::accept( socket_, (struct sockaddr *)&s.client_socket_addr_, &client_socket_length );
		if( s.socket_ < 0 )
		{
			throw SocketAcceptFailedException( "accept() failed" );
		}

		s.established_ = true;
		return std::move(s);
	}

	bool connect( const char* ip_address, const uint16_t port )
	{
		if( established_ )
		{
			return true;
		}

		std::memset( &server_socket_addr_, 0, sizeof(server_socket_addr_) );
		server_socket_addr_.sin_family = static_cast<int>(protocol_);
		server_socket_addr_.sin_port = htons( port );

		if( inet_aton( ip_address, &server_socket_addr_.sin_addr ) == 0 )
		{
			throw SocketConnectFailedException( "Invalid IP address" );
		}

		if( ::connect(socket_, (struct sockaddr*) &server_socket_addr_, sizeof(server_socket_addr_)) < 0 )
		{
			if( mode_ == blocking_mode::BLOCKING )
			{
				throw SocketConnectFailedException( "connect() failed" );
			}
			return false;
		}

		// 切断後すぐにソケットを再利用可能にするための設定
		int yes = 1;
		if( setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)(&yes), sizeof(yes)) < 0 )
		{
			throw SocketConnectFailedException( "setsockopt() failed" );
		}

		established_ = true;
		return true;
	}

	int receive( void* const buffer, const std::size_t size )
	{
		return ::recv( socket_, buffer, size, 0 );
	}

	int send( const void* const buffer, const std::size_t size )
	{
		return ::send( socket_, buffer, size, 0 );
	}

};

class PollerException
: public std::runtime_error
{
	public:
	PollerException( const char* message )
	: std::runtime_error( message ){}
};

class PollerAppendFailedException
: public PollerException
{
	public:
	PollerAppendFailedException( const char* message )
	: PollerException( message ){}
};

class PollerRemoveFailedException
: public PollerException
{
	public:
	PollerRemoveFailedException( const char* message )
	: PollerException( message ){}
};

class PollerEventDetectionFailedException
: public PollerException
{
	public:
	PollerEventDetectionFailedException( const char* message )
	: PollerException( message ){}
};

enum class poll_event
: int
{
	NONE = 0,
	ERROR = POLLERR,
	HUNG_UP = POLLHUP,
	IN = POLLIN,
	OUT = POLLOUT
};

constexpr poll_event operator | ( poll_event a, poll_event b )
{
	return static_cast<poll_event>( static_cast<int>(a) | static_cast<int>(b) );
}

constexpr poll_event operator & ( poll_event a, poll_event b )
{
	return static_cast<poll_event>( static_cast<int>(a) & static_cast<int>(b) );
}

class Poller
{
	private:
	const std::size_t max_num_of_fds_;
	std::vector< pollfd > fds_;
	std::vector< pollfd > results_;

	unsigned int index_ = 0;
	std::unordered_map< int, unsigned int > map_;  // fd, index
	
	public:
	Poller( const std::size_t N )
	: max_num_of_fds_( N ) 
	{
		fds_.reserve( N );
		results_.reserve( N );
	}

	void append( const Socket<socket_type::STREAM>& socket, const poll_event events )
	{
		if( index_ >= max_num_of_fds_ )
		{
			throw PollerAppendFailedException( "Could not append fd to fds" );
		}

		fds_.push_back( pollfd{.fd=socket.get(), .events=static_cast<int>(events)} );
		map_[socket.get()] = fds_.size() - 1;  // index
	}

	void append( const Socket<socket_type::DGRAM>& socket )
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

	void remove( const Socket<socket_type::STREAM>& socket )
	{
		auto itr = map_.find( socket.get() );
		if( itr == map_.end() )  // キーが存在しない
		{
			throw PollerRemoveFailedException( "Specified socket not registered" );
		}

		// 指定されたソケットをfds_の中から削除し、fds_を前方に詰める
		fds_[itr->second] = fds_.back();  // 末尾の要素で削除したい要素を上書き
		map_[fds_.back().fd] = itr->second;
		fds_.pop_back();
		map_.erase( itr );
	}

	bool poll( const int timeout=-1 )
	{
		int ret = ::poll( fds_.data(), fds_.size(), timeout );
		/*
		for( unsigned int i = 0; i < max_num_of_fds_; i ++ )
		{
			results_[i] = fds_[i];  // 結果を退避しておく
			fds_[i].revents = 0;  // 受け取ったイベントのみ初期化
		}
		*/
		results_ = fds_;  // 結果を退避しておく
		for( unsigned int i = 0; i < fds_.size(); i ++ )
		{
			fds_[i].revents = 0;  // 受け取ったイベントのみ初期化
		}
		
		if( ret <= 0 )
		{
			return false;
		}
		return true;
	}

	bool is_event_detected( const Socket<socket_type::STREAM>& socket )
	{
		auto itr = map_.find( socket.get() );
		if( itr == map_.end() )  // キーが存在しない
		{
			throw PollerEventDetectionFailedException( "Specified socket not registered" );
		}

		if( results_[ itr->second ].revents == 0 )  // イベントの発生なし
		{
			return false;
		}

		return true;
	}

	poll_event event_detected( const Socket<socket_type::STREAM>& socket )
	{
		auto itr = map_.find( socket.get() );
		if( itr == map_.end() )  // キーが存在しない
		{
			throw PollerEventDetectionFailedException( "Specified socket not registered" );
		}

		return static_cast<poll_event>( results_[ itr->second ].revents );
	}
	
};

}
}

#endif