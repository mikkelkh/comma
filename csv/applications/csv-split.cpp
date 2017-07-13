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

#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include "../../application/contact_info.h"
#include "../../csv/impl/program_options.h"
#include "../../csv/traits.h"
#include "split/split.h"

comma::csv::options csv;
boost::optional< boost::posix_time::time_duration > duration;
std::string suffix;
unsigned int size = 0;

bool passthrough;

template < typename T >
void run()
{
    comma::csv::applications::split< T > split( duration, suffix, csv, passthrough );
    if( size == 0 )
    {
        std::string line;
        while( std::cin.good() && !std::cin.eof() )
        {
            std::getline( std::cin, line );
            if( line.empty() ) { break; }
            split.write( line );
        }
    }
    else
    {
        #ifdef WIN32
            _setmode( _fileno( stdin ), _O_BINARY );
        #endif
        std::vector< char > packet( size );
        while( std::cin.good() && !std::cin.eof() )
        {
            std::cin.read( &packet[0], size );
            if( std::cin.gcount() > 0 ) { split.write( &packet[0], size ); }
        }
    }
}

int main( int argc, char** argv )
{
    try
    {
        double period = 0;
        std::string extension;
        boost::program_options::options_description description( "options" );
        description.add_options()
            ( "help,h", "display help message" )
            ( "size,c", boost::program_options::value< unsigned int >( &size ), "packet size, only full packets will be written" )
            ( "period,t", boost::program_options::value< double >( &period ), "period in seconds after which a new file is created" )
            ( "suffix,s", boost::program_options::value< std::string >( &extension ), "filename extension; default will be csv or bin, depending whether it is ascii or binary" )
            ( "string", "id is string; default: 32-bit integer" )
            ( "time", "id is time; default: 32-bit integer" )
            ( "passthrough,pass", "pass data through to stdout" );
        description.add( comma::csv::program_options::description() );
        boost::program_options::variables_map vm;
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, description), vm );
        boost::program_options::parsed_options parsed = boost::program_options::command_line_parser(argc, argv).options( description ).allow_unregistered().run();
        boost::program_options::notify( vm );
        if ( vm.count( "help" ) || vm.count( "long-help" ) )
        {
            std::cerr << std::endl;
            std::cerr << "read from stdin by packet or by line and split the data into files, named by field value or time (if split by time)" << std::endl;
            std::cerr << "usage: csv-split [options]" << std::endl;
            std::cerr << std::endl;
            std::cerr << description << std::endl;
            std::cerr << std::endl;
            std::cerr << "fields to split by listed in descending precedence" << std::endl;
            std::cerr << "    block: split on the block number change" << std::endl;
            std::cerr << "    id: split by id (same as block, except does not have to be contiguous by the price of worse performance)" << std::endl;
            std::cerr << "    t: if present, use timestamp from the packet; if absent, use system time" << std::endl;
            std::cerr << std::endl;
            std::cerr << comma::contact_info << std::endl;
            std::cerr << std::endl;
            return 1;
        }
        csv = comma::csv::program_options::get( vm );
        if( csv.binary() ) { size = csv.format().size(); }
        bool id_is_string = vm.count( "string" );
        bool id_is_time = vm.count( "time" );
        passthrough = vm.count("passthrough");
        
        if( id_is_string && id_is_time ) { std::cerr << "csv-split: either --string or --time" << std::endl; }
        if( period > 0 ) { duration = boost::posix_time::microseconds( period * 1e6 ); }
        if( extension.empty() ) { suffix = csv.binary() || size > 0 ? ".bin" : ".csv"; }
        else { suffix += "."; suffix += extension; }
        if( id_is_string ) { run< std::string >(); }
        else if( id_is_time ) { run< boost::posix_time::ptime >(); }
        else { run< comma::uint32 >(); }
        return 0;
    }
    catch( std::exception& ex )
    {
        std::cerr << argv[0] << ": " << ex.what() << std::endl;
    }
    catch( ... )
    {
        std::cerr << argv[0] << ": unknown exception" << std::endl;
    }
    return 1;
}
