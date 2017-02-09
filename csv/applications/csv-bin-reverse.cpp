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

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "../../application/command_line_options.h"
#include "../../csv/format.h"
#include "../../string/string.h"

using namespace comma;

static void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "for given fields, reverse byte order (e.g. if your computer uses little endian, and you got big-endian data)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat blah.bin | csv-bin-reverse <format> [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --complement: complement fields, same as in cut utility" << std::endl;
    std::cerr << "    --fields,-f=<fields>: field numbers, starting from 1 (to keep" << std::endl;
    std::cerr << "                          consistent with the standard cut utility)" << std::endl;
    std::cerr << "                          field name support: todo; meanwhile use something like:" << std::endl;
    std::cerr << "                          csv-bin-reverse --fields=$( echo ,,hello,,,world | csv-fields numbers )" << std::endl;
    std::cerr << "                          default: reverse byte order of all fields" << std::endl;
    std::cerr << "    --flush: if present, flush stdout after each record" << std::endl;
    std::cerr << "    --verbose,-v: more output, e.g. --help --verbose" << std::endl;
    std::cerr << std::endl;
    if( verbose ) { std::cerr << csv::format::usage() << std::endl << std::endl; }
    exit( 0 );
}

int main( int ac, char** av )
{
    #ifdef WIN32
    _setmode( _fileno( stdin ), _O_BINARY ); /// @todo move to a library
    _setmode( _fileno( stdout ), _O_BINARY ); /// @todo move to a library
    #endif
    try
    {
        command_line_options options( ac, av, usage );
        if( ac < 2 ) { usage(); }
        comma::csv::format format( av[1] );
        bool flush = options.exists( "--flush" );
        std::vector< std::string > v = comma::split( options.value< std::string >( "--fields,-f", std::string( "1-" ) + boost::lexical_cast< std::string >( format.count() ) ), ',' );
        std::vector< comma::csv::format::element > offsets;
        std::set< std::size_t > set;
        std::vector< std::size_t > indices;
        for( unsigned int i = 0; i < v.size(); ++i )
        {
            std::vector< std::string > r = comma::split( v[i], '-' );
            if( r.size() == 2 )
            {
                std::size_t begin = boost::lexical_cast< std::size_t >( r[0] );
                if( begin == 0 ) { std::cerr << "csv-bin-reverse: field numbers start with 1 (to keep it consistent with linux cut utility)" << std::endl; return 1; }
                --begin;
                std::size_t end = boost::lexical_cast< std::size_t >( r[1] );
                if( end <= begin ) { std::cerr << "csv-bin-reverse: expected range, got: " << v[i] << std::endl; return 1; }
                for( unsigned int k = begin; k < end; ++k ) { indices.push_back( k ); set.insert( k ); }
            }
            else
            {
                std::size_t index = boost::lexical_cast< std::size_t >( v[i] );
                if( index == 0 ) { std::cerr << "csv-bin-reverse: field numbers start with 1 (to keep it consistent with linux cut utility)" << std::endl; return 1; }
                --index;
                if( set.find( index ) != set.end() ) { std::cerr << "csv-bin-reverse: duplicated index " << v[i] << std::endl; return 1; }
                indices.push_back( index );
                set.insert( index );
            }
        }
        if( options.exists( "--complement" ) ) { for( unsigned int i = 0; i < format.count(); ++i ) { if( set.find( i ) == set.end() ) { offsets.push_back( format.offset( i ) ); } } }
        else { for( unsigned int i = 0; i < indices.size(); ++i ) { offsets.push_back( format.offset( indices[i] ) ); } }
        std::vector< char > buf( format.size() );
        while( std::cin.good() && !std::cin.eof() )
        {
            std::cin.read( &buf[0], format.size() );
            if( std::cin.gcount() == 0 ) { continue; }
            if( std::cin.gcount() < int( format.size() ) ) { std::cerr << "csv-bin-reverse: expected " << format.size() << " bytes, got only " << std::cin.gcount() << std::endl; return 1; }
            for( unsigned int i = 0; i < offsets.size(); ++i )
            {
                char* p = &buf[0] + offsets[i].offset;
                char* r = p + offsets[i].size - 1;
                for( ; p < r ; ++p, --r ) { std::swap( *p, *r ); }
            }
            std::cout.write( &buf[0], buf.size() );
            if( flush ) { std::cout.flush(); }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-bin-reverse: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-bin-reverse: unknown exception" << std::endl; }
    usage();
}

