// This file is part of comma, a generic and flexible library
// Copyright (c) 2016 The University of Sydney
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

/// @author dave jennings

#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../csv/options.h"
#include "../../csv/stream.h"
#include "../../io/select.h"
#include "../../io/stream.h"

static void bash_completion( unsigned const ac, char const * const * av )
{
    static const char* completion_options =
        " --help -h --verbose -v"
        " --timeout --period"
        " --append-fields --append -a"
        " --output-fields --output-format"
        ;
    std::cout << completion_options << std::endl;
    exit( 0 );
}

void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "pass stdin to stdout, repeating the last record after a period of inactivity" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: csv-repeat [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --help,-h: help; --help --verbose: more help" << std::endl;
    std::cerr << "    --timeout,-t=<seconds>: timeout before repeating the last record" << std::endl;
    std::cerr << "    --period=[<seconds>]: period of repeated record" << std::endl;
    std::cerr << "    --append-fields,--append,-a=[<fields>]: add extra fields to output" << std::endl;
    std::cerr << "    --output-fields: print output fields and exit" << std::endl;
    std::cerr << "    --output-format: print output format and exit" << std::endl;
    std::cerr << "    --verbose,-v: more output" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    if --period are not set --timeout acts as a watchdog. If no input is seen" << std::endl;
    std::cerr << "    within the timeout csv-repeat exits with an error." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --append fields are appended to output; supported fields are:" << std::endl;
    std::cerr << "        time: append timestamp" << std::endl;
    std::cerr << "        repeating: 1 if currently repeating" << std::endl;
    std::cerr << std::endl;
    if( verbose )
    {
        std::cerr << "csv options:" << std::endl;
        std::cerr << comma::csv::options::usage() << std::endl;
    }
    else { std::cerr << "    see --help --verbose for more details" << std::endl << std::endl; }
    std::cerr << "examples" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    { echo -e \"1\\n2\\n3\"; sleep 10; } | csv-repeat --timeout=3 --period=1" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    { echo -e \"1\\n2\\n3\"; sleep 10; } \\" << std::endl;
    std::cerr << "        | csv-repeat --timeout=3 --period=1 --append=time,repeating" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    { echo -e \"1,2,3\\n4,5,6\\n7,8,9\"; sleep 10; } \\" << std::endl;
    std::cerr << "        | csv-to-bin 3d --flush \\" << std::endl;
    std::cerr << "        | csv-repeat --timeout=3 --period=1 --binary=3d \\" << std::endl;
    std::cerr << "        | csv-from-bin 3d --flush" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    { echo -e \"1\\n2\\n3\"; sleep 10; } | csv-repeat --timeout=3" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

struct output_t
{
    boost::posix_time::ptime time;
    bool repeating;
    output_t() : repeating( false ) {}
    output_t( const boost::posix_time::ptime& time, bool repeating ) : time( time ), repeating( repeating ) {}
};

namespace comma { namespace visiting {

template <> struct traits< output_t >
{
    template < typename K, typename V > static void visit( const K&, const output_t& p, V& v )
    {
        v.apply( "time", p.time );
        v.apply( "repeating", p.repeating );
    }
    template < typename K, typename V > static void visit( const K&, output_t& p, V& v )
    {
        v.apply( "time", p.time );
        v.apply( "repeating", p.repeating );
    }
};
    
} } // namespace comma { namespace visiting {

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        if( options.exists( "--bash-completion" ) ) bash_completion( ac, av );

        comma::csv::options csv = comma::csv::options( options );

        std::size_t record_size = csv.binary() ? csv.format().size() : 0;
        std::vector< char > buffer( csv.binary() ? ( 65536ul / record_size + 1 ) * record_size : 0 );
        char* buffer_begin = &buffer[0];
        const char* buffer_end = &buffer[0] + buffer.size();
        char* read_position = buffer_begin;
        char* write_position = buffer_begin;
        char* last_record = NULL;

        comma::io::select select;
        select.read().add( comma::io::stdin_fd );
        comma::io::istream is( "-", comma::io::mode::binary );
        boost::scoped_ptr< comma::csv::output_stream< output_t > > ostream;

        comma::csv::options output_csv;

        if( options.exists( "--append-fields,--append,-a" ) )
        {
            output_csv.fields = options.value< std::string >( "--append-fields,--append,-a", "" );
            if( csv.binary() )
            {
                std::string format;
                if( output_csv.fields.empty() )
                {
                    format = comma::csv::format::value< output_t >();
                }
                else
                {
                    const std::vector< std::string >& v = comma::split( output_csv.fields, ',' );
                    std::string comma;
                    for( unsigned int i = 0; i < v.size(); ++i )
                    {
                        if( v[i] == "repeating" ) { format += comma + 'b'; }
                        else if( v[i] == "time" ) { format += comma + 't'; }
                        else { std::cerr << "csv-repeat: expected one of: " << comma::join( comma::csv::names< output_t >( false ), ',' ) << "; got: \"" << v[i] << "\"" << std::endl; return 1; }
                        comma = ",";
                    }
                }
                output_csv.format( format );
            }
            output_csv.flush = true;
            output_csv.delimiter = csv.delimiter;
            ostream.reset( new comma::csv::output_stream< output_t >( std::cout, output_csv ) );
        }

        if( options.exists( "--output-fields" ))
        {
            if( options.exists( "--fields,-f" ))
            {
                std::cout << options.value< std::string >( "--fields,-f" );
                if( !output_csv.fields.empty() ) { std::cout << csv.delimiter << output_csv.fields; }
                std::cout << std::endl;
                return 0;
            }
            else
            {
                std::cerr << "csv-repeat: --output-fields option requires --fields" << std::endl;
                return 1;
            }
        }

        if( options.exists( "--output-format" ))
        {
            if( options.exists( "--binary" ))
            {
                std::cout << options.value< std::string >( "--binary" );
                if( options.exists( "--append-fields,--append,-a" )) { std::cout << csv.delimiter << output_csv.format().string(); }
                std::cout << std::endl;
                return 0;
            }
            else
            {
                std::cerr << "csv-repeat: --output-format option requires --binary" << std::endl;
                return 1;
            }
        }

        boost::posix_time::time_duration timeout;
        boost::optional< boost::posix_time::time_duration > period;

        timeout = boost::posix_time::microseconds( options.value< double >( "--timeout,-t" ) * 1000000 );
        if( options.exists( "--period" )) { period = boost::posix_time::microseconds( options.value< double >( "--period" ) * 1000000 ); }
        bool end_of_stream = false;
        std::string line;
        std::ios_base::sync_with_stdio( false ); // unsync to make rdbuf()->in_avail() working
        std::cin.tie( NULL ); // std::cin is tied to std::cout by default
        bool repeating = false;
        while( is->good() && !end_of_stream )
        {
            select.wait( repeating ? *period : timeout );
            repeating = true;
            while( is->rdbuf()->in_avail() > 0 || ( select.check() && select.read().ready( is.fd() ) ) )
            {
                end_of_stream = true;
                if( csv.binary() )
                {
                    std::size_t available = is->rdbuf()->in_avail(); //std::size_t available = is.available_on_file_descriptor();
                    std::size_t bytes_to_read = std::min( available, ( std::size_t )( buffer_end - read_position ));
                    is->read( read_position, bytes_to_read );
                    std::size_t bytes_read = is->gcount();
                    if( bytes_read <= 0 ) { break; }
                    read_position += bytes_read;
                    while( read_position - write_position >= ( std::ptrdiff_t )record_size )
                    {
                        std::cout.write( write_position, record_size );
                        if( ostream ) { ostream->write( output_t( boost::posix_time::microsec_clock::universal_time(), false ) ); }
                        else { std::cout.flush(); }
                        last_record = write_position;
                        write_position += record_size;
                    }
                    if( read_position == buffer_end )
                    {
                        read_position = write_position = buffer_begin;
                    }
                }
                else
                {
                    std::getline( std::cin, line );
                    if( line.empty() ) { break; }
                    std::cout << line;
                    if( ostream )
                    {
                        std::cout << csv.delimiter;
                        ostream->write( output_t( boost::posix_time::microsec_clock::universal_time(), false ) );
                    }
                    else { std::cout << std::endl; }
                }
                end_of_stream = repeating = false;
            }

            if( !is->good() || end_of_stream ) { break; }

            if( repeating )
            {
                if( !period ) { std::cerr << "csv-repeat: input data timed out" << std::endl; return 1; }
                if( csv.binary() )
                {
                    if( last_record )
                    {
                        std::cout.write( last_record, record_size );
                        if( ostream ) { ostream->write( output_t( boost::posix_time::microsec_clock::universal_time(), true ) ); }
                    }
                }
                else
                {
                    if( !line.empty() )
                    {
                        std::cout << line;
                        if( ostream )
                        {
                            std::cout << csv.delimiter;
                            ostream->write( output_t( boost::posix_time::microsec_clock::universal_time(), true ) );
                        }
                        else { std::cout << std::endl; }
                    }
                }
                std::cout.flush();
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-repeat: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-repeat: unknown exception" << std::endl; }
    return 1;
}
