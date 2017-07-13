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
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../name_value/parser.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static void usage()
{
    std::cerr << std::endl;
    std::cerr << "subtract given value from timestamp" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat a.csv | csv-time-delay <seconds> [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<options>" << std::endl;
    std::cerr << comma::csv::options::usage( "t" ) << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( -1 );
}

struct Point
{
    boost::posix_time::ptime timestamp;
    Point() {}
    Point( const boost::posix_time::ptime& timestamp ) : timestamp( timestamp ) {}
};

namespace comma { namespace visiting {

template <> struct traits< Point >
{
    template < typename K, typename V > static void visit( const K&, const Point& p, V& v ) { v.apply( "t", p.timestamp ); }
    template < typename K, typename V > static void visit( const K&, Point& p, V& v ) { v.apply( "t", p.timestamp ); }
};
    
} } // namespace comma { namespace visiting {

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av );
        if( options.exists( "--help" ) || options.exists( "-h" ) || ac == 1 ) { usage(); }
        const std::vector< std::string >& v = options.unnamed( "--flush", "--binary,-b,--delimiter,-d,--fields,-f,--full-xpath,--precision,--quote" );
        if( v.empty() ) { std::cerr << "csv-time-delay: expected time delay, got none" << std::endl; return 1; }
        double d = boost::lexical_cast< double >( v[0] );
        int sign = d < 0 ? -1 : 1;
        int minutes = int( std::floor( std::abs( d ) / 60 ) );
        int seconds = int( std::floor( std::abs( d ) - double( minutes ) * 60 ) );
        int microseconds = int( ( std::abs( d ) - ( double( minutes ) * 60 + seconds ) ) * 1000000 );
        minutes *= sign;
        seconds *= sign;
        microseconds *= sign;
        boost::posix_time::time_duration delay = boost::posix_time::minutes( minutes ) + boost::posix_time::seconds( seconds ) + boost::posix_time::microseconds( microseconds );
        comma::csv::options csv( options );
        comma::csv::input_stream< Point > istream( std::cin, csv );
        comma::csv::output_stream< Point > ostream( std::cout, csv );
        while( std::cin.good() && !std::cin.eof() )
        {
            const Point* p = istream.read();
            if( !p ) { break; }
            Point q = *p;
            q.timestamp = p->timestamp + delay;
            if( csv.binary() ) { ostream.write( q, istream.binary().last() ); }
            else { ostream.write( q, istream.ascii().last() ); }
        }
        return 0;     
    }
    catch( std::exception& ex ) { std::cerr << "csv-time-delay: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-time-delay: unknown exception" << std::endl; }
    usage();
}
