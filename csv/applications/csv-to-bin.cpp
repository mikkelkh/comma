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
#include <iostream>
#include "../../application/contact_info.h"
#include "../../application/command_line_options.h"
#include "../../csv/format.h"
#include "../../string/string.h"

//#include <google/profiler.h>

using namespace comma;

static void usage( bool )
{
    std::cerr << std::endl;
    std::cerr << "usage: cat blah.csv | csv-to-bin <format> [options] > blah.bin" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --delimiter=[<delimiter>]; default: , (comma)" << std::endl;
    std::cerr << "    --flush; flush stdout after each record" << std::endl;
    std::cerr << std::endl;
    std::cerr << csv::format::usage() << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

int main( int ac, char** av )
{
    #ifdef WIN32
    _setmode( _fileno( stdout ), _O_BINARY ); /// @todo move to a library
    #endif
    std::string line;
    line.reserve( 4000 );
    try
    {
        command_line_options options( ac, av, usage );
        char delimiter = options.value( "--delimiter", ',' );
        bool flush = options.exists( "--flush" );
        comma::csv::format format( av[1] );
        //{ ProfilerStart( "csv-to-bin.prof" );
        while( std::cin.good() && !std::cin.eof() )
        {
            std::getline( std::cin, line );
            if( !line.empty() && *line.rbegin() == '\r' ) { line = line.substr( 0, line.length() - 1 ); } // windows... sigh...
            if( !line.empty() ) { format.csv_to_bin( std::cout, line, delimiter, flush ); }
        }
        //ProfilerStop(); }
        return 0;
    }
    catch( std::exception& ex )
    {
        std::cerr << "csv-to-bin: " << ex.what() << std::endl;
        std::cerr <<   "format: " << av[1]
                  << "\ninput: " << line
                  << "\n============================================" << std::endl;
    }
    catch( ... )
    {
        std::cerr << "csv-to-bin: unknown exception" << std::endl;
    }
    return 1;
}
