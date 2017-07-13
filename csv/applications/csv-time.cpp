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
/// @author mathew hounsell

#include <string.h>
#include <time.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include "../../application/contact_info.h"
#include "../../application/command_line_options.h"
#include "../../base/exception.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../csv/impl/epoch.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static void usage( bool )
{
    static char const * const msg_general =
        "\n"
        "\nConvert between a couple of common time representation"
        "\n"
        "\nUsage:"
        "\n    cat log.csv | csv-time <options> > converted.csv"
        "\n"
        "\nOptions"
        "\n    --from <what>: input format: any, iso, seconds, sql, xsd, local; default iso"
        "\n    --to <what>: output format: iso, seconds, sql, xsd, local; default iso"
        "\n    --delimiter,-d <delimiter> : default: ','"
        "\n    --fields <fields> : time field names or field numbers as in \"cut\""
        "\n                        e.g. \"1,5,7\" or \"a,b,,d\""
        "\n                        defaults to \"a\" (first field only is datetime)"
        "\n    --empty-as-not-a-date-time,--accept-empty,-e: if time field is empty, consider it as not-a-date-time"
        "\n"
        "\nTime formats"
        "\n    - iso, iso-8601-basic"
        "\n            YYYYMMDDTHHMMSS.FFFFFF, e.g. 20140101T001122.333"
        "\n    - sql, posix, ieee-std-1003.1"
        "\n            e.g. 2014-01-01 00:11:22"
        "\n    - xsd, iso-8601-extended"
        "\n            used in xsd:dateTime, xs:dateTime, gml and derivatives"
        "\n            e.g. 2014-12-25T00:00:00.000Z"
        "\n                 2014-12-25T00:00:00.000+11:00"
        "\n                 2014-12-25T00:00:00.000+1100"
        "\n                 2014-12-25T00:00:00.000+11"
        "\n    - seconds"
        "\n            seconds since UNIX epoch as double"
        "\n    - microseconds, us"
        "\n            microseconds since UNIX epoch as integer"
        "\n    - any, guess"
        "\n            a special input format - try to convert from all those supported,"
        "\n            default input format, will be slower"
        "\n    - format"
        "\n            user given time format, for e.g 'format;%Y%m%dT%H%M%S' will also convert to/from iso format."
        "\n            see date manual for details about time format specifications."
        "\n    local"
        "\n            same as iso but converts from/to local time adjusted using current machine settings"
        "\n"
        "\nDeprecated options:"
        "\n    --to-seconds,--sec,-s: iso input expected; use --from, --to"
        "\n    --to-iso-string,--iso,-i: input as seconds expected; use --from, --to"
        "\n"
        "\n";
    std::cerr << msg_general << comma::contact_info << std::endl << std::endl;
    exit( 0 );
}

enum what_t { guess, iso, seconds, microseconds, sql, xsd, local, format };
static what_t from = guess;
static what_t to = iso;
static std::string from_format;
static std::string to_format;
static bool accept_empty;
static const std::string not_a_date_time_string = boost::posix_time::to_iso_string( boost::posix_time::ptime() );

static std::string to_string( const boost::posix_time::ptime& t, what_t w );

static what_t what( const std::string& option, const comma::command_line_options& options )
{
    std::string s = options.value< std::string >( option, "iso" );
    if(! s.empty() )
    {
        if( 'i' == s[0] )
        {
            if( "iso" == s ) return iso;
            if( "iso-8601-basic" == s ) return iso;
            if( "iso-8601-extended" == s ) { return xsd; }
            if( "ieee-std-1003.1" == s ) return sql;
        }
        else if( 'p' == s[0] )
        {
            if( "posix" == s ) return sql;
        }
        else if( 's' == s[0] )
        {
            if( "sql" == s ) return sql;
            if( "seconds" == s ) return seconds;
        }
        else if( 'm' == s[0] )
        {
            if( "microseconds" == s ) return microseconds;
        }
        else if ('u' == s[0] )
        {
            if( "us" == s ) return microseconds;
        }
        else if( 'x' == s[0] )
        {
            if( "xsd" == s ) { return xsd; }
        }
        else if( 'g' == s[0] )
        {
            if( "guess" == s ) { return guess; }
        }
        else if( 'a' == s[0] )
        {
            if( "any" == s ) { return guess; }
        }
        else if( 'f' == s[0] )
        {
            std::string::size_type fmt_loc = s.find_first_of(';');
            if( "format" == s.substr( 0, fmt_loc ) && std::string::npos != fmt_loc )
            {
                std::string fmt( s.substr( fmt_loc + 1 ) );
                if( ! fmt.empty() )
                {
                    if ( "--from" == option ) { from_format = fmt; return format; }
                    if ( "--to" == option ) { to_format = fmt; return format; }
                }
            }
            std::cerr << "csv-time: unable to get the custom time format in option '" << option << "'" << std::endl;
            exit( 1 );
        }

        else if(s=="local")
            return local;
    }
    std::cerr << "csv-time: expected seconds, sql, or iso; got: \"" << s << "\"" << std::endl;
    exit( 1 );
}

static boost::posix_time::ptime from_string_xsd( const std::string& s )
{
    std::string t = s;

    // Set the delimiter between date and time to be what time_from_string expects
    const size_t idx_t = t.find( 'T' );
    if ( std::string::npos != idx_t ) t[idx_t] = ' ';

    // Determine the timezone offset. Could be any of Z,+-hh:mm,+-hhmm,+-hh
    signed hrs = 0;
    signed mins = 0;
    const size_t start_of_time = ( idx_t != std::string::npos ? idx_t : 8 );
    const size_t start_of_timezone = t.find_first_of( "Z+-", start_of_time );
    if( start_of_timezone != std::string::npos )
    {
        if( t[start_of_timezone] != 'Z' ) // No action required for 'Z'
        {
            signed multiple = ( t[start_of_timezone] == '+' ? -1 : 1 ); // multiple is the reverse of the sign

            const size_t start_of_offset_hours = start_of_timezone + 1;
            // Check for the presence of an optional colon, ie, either 11:00 or 1100
            const bool has_colon_sep = ( t.find_first_of( ":", start_of_offset_hours ) != std::string::npos );
            const size_t start_of_offset_mins = start_of_timezone + 3 + ( has_colon_sep ? 1 : 0 );

            hrs = multiple * boost::lexical_cast<unsigned>( std::string( &t[start_of_offset_hours], 2 ) );
            if (hrs < -12 || hrs > 12 ) COMMA_THROW( comma::exception, "hours must be [0..12]" );
            if( start_of_offset_mins != t.length() )
            {
                mins = multiple * boost::lexical_cast<unsigned>( std::string( &t[start_of_offset_mins], 2 ) );
                if (mins < -60 || mins > 60 ) COMMA_THROW( comma::exception, "minutes must be [0..60]" );
            }
        }
        t.resize( start_of_timezone );
    }
    // Construct the time from the string and apply the offset
    boost::posix_time::ptime result = boost::posix_time::time_from_string( t );
    result += boost::posix_time::hours(hrs) + boost::posix_time::minutes(mins);
    return result;
}

static boost::posix_time::ptime from_string( const std::string& s, const what_t w )
{
    if ( s.empty() )
    {
        if( accept_empty ) { return boost::posix_time::not_a_date_time; }
        COMMA_THROW( comma::exception, "expected non-empty field, got empty; use --accept-empty" );
    }
    switch( w )
    {
        case iso:
            return s == not_a_date_time_string ? boost::posix_time::not_a_date_time : boost::posix_time::from_iso_string( s );
        case local:
        {
            boost::posix_time::ptime l=boost::posix_time::from_iso_string(s);
            tm tm=boost::posix_time::to_tm(l);
            time_t utc=mktime(&tm);
            boost::posix_time::ptime day0(boost::gregorian::date(1970,1,1), boost::posix_time::time_duration(0,0,0));
            return boost::posix_time::from_time_t(utc) + boost::posix_time::microseconds((l-day0).total_microseconds() % 1000000);
        }

        case seconds:
        {
            std::cerr.precision( 20 );
            double d = boost::lexical_cast< double >( s );
            long long seconds = d;
            // ::round() does not compile on windows for c++03
            int microseconds = ::ceil( ( d - seconds ) * 1000000 - 0.5 ); //int microseconds = ::round( ( d - seconds ) * 1000000 ); // although ::round() is slow, have to round, since lexical cast has floating point jitter, e.g. try: boost::lexical_cast< double >( "1369179610.752231000" );
            return boost::posix_time::ptime( comma::csv::impl::epoch, boost::posix_time::seconds( seconds ) + boost::posix_time::microseconds( microseconds ) );
        }

        case microseconds:
        {
            comma::int64 microseconds = boost::lexical_cast< comma::int64 >( s );
            return boost::posix_time::ptime( comma::csv::impl::epoch, boost::posix_time::microseconds( microseconds ) );
        }
        
        case sql:
            return s == "NULL" || s == "null" ? boost::posix_time::not_a_date_time : boost::posix_time::time_from_string( s );

        case xsd: // 2014-03-05T23:00:00.000Z
            return from_string_xsd( s );

        case format:
        {
            std::istringstream is( s );
            boost::posix_time::ptime pt;
            is.exceptions(std::ios_base::failbit);
            is.imbue( std::locale( std::cin.getloc(), new boost::posix_time::time_input_facet( from_format ) ) );
            is >> pt;
            return pt;
        }

        case guess:
            try
            { 
                return from_string( s, iso );
            }
            catch( ... )
            {
                try
                { 
                    return from_string( s, xsd );
                }
                catch( ... )
                {
                    try
                    { 
                        return from_string( s, sql );
                    }
                    catch( ... )
                    {
                        try
                        { 
                            return from_string( s, seconds );
                        }
                        catch( std::exception& ex ) { COMMA_THROW( comma::exception, "expected time, got: \"" << s << "\"; " << ex.what() ); }
                        catch( ... ) { COMMA_THROW( comma::exception, "expected time, got: \"" << s << "\"; unknown exception" ); }
                    }
                }
            }
    }
    COMMA_THROW( comma::exception, "never here" );
}

std::string to_string( const boost::posix_time::ptime& t, what_t w )
{
    switch( w )
    {
        case iso:
            return boost::posix_time::to_iso_string( t );

        case local:
            return boost::posix_time::to_iso_string(boost::date_time::c_local_adjustor<boost::posix_time::ptime>::utc_to_local(t));
            
        case seconds: // quick and dirty
        {
            const boost::posix_time::ptime base( comma::csv::impl::epoch );
            const boost::posix_time::time_duration d = t - base;
            comma::int64 seconds = d.total_seconds();
            comma::int32 nanoseconds = ( d - boost::posix_time::seconds( seconds ) ).total_microseconds() * 1000;
            std::ostringstream oss;
            oss << ( seconds == 0 && nanoseconds < 0 ? "-" : "" ) << seconds;
            if( nanoseconds != 0 )
            {
                oss << '.';
                oss.width( 9 );
                oss.fill( '0' );
                oss << std::abs( nanoseconds );
            }
            return oss.str();
        }
        
        case microseconds:
        {
            const boost::posix_time::ptime base( comma::csv::impl::epoch );
            const boost::posix_time::time_duration d = t - base;
            comma::int64 microseconds = d.total_microseconds();
            std::ostringstream oss;
            oss << ( microseconds < 0 ? "-" : "" ) << microseconds;
            return oss.str();
        }

        case sql:
            return t.is_not_a_date_time() ? std::string( "NULL" ) : comma::split( boost::replace_all_copy( boost::posix_time::to_iso_extended_string( t ), "T", " " ), '.' )[0];

        case xsd: // 2014-03-05T23:00:00.000Z
            return boost::posix_time::to_iso_extended_string( t );

        case format:
        {
            std::ostringstream os;
            os.imbue( std::locale( std::cout.getloc(), new boost::posix_time::time_facet( to_format.c_str() ) ) );
            os << t;
            return os.str();
        }

        case guess:
            COMMA_THROW( comma::exception, "never here" );
    }
    COMMA_THROW( comma::exception, "never here" );
}

struct input_t { std::vector< std::string > values; };

namespace comma { namespace visiting {

template <> struct traits< input_t >
{
    template < typename K, typename V > static void visit( const K&, const input_t& p, V& v ) { v.apply( "values", p.values ); }

    template < typename K, typename V > static void visit( const K&, input_t& p, V& v ) { v.apply( "values", p.values ); }
};

} } // namespace comma { namespace visiting {

static comma::csv::options csv;
static input_t input;

static void init_input()
{
    const std::vector< std::string >& original_names = comma::split( csv.fields, ',' );

    std::vector< std::string > names;
    names.reserve( original_names.size() );
    for( unsigned i = 0; i < original_names.size(); ++i )
        names.push_back( comma::strip( original_names[i], ' ' ) );
    
    std::vector< bool > keep;
    
    bool legacy = true;
    for( unsigned i = 0; i < names.size() && legacy; ++i )
    {
        if( names[i].empty() )
        {
            legacy = false;
            break;
        }
        try {
            const unsigned idx = boost::lexical_cast< unsigned >( names[i] );
            if ( keep.size() < idx ) keep.resize(idx, false);
            keep[idx - 1] = true;
        } catch( ... ) {
            legacy = false;
        }
    }
    
    std::string fields;
    std::string comma;
    unsigned size = 0;
    if ( legacy )
    {
        for( unsigned i = 0; i < keep.size(); ++i )
        {
            fields += comma;
            comma = ",";
            if( keep[i] )
                fields += "values[" + boost::lexical_cast< std::string >( size++ ) + "]";
        }
    }
    else
    {
        for( unsigned i = 0; i < names.size(); ++i )
        {
            fields += comma;
            comma = ",";

            if( ! names[i].empty() )
                fields += "values[" + boost::lexical_cast< std::string >( size++ ) + "]";
        }
    }

    csv.fields = fields;
    csv.full_xpath = true;
    input.values.resize( size );
}

static int run()
{
    comma::csv::input_stream< input_t > istream( std::cin, csv, input );
    comma::csv::output_stream< input_t > ostream( std::cout, csv, input );
    while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
    {
        const input_t* p = istream.read();
        if( !p ) { break; }
        input_t output = *p;        
        for( unsigned int i = 0; i < output.values.size(); output.values[i] = to_string( from_string( output.values[i], from ), to ), ++i );
        ostream.write( output, istream );
    }
    return 0;
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        accept_empty = options.exists( "--empty-as-not-a-date-time,--accept-empty,-e" );
        csv = comma::csv::options( options );
        csv.precision = 16;
        if( csv.fields.empty() ) { csv.fields="a"; }
        init_input();
        options.assert_mutually_exclusive( "--to-seconds,--to-iso-string,--seconds,--sec,--iso,--xsd,-s,-i,--from" );
        options.assert_mutually_exclusive( "--to-seconds,--to-iso-string,--seconds,--sec,--iso,--xsd,-s,-i,--to" );
        if( options.exists( "--to-seconds,--to-iso-string,--seconds,--sec,--iso,--xsd,-s,-i" ) ) { std::cerr << "csv-time: one of deprecated options used: --to-seconds,--to-iso-string,--seconds,--sec,--iso,--xsd,-s,-i; please try to use --from and --to instead" << std::endl; }
        if( options.exists( "--to-iso-string,--iso,-i" ) ) { from = seconds; to = iso; }
        else if ( options.exists( "--to-seconds,--sec,-s" ) ) { from = iso; to = seconds; }
        else { from = what( "--from", options ); to = what( "--to", options ); }
        if( guess == to ) { std::cerr << "csv-time: please specify valid --to" << std::endl; return 1; }
        return run();
    }
    catch( std::exception& ex ) { std::cerr << "csv-time: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-time: unknown exception" << std::endl; }
    return 1;
}
