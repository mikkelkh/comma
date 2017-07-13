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

#include <iostream>
#include <string>
#include <set>
#include "../../application/contact_info.h"
#include "../../application/command_line_options.h"
#include "../../csv/format.h"

using namespace comma;
static const char *app_name = "csv-format";

static void usage()
{
    std::cerr << std::endl;
    std::cerr << "Usage: echo \"3f,2f,d\" | " << app_name << " [options] (expand|collapse|count|repeat)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Perform various operations on format strings." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Operations:" << std::endl;
    std::cerr << "    expand: output fully expand format, e.g. 2i,3f -> i,i,f,f,f" << std::endl;
    std::cerr << "    collapse: output minimal format, e.g. i,i,f,f,f -> 2i,3f" << std::endl;
    std::cerr << "    count: output number of fields, e.g. 2i,3f -> 5" << std::endl;
    std::cerr << "    repeat: replicate the format n times, e.g. 2i,3f --count 2 -> 2i,3f,2i,3f" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "    repeat:" << std::endl;
    std::cerr << "        --count=n: replicate the format n times" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 1 );
}

int main( int ac, char** av )
{
    try
    {
        command_line_options options( ac, av );
        if( options.exists( "--help,-h" ) ) { usage(); }
        const std::vector< std::string > unnamed = options.unnamed( "--help,-h", "-.*" );
        if( unnamed.empty() )
        { std::cerr << app_name << ": expected an operation (e.g. \"expand\")" << std::endl; return 1; }
        if( unnamed.size() != 1 ) { usage(); }
        std::string operation = unnamed[0];
        std::string line;
        while ( std::getline( std::cin, line ) )
        {
            comma::csv::format format( line );
            if( operation == "expand" )        { std::cout << format.expanded_string() << std::endl; }
            else if( operation == "collapse" ) { std::cout << format.collapsed_string() << std::endl; }
            else if( operation == "count" ) { std::cout << format.count() << std::endl; }
            else if( operation == "repeat" ) {
                const unsigned int count = options.value< unsigned int >( "--count" );
                if ( count ) {
                    std::cout << format.string();
                    for ( unsigned int i = 0; i < count - 1; ++i ) { std::cout << "," << format.string(); }
                    std::cout << std::endl;
                }
            }
            else { std::cerr << app_name << ": unknown operation \"" << operation << "\"" << std::endl; return 1; }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << app_name << ": " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << app_name << ": unknown exception" << std::endl; }
    return 1;
}
