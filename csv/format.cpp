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

#include <sstream>
#include <string.h>
#include <time.h>
#include <cmath>
#include <sstream>
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include "../base/exception.h"
#include "../base/types.h"
#include "../string/string.h"
#include "../csv/format.h"
#include "impl/epoch.h"

namespace comma { namespace csv {

format::format( const std::string& f )
    : string_( f )
    , size_( 0 )
    , count_( 0 )
{
    std::string format = comma::strip( f, " \t\r\n" );
    if( format == "" ) { return; }
    format = comma::strip( format, "%" );
    std::vector< std::string > v = comma::split( format, ",%" );
    std::size_t offset = 0;
    for( unsigned int i = 0; i < v.size(); ++i )
    {
        std::string s;
        for( ; s.length() < v[i].length() && v[i][ s.length() ] >= '0' && v[i][ s.length() ] <= '9'; s += v[i][ s.length() ] );
        if( s.length() >= v[i].length() ) { COMMA_THROW( comma::exception, "expected format, got '" << v[i] << "' in " << format ); }
        std::size_t arraySize = s.empty() ? 1 : boost::lexical_cast< std::size_t >( s );
        std::string type = v[i].substr( s.length() );
        types_enum t;
        unsigned int size;
        if( type == "b" ) { t = format::int8; size = 1; }
        else if( type == "ub" ) { t = format::uint8; size = 1; }
        else if( type == "w" ) { t = format::int16; size = 2; }
        else if( type == "uw" ) { t = format::uint16; size = 2; }
        else if( type == "i" ) { t = format::int32; size = 4; }
        else if( type == "ui" ) { t = format::uint32; size = 4; }
        else if( type == "l" ) { t = format::int64; size = 8; }
        else if( type == "ul" ) { t = format::uint64; size = 8; }
        else if( type == "c" ) { t = format::char_t; size = 1; }
        else if( type == "t" ) { t = format::time; size = sizeof( comma::int64 ); }
        else if( type == "lt" ) { t = format::long_time; size = sizeof( comma::int32 ) + sizeof( comma::int64 ); }
        else if( type == "f" ) { t = format::float_t; size = sizeof( float ); }
        else if( type == "d" ) { t = format::double_t; size = sizeof( double ); }
        else if( type[0] == 's' && type.length() == 1 ) { COMMA_THROW( comma::exception, "got variable size string in [" << format << "]: not implemented, use fixed size string instead, e.g. \"s[8]\"" ); }
        else if( type[0] == 's' && type.length() > 3 && type[1] == '[' && *type.rbegin() == ']' )
        {
            t = format::fixed_string;
            size = boost::lexical_cast< std::size_t >( type.substr( 2, type.length() - 3 ) );
        }
        else { COMMA_THROW( comma::exception, "expected format, got '" << type << "' in " << format ); }
        elements_.push_back( element( offset, arraySize, size, t ) );
        count_ += arraySize;
        size *= arraySize;
        offset += size;
        size_ += size;
    }
}

const std::string& format::string() const { return string_; }

std::string format::to_format( types_enum type, unsigned int size )
{
    std::ostringstream oss;
    oss << to_format( type );
    if( type == format::fixed_string ) { oss << "[" << size << "]"; }
    return oss.str();
}

std::string format::to_format( format::types_enum type )
{
    switch( type )
    {
        case format::int8: return "b";
        case format::uint8: return "ub";
        case format::int16: return "w";
        case format::uint16: return "uw";
        case format::int32: return "i";
        case format::uint32: return "ui";
        case format::int64: return "l";
        case format::uint64: return "ul";
        case format::char_t: return "c";
        case format::float_t: return "f";
        case format::double_t: return "d";
        case format::time: return "t";
        case format::long_time: return "lt";
        case format::fixed_string: return "s";
    }
    COMMA_THROW( comma::exception, "expected type, got " << type );
}

std::size_t format::size() const { return size_; }

std::size_t format::count() const { return count_; }

static boost::array< unsigned int, 12 > Sizesimpl()
{
    boost::array< unsigned int, 12 > sizes;
    sizes[ format::char_t ] = sizeof( char );
    sizes[ format::int8 ] = sizeof( char );
    sizes[ format::uint8 ] = sizeof( unsigned char );
    sizes[ format::int16 ] = sizeof( int16 );
    sizes[ format::uint16 ] = sizeof( uint16 );
    sizes[ format::int32 ] = sizeof( int32 );
    sizes[ format::uint32 ] = sizeof( uint32 );
    sizes[ format::int64 ] = sizeof( int64 );
    sizes[ format::uint64 ] = sizeof( uint64 );
    sizes[ format::float_t ] = sizeof( float );
    sizes[ format::double_t ] = sizeof( double );
    sizes[ format::time ] = sizeof( int64 );
    sizes[ format::long_time ] = sizeof( int64 ) + sizeof( int32 );
    sizes[ format::fixed_string ] = 0; // will it blast somewhere?
    return sizes;
}

const format& format::operator+=( const std::string& rhs ) // quick and dirty
{
    if( rhs == "" ) { return *this; }
    const std::string& lhs = string();
    return operator=( format( lhs == "" ? rhs : lhs + "," + rhs ) );
}

const format& format::operator+=( const format& rhs )
{
    return operator+=( rhs.string() );
}

std::string format::usage()
{
    std::ostringstream oss;
    oss << "        <format> : e.g. \"%t%d%d%d%ul\" or \"t,d,d,d,ul\" (same)" << std::endl
        << "            b  : byte (8-bit int)" << std::endl
        << "            ub : unsigned byte (unsigned 8-bit int)" << std::endl
        << "            w  : 16-bit int" << std::endl
        << "            uw : unsigned 16-bit int" << std::endl
        << "            i  : 32-bit int" << std::endl
        << "            ui : unsigned 32-bit int" << std::endl
        << "            l  : 64-bit int" << std::endl
        << "            ul : unsigned 64-bit int" << std::endl
        << "            c  : char" << std::endl
        << "            f  : float" << std::endl
        << "            d  : double" << std::endl
        << "            s  : variable size string (not implemented)" << std::endl
        << "            s[<length>]  : fixed size string, e.g. \"s[4]\"" << std::endl
        << "            t  : time (64-bit signed int, number of microseconds since epoch)" << std::endl
        << "            lt  : time (64+32 bit, seconds since epoch and nanoseconds)" << std::endl;
    return oss.str();
}

std::size_t format::size_of( types_enum type ) // todo: returns 0 for fixed size string, which is lame
{
    static boost::array< unsigned int, 12 > sizes = Sizesimpl();
    return sizes[ static_cast< std::size_t >( type ) ];
}

namespace impl {

template < typename T >
static std::size_t csv_to_bin( char* buf, const std::string& s )
{
    // T t = boost::lexical_cast< T >( s );
    //::memcpy( buf, &t, sizeof( T ) );
    *reinterpret_cast< T* >( buf ) = boost::lexical_cast< T >( s );
    return sizeof( T );
}

template < typename T >
static void withPrecision( std::ostringstream& oss, T t, const boost::optional< unsigned int >& ) { oss << t; }

static void withPrecision( std::ostringstream& oss, float t, const boost::optional< unsigned int >& precision )
{
    oss.precision( precision ? *precision : 6 );
    oss << t;
}

static void withPrecision( std::ostringstream& oss, double t, const boost::optional< unsigned int >& precision )
{
    oss.precision( precision ? *precision : 16 );
    oss << t;
}

template < typename T >
static std::size_t bin_to_csv( std::ostringstream& oss, const char* buf, const boost::optional< unsigned int >& precision )
{
    //T t;
    //::memcpy( &t, buf, sizeof( T ) );
    //withPrecision( oss, t, precision );
    withPrecision( oss, *reinterpret_cast< const T* >( buf ), precision );
    return sizeof( T );
}

static boost::posix_time::ptime time_from_iso_string( const std::string& s )
{
    if ( s.empty() || s == "not-a-date-time" ) { return boost::posix_time::not_a_date_time; }
    else if ( s == "+infinity" || s == "+inf" || s == "inf" ) { return boost::posix_time::pos_infin; }
    else if ( s == "-infinity" || s == "-inf" ) { return boost::posix_time::neg_infin; }
    else 
    { 
        try { return boost::posix_time::from_iso_string( s ); }
        catch ( ... ) { return boost::posix_time::not_a_date_time; }
    }
    return boost::posix_time::not_a_date_time;
}

static std::size_t csv_to_bin( char* buf, const std::string& s, format::types_enum type, std::size_t size )
{
    try
    {
        switch( type ) // todo: tear down csv_to_bin, use format::traits
        {
            case format::int8:
            {
                int i = boost::lexical_cast< int >( s );
                if( i < -127 || i > 128 ) { COMMA_THROW( comma::exception, "expected byte, got " << i ); }
                *buf = static_cast< char >( i );
                return sizeof( char );
            }
            case format::uint8:
            {
                unsigned int i = boost::lexical_cast< unsigned int >( s );
                if( i > 255 ) { COMMA_THROW( comma::exception, "expected unsigned byte, got " << i ); }
                //unsigned char c = static_cast< unsigned char >( i );
                //::memcpy( buf, &c, 1 );
                *buf = static_cast< unsigned char >( i );
                return sizeof( unsigned char );
            }
            case format::int16: return csv_to_bin< comma::int16 >( buf, s );
            case format::uint16: return csv_to_bin< comma::uint16 >( buf, s );
            case format::int32: return csv_to_bin< comma::int32 >( buf, s );
            case format::uint32: return csv_to_bin< comma::uint32 >( buf, s );
            case format::int64: return csv_to_bin< comma::int64 >( buf, s );
            case format::uint64: return csv_to_bin< comma::uint64 >( buf, s );
            case format::char_t: return csv_to_bin< char >( buf, s );
            case format::float_t: return csv_to_bin< float >( buf, s );
            case format::double_t: return csv_to_bin< double >( buf, s );
            case format::time: // TODO: quick and dirty: use serialization traits
                format::traits< boost::posix_time::ptime, format::time >::to_bin( time_from_iso_string(s), buf );
                return format::traits< boost::posix_time::ptime, format::time >::size;
            case format::long_time: // TODO: quick and dirty: use serialization traits
                format::traits< boost::posix_time::ptime, format::long_time >::to_bin( time_from_iso_string(s), buf );
                return format::traits< boost::posix_time::ptime, format::long_time >::size;
            case format::fixed_string:
            {
                if( s.length() > size ) { COMMA_THROW( comma::exception, "expected string not longer than " << size << "; got \"" << s << "\"" ); }
                ::memset( buf, 0, size );
                if( !s.empty() )
                {
                    unsigned int length = s.length();
                    unsigned int begin = 0;
                    if( length > 1 && s[0] == '\"' && s[ s.length() - 1 ] == '\"' ) { length -= 2; ++begin; } // todo quick and dirty; implement proper character escaping and consistent semantics
                    ::memcpy( buf, &s[begin], length );
                }
                return size;
            }
            default: COMMA_THROW( comma::exception, "todo: not implemented" );
        }
    }
    catch( std::exception& ex )
    {
        COMMA_THROW( comma::exception, "failed to convert \"" << s << "\" to type \"" << format::to_format(type) << "\": "  << ex.what() );
    }
    catch( ... )
    {
        throw;
    }
}

static std::size_t bin_to_csv( std::ostringstream& oss, const char* buf, format::types_enum type, std::size_t size, const boost::optional< unsigned int >& precision )
{
    switch( type ) // todo: tear down bin_to_csv, use format::traits
    {
        case format::int8:
            oss << static_cast< int >( *buf );
            return sizeof( char );
        case format::uint8:
            oss << static_cast< unsigned int >( static_cast< unsigned char >( *buf ) );
            return sizeof( unsigned char );
        case format::int16: return bin_to_csv< comma::int16 >( oss, buf, precision );
        case format::uint16: return bin_to_csv< comma::uint16 >( oss, buf, precision );
        case format::int32: return bin_to_csv< comma::int32 >( oss, buf, precision );
        case format::uint32: return bin_to_csv< comma::uint32 >( oss, buf, precision );
        case format::int64: return bin_to_csv< comma::int64 >( oss, buf, precision );
        case format::uint64: return bin_to_csv< comma::uint64 >( oss, buf, precision );
        case format::char_t: return bin_to_csv< char >( oss, buf, precision );
        case format::float_t: return bin_to_csv< float >( oss, buf, precision );
        case format::double_t: return bin_to_csv< double >( oss, buf, precision );
        case format::time:
            oss << boost::posix_time::to_iso_string( format::traits< boost::posix_time::ptime, format::time >::from_bin( buf, sizeof( comma::uint64 ) ) );
            return format::traits< boost::posix_time::ptime, format::time >::size;
        case format::long_time:
            oss << boost::posix_time::to_iso_string( format::traits< boost::posix_time::ptime, format::long_time >::from_bin( buf, sizeof( comma::uint64 ) + sizeof( comma::uint32 ) ) );
            return format::traits< boost::posix_time::ptime, format::long_time >::size;
        case format::fixed_string:
            oss << ( buf[ size - 1 ] == 0 ? std::string( buf ) : std::string( buf, size ) );
            return size;
        default : COMMA_THROW( comma::exception, "on type: " << type << ": todo: not implemented" );
    }
}

} // namespace impl {

void format::csv_to_bin( std::ostream& os, const std::string& csv, char delimiter, bool flush ) const
{
    const std::vector< std::string >& v = comma::split( csv, delimiter );
    csv_to_bin( os, v, flush );
}

void format::csv_to_bin( std::ostream& os, const std::vector< std::string >& v, bool flush ) const
{
    if( v.size() != count_ ) { COMMA_THROW( comma::exception, "expected csv string with " << count_ << " elements, got [" << comma::join( v, ',' ) << "]" ); }
    std::vector< char > buf( size_ ); //char buf[ size_ ]; // stupid Windows
    char* p = &buf[0];
    unsigned int offsetIndex = 0u;
    unsigned int count = 0u;
    for( unsigned int i = 0; i < v.size(); ++i, ++count )
    {
        try
        {
            if( count >= elements_[ offsetIndex ].count ) { count = 0; ++offsetIndex; }
            p += impl::csv_to_bin( p, v[i], elements_[ offsetIndex ].type, elements_[ offsetIndex ].size );
        }
        catch( std::exception& ex )
        {
            COMMA_THROW( comma::exception, "column " << i + 1 << ": "  << ex.what() );
        }
    }
    os.write( &buf[0], size_ );
    if( flush ) { os.flush(); }
}

std::string format::csv_to_bin( const std::string& csv, char delimiter ) const
{
    std::ostringstream oss( std::ios::out | std::ios::binary );
    csv_to_bin( oss, csv, delimiter );
    return oss.str();
}

std::string format::csv_to_bin( const std::vector< std::string >& csv ) const
{
    std::ostringstream oss( std::ios::out | std::ios::binary );
    csv_to_bin( oss, csv );
    return oss.str();
}

std::string format::bin_to_csv( const std::string& bin, char delimiter, const boost::optional< unsigned int >& precision ) const
{
    if( bin.length() != size_ ) { COMMA_THROW( comma::exception, "expected binary string of size " << size_ << ", got " << bin.length() << " bytes" ); }
    return bin_to_csv( bin.c_str(), delimiter, precision );
}

std::string format::bin_to_csv( const char* buf, char delimiter, const boost::optional< unsigned int >& precision ) const
{
    std::ostringstream oss;
    const char* p = buf;
    unsigned int offsetIndex = 0u; // index in elements_
    unsigned int count = 0u;
    for( unsigned int i = 0u; i < count_; ++i, ++count )
    {
        if( i > 0 ) { oss << delimiter; }
        if( count >= elements_[ offsetIndex ].count ) { count = 0; ++offsetIndex; }
        p += impl::bin_to_csv( oss, p, elements_[ offsetIndex ].type, elements_[ offsetIndex ].size, precision );
    }
    return oss.str();
}

const std::vector< format::element >& format::elements() const { return elements_; }

std::pair< unsigned int, unsigned int > format::index( std::size_t ind ) const
{
    unsigned int count = 0;
    for( unsigned int i = 0; i < elements_.size(); count += elements_[i].count, ++i )
    {
        if( ind < count + elements_[i].count ) { return std::make_pair( i, ind - count ); }
    }
    COMMA_THROW( comma::exception, "expected index less than " << count << "; got " << ind );
}

format::element format::offset( std::size_t ind ) const
{
    std::pair< unsigned int, unsigned int > i = index( ind );
    return element( elements_[ i.first ].offset + elements_[ i.first ].size * i.second
                  , 1
                  , elements_[ i.first ].size
                  , elements_[ i.first ].type );
}

std::string format::expanded_string() const
{
    format result;
    for ( unsigned int i = 0; i < elements_.size(); ++i )
    {
        for ( unsigned int n = 0; n < elements_[ i ].count; ++n )
        { result += format::to_format( elements_[ i ].type, elements_[ i ].size ); }
    }
    return result.string();
}

std::string format::collapsed_string() const
{
    format result;

    if( elements_.size() >= 1 )
    {
        element last_element( elements_[ 0 ] );
        std::size_t count = elements_[ 0 ].count;
        for ( unsigned int i = 1; i < elements_.size(); ++i )
        {
            types_enum type = elements_[ i ].type;
            if( type != last_element.type || last_element.type == format::fixed_string )
            {
                std::ostringstream new_element;
                if( count > 1 ) { new_element << count; }
                new_element << format::to_format( last_element.type, last_element.size );
                result += new_element.str();
                last_element = elements_[ i ];
                count = 0;
            }
            count += elements_[ i ].count;
        }
        std::ostringstream new_element;
        if( count > 1 ) { new_element << count; }
        new_element << format::to_format( last_element.type, last_element.size );
        result += new_element.str();
    }
    return result.string();
}

// formats for not-a-date-time, +infinity, -infinity
// note: these are not boost representations. in boost, +infinity = int64::max() - 1, -infinity = int64::min(), not-a-date-time = int64::max()
// not-a-date-time is chosen to matche python numpy.datetime64('NaT') = int64::min()
static const comma::int64 bin_not_a_date_time = std::numeric_limits< comma::int64 >::min();
static const comma::int64 bin_time_pos_infin = std::numeric_limits< comma::int64 >::max();
static const comma::int64 bin_time_neg_infin = std::numeric_limits< comma::int64 >::min() + 1;

boost::posix_time::ptime format::traits< boost::posix_time::ptime, format::long_time >::from_bin( const char* buf, std::size_t size )
{
    //comma::int64 seconds; // todo: due to bug in boost, will be casted down to int32, but for the dates we use seconds will never overflow, thus, leave it like this now
    //comma::int32 nanoseconds;
    //::memcpy( &seconds, buf, sizeof( comma::int64 ) );
    //::memcpy( &nanoseconds, buf + sizeof( comma::int64 ), sizeof( comma::int32 ) );
	(void) size;
    comma::int64 seconds = *reinterpret_cast< const comma::int64* >( buf );
    if( seconds == bin_not_a_date_time ) { return boost::posix_time::not_a_date_time; }
    if( seconds == bin_time_pos_infin ) { return boost::posix_time::pos_infin; }
    if( seconds == bin_time_neg_infin ) { return boost::posix_time::neg_infin; }
    comma::int32 nanoseconds = *reinterpret_cast< const comma::int32* >( buf + sizeof( comma::int64 ) );
    return boost::posix_time::ptime( csv::impl::epoch, boost::posix_time::seconds( static_cast< long >( seconds ) ) + boost::posix_time::microseconds( nanoseconds / 1000 ) );
}

void format::traits< boost::posix_time::ptime, format::long_time >::to_bin( const boost::posix_time::ptime& t, char* buf, std::size_t size )
{
	(void)size;
    if( t.is_not_a_date_time() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_not_a_date_time; *reinterpret_cast< comma::int32* >( buf + sizeof( comma::int64 ) ) = 0; return; }
    if( t.is_pos_infinity() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_time_pos_infin; *reinterpret_cast< comma::int32* >( buf + sizeof( comma::int64 ) ) = 0; return; }
    if( t.is_neg_infinity() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_time_neg_infin; *reinterpret_cast< comma::int32* >( buf + sizeof( comma::int64 ) ) = 0; return; }
    static const boost::posix_time::ptime base( csv::impl::epoch );
    const boost::posix_time::time_duration duration = t - base;
    comma::int64 seconds = duration.total_seconds();
    comma::int32 nanoseconds = static_cast< comma::int32 >( ( duration - boost::posix_time::seconds( static_cast< long >( seconds ) ) ).total_microseconds() % 1000000 ) * 1000;
    //::memcpy( buf, &seconds, sizeof( comma::int64 ) );
    //::memcpy( buf + sizeof( comma::int64 ), &nanoseconds, sizeof( comma::int32 ) );
    *reinterpret_cast< comma::int64* >( buf ) = seconds;
    *reinterpret_cast< comma::int32* >( buf + sizeof( comma::int64 ) ) = nanoseconds;
}

boost::posix_time::ptime format::traits< boost::posix_time::ptime, format::time >::from_bin( const char* buf, std::size_t size )
{
    //comma::int64 microseconds;
    //::memcpy( &microseconds, buf, sizeof( comma::int64 ) );
	(void)size;
    comma::int64 microseconds = *reinterpret_cast< const comma::int64* >( buf );
    if( microseconds == bin_not_a_date_time ) { return boost::posix_time::not_a_date_time; }
    if( microseconds == bin_time_pos_infin ) { return boost::posix_time::pos_infin; }
    if( microseconds == bin_time_neg_infin ) { return boost::posix_time::neg_infin; }
    long seconds = static_cast< long >( microseconds / 1000000 ); // todo: due to bug in boost, will be casted down to int32, but for the dates we use seconds will never overflow, thus, leave it like this now
    microseconds -= static_cast< comma::int64 >( seconds ) * 1000000;
    return boost::posix_time::ptime( csv::impl::epoch, boost::posix_time::seconds( seconds ) + boost::posix_time::microseconds( static_cast< long >( microseconds ) ) );
}

void format::traits< boost::posix_time::ptime, format::time >::to_bin( const boost::posix_time::ptime& t, char* buf, std::size_t size )
{
    if( t.is_not_a_date_time() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_not_a_date_time; return; }
    if( t.is_pos_infinity() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_time_pos_infin; return; }
    if( t.is_neg_infinity() ) { *reinterpret_cast< comma::int64* >( buf ) = bin_time_neg_infin; return; }
	(void)size;
    static const boost::posix_time::ptime base( csv::impl::epoch );
    const boost::posix_time::time_duration duration = t - base;
    long seconds = duration.total_seconds(); // boost uses long, which is a bug for 32-bit
    comma::int64 microseconds = static_cast< comma::int64 >( seconds ) * 1000000l;
    microseconds += ( duration - boost::posix_time::seconds( seconds ) ).total_microseconds();
    *reinterpret_cast< comma::int64* >( buf ) = microseconds; // ::memcpy( buf, &microseconds, sizeof( comma::int64 ) );

}

std::string format::traits< std::string, format::fixed_string >::from_bin( const char* buf, std::size_t size )
{
    return buf[ size - 1 ] == 0 ? std::string( buf ) : std::string( buf, size );
}

void format::traits< std::string, format::fixed_string >::to_bin( const std::string& t, char* buf, std::size_t size )
{
    if( t.length() > size ) { COMMA_THROW( comma::exception, "expected string no longer than " << size << ", got '" << t << "' of " << t.length() ); }
    ::memcpy( buf, &t[0], t.length() );
    if( t.length() < size ) { ::memset( buf + t.length(), 0, size - t.length() ); }
}

} } // namespace comma { namespace csv {
