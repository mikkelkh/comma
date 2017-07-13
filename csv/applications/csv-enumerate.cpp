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

#include <boost/unordered_map.hpp>
#include "../../application/command_line_options.h"
#include "../../base/exception.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../csv/impl/unstructured.h"
#include "../../string/string.h"

static void usage( bool verbose )
{
    std::cerr << std::endl;
    std::cerr << "append unique id to csv records with the same values; support integer, time, and string fields" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat data.csv | csv-enumerate <options>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "todo: support floating point values" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --fields,-f=<fields>; fields of interest, actual field names do not matter; e.g: --fields ,,,a,,b,,,c" << std::endl;
    std::cerr << "    --format=<binary format>; if input is ascii and deducing data types may be ambiguous, define field types explicitly, value as in --binary" << std::endl;
    std::cerr << "    --output-map,--map: do not output input records, only the list of key values, corresponding ids (as ui), and value count (as ui)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "csv options" << std::endl;
    if( verbose ) { std::cerr << comma::csv::options::usage() << std::endl; } else { std::cerr << "    run csv-enumerate --help --verbose for more..." << std::endl; }
    std::cerr << std::endl;
    exit( 0 );
}

int main( int ac, char** av )
{
    typedef comma::csv::impl::unstructured input_t;
    typedef boost::unordered_map< comma::csv::impl::unstructured, std::pair< comma::uint32, comma::uint32 >, comma::csv::impl::unstructured::hash >  map_t;
    try
    {
        comma::command_line_options options( ac, av, usage );
        bool verbose = options.exists( "--verbose,-v" );
        bool output_map = options.exists( "--output-map,--map" );
        comma::csv::options csv( options );
        csv.full_xpath = true;
        std::string first_line;
        comma::csv::format f;
        if( csv.binary() ) { f = csv.format(); }
        else if( options.exists( "--format" ) ) { f = comma::csv::format( options.value< std::string >( "--format" ) ); }
        else
        {
            while( std::cin.good() && first_line.empty() ) { std::getline( std::cin, first_line ); }
            if( first_line.empty() ) { return 0; }
            f = comma::csv::impl::unstructured::guess_format( first_line, csv.delimiter );
            if( verbose ) { std::cerr << "csv-enumerate: guessed format: " << f.string() << std::endl; }
        }
        input_t default_input;
        std::vector< std::string > v = comma::split( csv.fields, ',' );
        for( unsigned int i = 0; i < v.size(); ++i )
        {
            if( v[i].empty() ) { continue; }
            v[i] = default_input.append( f.offset( i ).type );
        }
        if( verbose ) { std::cerr << "csv-enumerate: fields " << csv.fields << " interpreted as: " << comma::join( v, ',' ) << std::endl; }
        csv.fields = comma::join( v, ',' );
        comma::csv::input_stream< input_t > istream( std::cin, csv, default_input );
        #ifdef WIN32
        if( istream.is_binary() ) { _setmode( _fileno( stdout ), _O_BINARY ); }
        #endif
        static map_t map;
        comma::uint32 id = 0;
        if( !first_line.empty() )
        { 
            input_t input = comma::csv::ascii< input_t >( csv, default_input ).get( first_line );
            map[ comma::csv::ascii< input_t >( csv, default_input ).get( first_line ) ] = std::make_pair( id++, 1 );
            if( !output_map ) { std::cout << first_line << csv.delimiter << 0 << std::endl; }
        }
        while( istream.ready() || std::cin.good() )
        {
            const input_t* p = istream.read();
            if( !p ) { break; }
            map_t::iterator it = map.find( *p );
            comma::uint32 cur = id;
            if( it == map.end() ) { map[ *p ] = std::make_pair( id++, 1 ); } else { cur = it->second.first; ++( it->second.second ); }
            if( !output_map )
            {
                if( csv.binary() )
                {
                    std::cout.write( istream.binary().last(), csv.format().size() );
                    std::cout.write( reinterpret_cast< const char* >( &cur ), sizeof( comma::uint32 ) );
                    if( csv.flush ) { std::cout.flush(); }
                }
                else
                {
                    std::cout << comma::join( istream.ascii().last(), csv.delimiter ) << csv.delimiter << cur << std::endl;
                }
            }
        }
        comma::csv::options output_csv;
        output_csv.delimiter = csv.delimiter;
        output_csv.full_xpath = true;
        if( csv.binary() ) { output_csv.format( comma::csv::format::value< input_t >( default_input ) + "2ui" ); }
        comma::csv::output_stream< map_t::value_type > ostream( std::cout, output_csv, std::make_pair( default_input, std::make_pair( 0, 0 ) ) );
        if( output_map ) { for( map_t::const_iterator it = map.begin(); it != map.end(); ++it ) { ostream.write( *it ); } }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-enumerate: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-enumerate: unknown exception" << std::endl; }
    return 1;
}
