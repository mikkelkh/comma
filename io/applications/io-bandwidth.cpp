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
#include <numeric>
#include <boost/algorithm/string/replace.hpp>
#include <boost/array.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../io/select.h"
#include "../../io/stream.h"

static const double default_window = 1.0f;
static const double default_window_resolution = 0.1f;
static const double default_update_interval = 1.0f;
static const char default_delimiter = ',';
static const std::string standard_output_fields="timestamp,received_bytes,bandwidth/all_time,bandwidth/window";
static const std::string extended_output_fields="timestamp,received_bytes,bandwidth/all_time,bandwidth/window,records_per_second/all_time,records_per_second/window";

static void bash_completion( unsigned const ac, char const * const * av )
{
    static const char* completion_options =
        " --help -h"
        " --size -s --window -w --update -u --resolution -r"
        " --delimiter -d --output-fields"
        ;
    std::cout << completion_options << std::endl;
    exit( 0 );
}

void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "pass stdin to stdout and echo bandwidth to stderr" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: io-bandwidth [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --size,-s=[<bytes>]: specify size of one record of input data" << std::endl;
    std::cerr << "    --window,-w=[<n>]: sliding window; default=" << default_window << "s" << std::endl;
    std::cerr << "    --update,-u=[<n>]: update interval; default=" << default_update_interval << "s" << std::endl;
    std::cerr << "    --resolution,-r=[<n>]: sliding window resolution; default=" << default_window_resolution << "s" << std::endl;
    std::cerr << "    --output-fields: list output fields and exit" << std::endl;
    std::cerr << "    --delimiter,-d <delimiter>: default ','" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    The sliding window consists of a number of buckets. The width of each" << std::endl;
    std::cerr << "    bucket is given by --resolution, and there are sufficient buckets to" << std::endl;
    std::cerr << "    encompass the requested --window." << std::endl;
    std::cerr << std::endl;
    std::cerr << "output" << std::endl;
    std::cerr << "    The standard output fields are:" << std::endl;
    std::cerr << "        " << boost::replace_all_copy( standard_output_fields, ",", "\n        " ) << std::endl;
    std::cerr << std::endl;
    std::cerr << "    But if the --size option is used then they are extended to:" << std::endl;
    std::cerr << "        " << boost::replace_all_copy( extended_output_fields, ",", "\n        " ) << std::endl;
    std::cerr << std::endl;
    std::cerr << "    Use --output-fields to see these fields programatically" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    basics:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "        while : ; do echo 1; sleep 0.1; done | io-bandwidth > /dev/null" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    outputting records/second:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "        while : ; do" << std::endl;
    std::cerr << "            echo 1,2 | csv-to-bin 2ui; sleep 0.1" << std::endl;
    std::cerr << "        done | io-bandwidth --size=$( csv-size 2ui ) > /dev/null" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    mocking up a more complex input stream:" << std::endl;
    std::cerr << "    pass data to hexdump and publish bandwidth stats on port 8888" << std::endl;
    std::cerr << std::endl;
    std::cerr << "        while : ; do" << std::endl;
    std::cerr << "            dd if=/dev/urandom bs=100 count=1 2> /dev/null; sleep 0.1" << std::endl;
    std::cerr << "        done | io-bandwidth 2> >( io-publish tcp:8888 ) | hexdump" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

static const boost::posix_time::time_duration wait_interval = boost::posix_time::milliseconds( 10 );

// todo: optionally use exponential moving average

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        if( options.exists( "--bash-completion" ) ) bash_completion( ac, av );

        if( options.exists( "--output-fields" ))
        {
            if( options.exists( "--size" )) { std::cout << extended_output_fields << std::endl; }
            else { std::cout << standard_output_fields << std::endl; }
            return 0;
        }

        // Functionally equivalent to boost::optional< std::size_t > record_size
        // but eliminates the gcc "maybe-uninitialized" warning
        boost::optional< std::size_t > record_size = boost::make_optional< std::size_t >( false, 0 );
        if( options.exists( "--size" )) { record_size = options.value< std::size_t >( "--size" ); }

        boost::posix_time::time_duration update_interval = boost::posix_time::microseconds( options.value< double >( "--update,-u", default_update_interval ) * 1000000 );
        double window = options.value< double >( "--window,-w", default_window );
        double bucket_width = options.value< double >( "--resolution,-r", default_window_resolution );
        boost::posix_time::time_duration bucket_duration = boost::posix_time::microseconds( bucket_width * 1000000 );
        char delimiter = options.value( "--delimiter,-d", default_delimiter );

        comma::io::select select;
        select.read().add( comma::io::stdin_fd );
        comma::io::istream is( "-", comma::io::mode::binary );

        unsigned long long total_bytes = 0;
        unsigned int bucket_bytes = 0;
        boost::circular_buffer< unsigned int > window_buckets( std::ceil( window / bucket_width ));

        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::ptime next_update = start_time + update_interval;
        boost::posix_time::ptime next_bucket = start_time + bucket_duration;

        bool end_of_stream = false;
        boost::array< char, 65536 > buffer;
        std::ios_base::sync_with_stdio( false ); // unsync to make rdbuf()->in_avail() working
        std::cin.tie( NULL ); // std::cin is tied to std::cout by default
        
        while( !end_of_stream )
        {
            select.wait( wait_interval );
            while( is->rdbuf()->in_avail() || ( select.check() && select.read().ready( is.fd() ) && is->good() ) )
            {
                std::size_t available = is->rdbuf()->in_avail(); // std::size_t available = is.available_on_file_descriptor();
                if( available == 0 ) { end_of_stream = true; break; }
                std::size_t size = std::min( available, buffer.size() );
                is->read( &buffer[0], size );
                bucket_bytes += size;
                total_bytes += size;
                std::cout.write( &buffer[0], size );
                std::cout.flush();
            }

            boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

            if( now >= next_bucket )
            {
                window_buckets.push_back( bucket_bytes );
                bucket_bytes = 0;
                next_bucket += bucket_duration;
                // If there's been a large pause (for some reason), catch up
                if( now > next_bucket )
                {
                    window_buckets.clear();
                    next_bucket = now + bucket_duration;
                }
            }

            if( now >= next_update && !window_buckets.empty() )
            {
                double elapsed_time = double( ( now - start_time ).total_milliseconds() ) / 1000.0f;
                double bandwidth = (double)total_bytes / elapsed_time;
                double window_bandwidth = (double)std::accumulate( window_buckets.begin()
                                                                 , window_buckets.end()
                                                                 , 0.0f )
                                                  / window_buckets.size() / bucket_width;

                std::cerr << boost::posix_time::to_iso_string( now )
                          << std::fixed
                          << delimiter << total_bytes
                          << delimiter << bandwidth
                          << delimiter << window_bandwidth;
                std::cerr.unsetf( std::ios_base::floatfield );
                if( record_size )
                {
                    std::cerr << delimiter << bandwidth / *record_size
                              << delimiter << window_bandwidth / *record_size;
                }
                std::cerr << std::endl;

                next_update += update_interval;
                // If there's been a large pause (for some reason), catch up
                if( now > next_update ) { next_update = now + update_interval; }
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "io-bandwidth: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "io-bandwidth: unknown exception" << std::endl; }
    return 1;
}
