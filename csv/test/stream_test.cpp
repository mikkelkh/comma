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


#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <boost/array.hpp>
//#include <google/profiler.h>
#include "../../base/types.h"
#include "../../csv/stream.h"

namespace comma { namespace csv { namespace stream_test {

struct test_struct
{
    comma::uint32 x;
    comma::uint32 y;
    test_struct() : x( 0 ), y( 0 ) {}
    test_struct( comma::uint32 x, comma::uint32 y ) : x( x ), y( y ) {}
};

struct test_container
{
    std::vector< int > vector;
};

} } } // namespace comma { namespace csv { namespace test {

namespace comma { namespace visiting {

template <> struct traits< comma::csv::stream_test::test_struct >
{
    template < typename Key, class Visitor >
    static void visit( const Key&, const comma::csv::stream_test::test_struct& p, Visitor& v )
    {
        v.apply( "x", p.x );
        v.apply( "y", p.y );
    }
    
    template < typename Key, class Visitor >
    static void visit( const Key&, comma::csv::stream_test::test_struct& p, Visitor& v )
    {
        v.apply( "x", p.x );
        v.apply( "y", p.y );
    }    
};

template <> struct traits< comma::csv::stream_test::test_container >
{
    template < typename Key, class Visitor >
    static void visit( const Key&, const comma::csv::stream_test::test_container& p, Visitor& v )
    {
        v.apply( "vector", p.vector );
    }

    template < typename Key, class Visitor >
    static void visit( const Key&, comma::csv::stream_test::test_container& p, Visitor& v )
    {
        v.apply( "vector", p.vector );
    }
};

} } // namespace comma { namespace visiting {

namespace comma { namespace csv { namespace stream_test {

TEST( csv, container )
{
	comma::csv::options csv;
     csv.full_xpath = true;
	{
        std::string s( "2,3,,,6" );
        std::istringstream iss( s );
        test_container sample; sample.vector = std::vector< int >( 5, 1 );
        comma::csv::input_stream< test_container > istream( iss, csv, sample );
        const test_container *c = istream.read();
        EXPECT_EQ( c->vector.size(), 5 );
        std::string so = comma::join( c->vector, ',' );
        EXPECT_EQ( so, "2,3,1,1,6" );
    }
    {
        std::ostringstream oss;
        test_container c; c.vector = std::vector< int >( 5, 1 );
        comma::csv::output_stream< test_container > ostream( oss, csv, c );
        c.vector[1] = 5;
        c.vector[2] = 3;
        ostream.write( c );
        EXPECT_EQ( c.vector.size(), 5 );
        EXPECT_EQ( oss.str(), "1,5,3,1,1\n" );
    }
}

} } } // namespace comma { namespace csv { namespace stream_test {

namespace comma { namespace csv { namespace stream_test {

TEST( csv, stream )
{
//	std::cerr << "ProfileStream(): start" << std::endl;
//	const std::size_t size = 100000;
//	std::string istring( size * 4 * 2, 0 );
//	for( unsigned int i = 0; i < size; ++i ) // no need, really
//	{
//		::memcpy( &istring[i*4*2], &i, 4 );
//		::memcpy( &istring[4 + i*4*2], &i, 4 );
//	}
//	std::istringstream iss( istring );
//	std::ostringstream oss;
//	comma::csv::options csv;
//	csv.format( "%ui%ui" );
//	EXPECT_TRUE( csv.binary() );
//	comma::csv::input_stream< test_struct > istream( iss, csv );
//	comma::csv::output_stream< test_struct > ostream( oss, csv );
//	ProfilerStart( "csv_stream.prof" );	{
//	for( unsigned int i = 0; i < size; ++i )
//	{
//		const test_struct* s = istream.read();
//		test_struct t( s->x + 1, s->y + 1 );
//		//ostream.write( t, istream.last() );
//		ostream.binary().write( t, istream.binary().last() );
//	}
//	ProfilerStop(); }
//	std::cerr << "ProfileStream(): stop" << std::endl;
}

} } } // namespace comma { namespace csv { namespace stream_test {

