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


/// @author cedric wohlleber

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <iostream>
#include <string>
#include <boost/optional.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/types.h"
#include "../../csv/format.h"

static void usage()
{
    std::cerr << std::endl;
    std::cerr << "prepend input with timestamp" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat a.csv | csv-time-stamp [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << "    --binary,-b=<format>: binary format" << std::endl;
    std::cerr << "    --size=<size>: binary input of size" << std::endl;
    std::cerr << "    --delimiter,-d <delimiter>: ascii only; default ','" << std::endl;
    std::cerr << "    --local: if present, local time; default: utc" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples:" << std::endl;
    std::cerr << "    cat input.csv | csv-time-stamp" << std::endl;
    std::cerr << "    cat input.bin | csv-time-stamp --binary=3ui" << std::endl;
    std::cerr << "    cat input.bin | csv-time-stamp --size=12" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( -1 );
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av );
        if( options.exists( "--help" ) || options.exists( "-h" ) ) { usage(); }

        bool local = options.exists( "--local" );
        char delimiter = options.value( "--delimiter,-d", ',' );

        boost::optional< comma::csv::format > format;
        if( options.exists( "--binary,-b" ))
        {
            format = comma::csv::format( options.value< std::string >( "--binary,-b" ));
        }
        bool binary = options.exists( "--binary,-b,--size" );
        std::size_t size = options.value( "--size", 0 );
        if( binary && size == 0 ) { size = format->size(); }

        #ifdef WIN32
        if( binary )
        {
            _setmode( _fileno( stdin ), _O_BINARY );
            _setmode( _fileno( stdout ), _O_BINARY );
        }
        #endif

        if( binary )
        {
            boost::array< char, 65536 > buf;
            char* begin = &buf[0];
            const char* end = begin + ( buf.size() / size ) * size;
            char* cur = begin;
            unsigned int offset = 0;
            while( std::cin.good() && !std::cin.eof() )
            {
                if( offset >= size )
                {
                    boost::posix_time::ptime now = local ? boost::posix_time::microsec_clock::local_time() : boost::posix_time::microsec_clock::universal_time();
                    static const unsigned int time_size = comma::csv::format::traits< boost::posix_time::ptime, comma::csv::format::time >::size;
                    static char timestamp[ time_size ];
                    comma::csv::format::traits< boost::posix_time::ptime, comma::csv::format::time >::to_bin( now, timestamp );
                    for( ; offset >= size; cur += size, offset -= size )
                    {
                        std::cout.write( ( char* )( &timestamp ), time_size );
                        std::cout.write( cur, size );
                    }
                    std::cout.flush();
                    if( cur == end ) { cur = begin; }
                }
                int r = ::read( 0, cur + offset, end - cur - offset );
                if( r <= 0 ) { break; }
                offset += r;
            }            
        }
        else
        {
            while( std::cin.good() && !std::cin.eof() )
            {
                std::string line;
                std::getline( std::cin, line );
                if( line.empty() ) { continue; }
                boost::posix_time::ptime now = local ? boost::posix_time::microsec_clock::local_time() : boost::posix_time::microsec_clock::universal_time();
                std::cout << boost::posix_time::to_iso_string( now ) << delimiter << line << std::endl;
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-time-stamp: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-time-stamp: unknown exception" << std::endl; }
    usage();
}
