// This file is part of comma, a generic and flexible library
// Copyright (c) 2011 The University of Sydney
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Sydney nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


/// @author vsevolod vlaskine

#ifndef WIN32
#include <stdlib.h>
#endif
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include "../../application/contact_info.h"
#include "../../application/command_line_options.h"
#include "../../base/types.h"
#include "../../csv/format.h"

void usage()
{
    std::cerr << "simple udp client: receives udp packets and outputs them on stdout" << std::endl;
    std::cerr << std::endl;
    std::cerr << "rationale: netcat and socat somehow do not work very well with udp" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: udp-client <port> [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<options>" << std::endl;
    std::cerr << "    --ascii: output timestamp as ascii; default: 64-bit binary" << std::endl;
    std::cerr << "    --binary: output timestamp as 64-bit binary; default" << std::endl;
    std::cerr << "    --delimiter=<delimiter>: if ascii and --timestamp, use this delimiter; default: ','" << std::endl;
    std::cerr << "    --size=<size>: hint of maximum buffer size; default 16384" << std::endl;
    std::cerr << "    --reuse-addr,--reuseaddr: reuse udp address/port" << std::endl;
    std::cerr << "    --timestamp: output packet timestamp (currently just system time)" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 1 );
}

int main( int argc, char** argv )
{
    comma::command_line_options options( argc, argv );
    if( argc < 2 || options.exists( "--help,-h" ) ) { usage(); }
    const std::vector< std::string >& unnamed = options.unnamed( "--ascii,--binary,--reuse-addr,--reuseaddr,--timestamp", "--delimiter,--size" );
    if( unnamed.empty() ) { std::cerr << "udp-client: please specify port" << std::endl; return 1; }
    unsigned short port = boost::lexical_cast< unsigned short >( unnamed[0] );
    bool timestamped = options.exists( "--timestamp" );
    bool binary = !options.exists( "--ascii" );
    char delimiter = options.value( "--delimiter", ',' );
    std::vector< char > packet( options.value( "--size", 16384 ) );
    boost::asio::io_service service;
    boost::asio::ip::udp::socket socket( service );
    socket.open( boost::asio::ip::udp::v4() );
    boost::system::error_code error;
    socket.set_option( boost::asio::ip::udp::socket::broadcast( true ), error );
    if( error ) { std::cerr << "udp-client: failed to set broadcast option on port " << port << std::endl; return 1; }
    if( options.exists( "--reuse-addr,--reuseaddr" ) )
    {
        socket.set_option( boost::asio::ip::udp::socket::reuse_address( true ), error );
        if( error ) { std::cerr << "udp-client: failed to set reuse address option on port " << port << std::endl; return 1; }
    }
    socket.bind( boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), port ), error );
    if( error ) { std::cerr << "udp-client: failed to bind port " << port << std::endl; return 1; }

    #ifdef WIN32
    if( binary )
    {
        _setmode( _fileno( stdout ), _O_BINARY );        
    }
    #endif
    
    while( std::cout.good() )
    {
        boost::system::error_code error;
        std::size_t size = socket.receive( boost::asio::buffer( packet ), 0, error );
        if( error || size == 0 ) { break; }
        if( timestamped )
        {
            boost::posix_time::ptime timestamp = boost::posix_time::microsec_clock::universal_time();
            BOOST_STATIC_ASSERT( sizeof( boost::posix_time::ptime ) == sizeof( comma::uint64 ) );
            if( binary )
            { 
                static char buf[ sizeof( comma::int64 ) ];
                comma::csv::format::traits< boost::posix_time::ptime, comma::csv::format::time >::to_bin( timestamp, buf );
                std::cout.write( buf, sizeof( comma::int64 ) );
            }
            else
            {
                std::cout << boost::posix_time::to_iso_string( timestamp ) << delimiter;
            }
        }
        std::cout.write( &packet[0], size );
        std::cout.flush();
   }
   return 0;
}
