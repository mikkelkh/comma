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
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "../../base/types.h"
#include "../../name_value/ptree.h"
#include "../../name_value/serialize.h"
#include "../../visiting/apply.h"

namespace comma { namespace test {

struct nested_type
{
    nested_type() : moon( 0 ) {}
    int moon;
    boost::optional< std::string > value;
};

struct test_type
{
    int hello;
    boost::optional< std::string > world;
    boost::optional< nested_type > nested;
    test_type() : hello( 0 ) {  }
};

struct vector_type
{
    std::vector< std::string > v;
};

struct nested_vector
{
    std::string hello;
    vector_type v;
};

} } // namespace comma { namespace test {

namespace comma { namespace visiting {

template <>
struct traits< comma::test::nested_type >
{
    template < typename Key, typename Visitor >
    static void visit( const Key&, comma::test::nested_type& t, Visitor& v )
    {
        v.apply( "moon", t.moon );
        v.apply( "value", t.value );
    }

    template < typename Key, typename Visitor >
    static void visit( const Key&, const comma::test::nested_type& t, Visitor& v )
    {
        v.apply( "moon", t.moon );
        v.apply( "value", t.value );
    }
};

template <>
struct traits< comma::test::test_type >
{
    template < typename Key, typename Visitor >
    static void visit( const Key&, comma::test::test_type& t, Visitor& v )
    {
        v.apply( "hello", t.hello );
        v.apply( "world", t.world );
        v.apply( "nested", t.nested );
    }

    template < typename Key, typename Visitor >
    static void visit( const Key&, const comma::test::test_type& t, Visitor& v )
    {
        v.apply( "hello", t.hello );
        v.apply( "world", t.world );
        v.apply( "nested", t.nested );
    }
};

template <>
struct traits< comma::test::vector_type >
{
    template < typename Key, typename Visitor >
    static void visit( const Key&, comma::test::vector_type& t, Visitor& v )
    {
        v.apply( "v", t.v );
    }

    template < typename Key, typename Visitor >
    static void visit( const Key&, const comma::test::vector_type& t, Visitor& v )
    {
        v.apply( "v", t.v );
    }
};

template <>
struct traits< comma::test::nested_vector >
{
    template < typename Key, typename Visitor >
    static void visit( const Key&, comma::test::nested_vector& t, Visitor& v )
    {
        v.apply( "hello", t.hello );
        v.apply( "v", t.v );
    }

    template < typename Key, typename Visitor >
    static void visit( const Key&, const comma::test::nested_vector& t, Visitor& v )
    {
        v.apply( "hello", t.hello );
        v.apply( "v", t.v );
    }
};

} } // namespace comma { namespace visiting {

namespace comma { namespace test {

TEST( ptree, basics )
{
    {
        test_type t;
        t.world = "value";
        boost::property_tree::ptree tree;
        to_ptree to( tree, "test_type" );
        visiting::apply( to, t );
        
        test_type t1;
        from_ptree from( tree, "test_type" );
        visiting::apply( from, t1 );
        EXPECT_EQ( t.hello, t1.hello );
        EXPECT_EQ( t.world, t1.world );
        EXPECT_EQ( *t1.world, "value" );
        EXPECT_TRUE( !t.nested );
        EXPECT_TRUE( !t1.nested );

        boost::property_tree::ptree tree1;
        to_ptree to1( tree1, "test_type" );
        t.world.reset();
        t.nested = nested_type();
        t1.world.reset();
        visiting::apply( to1, t );
        from_ptree from1( tree1, "test_type" );
        visiting::apply( from1, t1 );
        EXPECT_EQ( t.hello, t1.hello );
        EXPECT_TRUE( !t.world );
        EXPECT_TRUE( !t1.world );
        EXPECT_EQ( t.nested->moon, t1.nested->moon );
    }
    {
        boost::property_tree::ptree ptree;
        to_ptree to_ptree( ptree, "vector" );
        std::vector< std::string > v;
        v.push_back( "hello" );
        v.push_back( "world" );
        visiting::apply( to_ptree, v );
        from_ptree from_ptree( ptree, "vector" );
        visiting::apply( from_ptree, v );
        EXPECT_EQ( v.size(), 2u );
        EXPECT_EQ( v[0], "hello" );
        EXPECT_EQ( v[1], "world" );
        v.clear();
        visiting::apply( from_ptree, v );
        EXPECT_EQ( v.size(), 2u );
        EXPECT_EQ( v[0], "hello" );
        EXPECT_EQ( v[1], "world" );
    }
    {
        boost::property_tree::ptree ptree;
        to_ptree to_ptree( ptree, "vector" );
        std::vector< std::string > v;
        v.push_back( "hello" );
        v.push_back( "world" );
        vector_type vec;
        vec.v = v;
        nested_vector nested;
        nested.v = vec;
        visiting::apply( to_ptree, nested );
        from_ptree from_ptree( ptree, "vector" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 2u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.v.v[1], "world" );
        nested.v.v.clear();
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 2u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.v.v[1], "world" );
    }
    {
        boost::property_tree::ptree ptree;
        to_ptree t( ptree, "map" );
        std::map< unsigned int, std::string > m;
        m.insert( std::make_pair( 1, "hello" ) );
        m.insert( std::make_pair( 3, "world" ) );
        visiting::apply( t, m );
        from_ptree from_ptree( ptree, "map" );
        visiting::apply( from_ptree, m );
        EXPECT_EQ( m.size(), 2u );
        EXPECT_EQ( m[1], "hello" );
        EXPECT_EQ( m[3], "world" );
        m.clear();
        //from_ptree from_ptree( ptree, "map" );
        visiting::apply( from_ptree, m );
        EXPECT_EQ( m.size(), 2u );
        EXPECT_EQ( m[1], "hello" );
        EXPECT_EQ( m[3], "world" );
    }
}

TEST( ptree, from_path_value )
{
    {
        boost::property_tree::ptree ptree;
        std::string s = "a/4/hello=4,a/4/nested/moon=1,a/4/nested/value=hello,a/6/hello=6,a/6/nested/moon=2,a/6/nested/value=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree from_ptree( ptree, "a" );
        std::map< unsigned int, test_type > m;
        visiting::apply( from_ptree, m );
        EXPECT_EQ( 2u, m.size() );
        EXPECT_EQ( 4, m[4].hello );
        EXPECT_EQ( 1, m[4].nested->moon );
        EXPECT_EQ( "hello", *m[4].nested->value );
        EXPECT_EQ( 6, m[6].hello );
        EXPECT_EQ( 2, m[6].nested->moon );
        EXPECT_EQ( "world",  *m[6].nested->value );
    }
    {
        boost::property_tree::ptree ptree;
        std::string s = "a/4/hello=4\na/4/nested/moon=1\na/4/nested/value=hello\na/6/hello=6\na/6/nested/moon=2\na/6/nested/value=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree, comma::property_tree::path_value::no_check, '=', '\n' );
        from_ptree from_ptree( ptree, "a" );
        std::map< unsigned int, test_type > m;
        visiting::apply( from_ptree, m );
        EXPECT_EQ( 2u, m.size() );
        EXPECT_EQ( 4, m[4].hello );
        EXPECT_EQ( 1, m[4].nested->moon );
        EXPECT_EQ( "hello", *m[4].nested->value );
        EXPECT_EQ( 6, m[6].hello );
        EXPECT_EQ( 2, m[6].nested->moon );
        EXPECT_EQ( "world",  *m[6].nested->value );
    }

}

TEST( ptree, permissive_visiting )
{
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree );
        nested_vector nested;
        ptree.put< std::string >( "hello", "blah" );
        ptree.put< std::string >( "v.v.0", "hello" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 1u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.hello, "blah" );
        nested.v.v.clear();
        ptree.put< std::string >( "v.v.1", "world" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 2u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.v.v[1], "world" );
        EXPECT_EQ( nested.hello, "blah" );
    }
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree, true );
        nested_vector nested;
        ptree.put< std::string >( "hello", "blah" );
        ptree.put< std::string >( "v.v.0", "hello" );
        ptree.put< std::string >( "v.v.1", "world" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 2u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.v.v[1], "world" );
        EXPECT_EQ( nested.hello, "blah" );
    }        
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree, true );
        nested_vector nested;
        ptree.put< std::string >( "v.v.0", "hello" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 1u );
        EXPECT_EQ( nested.v.v[0], "hello" );
        EXPECT_EQ( nested.hello, "" );
    }
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree );
        nested_vector nested;
        try { visiting::apply( from_ptree, nested ); EXPECT_TRUE( false ); } catch ( ... ) {}
    }
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree );
        nested_vector nested;
        ptree.put< std::string >( "hello", "blah" );
        try { visiting::apply( from_ptree, nested ); EXPECT_TRUE( false ); } catch ( ... ) {}
    }
    {
        boost::property_tree::ptree ptree;
        from_ptree from_ptree( ptree );
        nested_vector nested;
        ptree.put< std::string >( "hello", "blah" );
        ptree.put< std::string >( "v.v.0", "hello" );
        visiting::apply( from_ptree, nested );
        EXPECT_EQ( nested.v.v.size(), 1u );
        EXPECT_EQ( nested.v.v[0], "hello" );
    }
    {
        boost::property_tree::ptree ptree;
        std::string s = "root/hello=blah,root/v/v=";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree f( ptree, "root" );
        nested_vector nested;
        visiting::apply( f, nested );
        EXPECT_EQ( nested.hello, "blah" );
        EXPECT_TRUE( nested.v.v.empty() );
    }
    {
        boost::property_tree::ptree ptree;
        std::string s = "root/hello=blah,root/v=";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree f( ptree, "root" );
        nested_vector nested;
        try { visiting::apply( f, nested ); EXPECT_TRUE( false ); } catch( ... ) {}
        from_ptree from_ptree_permissive( ptree, "root", true );
        try { visiting::apply( from_ptree_permissive, nested ); } catch( ... ) { EXPECT_TRUE( false ); }
    }
}

TEST( ptree, array )
{
    {
        boost::property_tree::ptree ptree;
        std::string s = "0=hello,1=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree f( ptree );
        boost::array< std::string, 2 > array;
        visiting::apply( f, array );
        EXPECT_EQ( array[0], "hello" );
        EXPECT_EQ( array[1], "world" );
    }        
    {
        boost::property_tree::ptree ptree;
        std::string s = "0=hello,1=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree from_ptree( ptree );
        boost::array< std::string, 3 > array;
        try { visiting::apply( from_ptree, array ); EXPECT_TRUE( false ); } catch( ... ) {}
    }
    {
        boost::property_tree::ptree ptree;
        std::string s = "root/0=hello,root/1=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree );
        from_ptree from_ptree( ptree, "root" );
        boost::array< std::string, 2 > array;
        visiting::apply( from_ptree, array );
        EXPECT_EQ( array[0], "hello" );
        EXPECT_EQ( array[1], "world" );
    }
    {
        boost::property_tree::ptree ptree;
        std::string s = "root/0=hello\nroot/1=world";
        std::istringstream iss( s );
        property_tree::from_path_value( iss, ptree, comma::property_tree::path_value::no_check, '=', '\n' );
        from_ptree from_ptree( ptree, "root" );
        boost::array< std::string, 2 > array;
        visiting::apply( from_ptree, array );
        EXPECT_EQ( array[0], "hello" );
        EXPECT_EQ( array[1], "world" );
    }
}

TEST( ptree, path_value_string )
{
    {
        std::string s = "x=1,y=2";
        boost::property_tree::ptree ptree = property_tree::from_path_value_string( s, '=', ',' );
        EXPECT_EQ( property_tree::to_path_value_string( ptree ), "x=\"1\",y=\"2\"" );
    }
    {
        std::string s = "x/a/b=1,x/a/c=2,y=3";
        boost::property_tree::ptree ptree = property_tree::from_path_value_string( s, '=', ',' );
        EXPECT_EQ( property_tree::to_path_value_string( ptree ), "x/a/b=\"1\",x/a/c=\"2\",y=\"3\"" );
    }
    // todo: more tests
}

struct a_t
{
    std::string a;
    std::string b;
};

struct b_t
{
    std::string c;
    a_t d;
};

struct c_t
{
    b_t e;
    std::vector< b_t > f;
};
    
template < typename T > struct test_xml_traits {};

template <> struct test_xml_traits< a_t >
{
    template < typename K, typename V >
    static void visit( const K&, a_t& t, V& v )
    {
        v.apply( "a", t.a );
        v.apply( "b", t.b );
    }
};

template <> struct test_xml_traits< b_t >
{
    template < typename K, typename V >
    static void visit( const K&, b_t& t, V& v )
    {
        v.apply( "c", t.c );
        v.apply( "d", t.d );
    }
};

template <> struct test_xml_traits< c_t >
{
    template < typename K, typename V >
    static void visit( const K&, c_t& t, V& v )
    {
        v.apply( "e", t.e );
        v.apply( "f", t.f );
    }
};

TEST( ptree, xmlattr )
{
    std::string s = "<root>"
                    "    <e c=\"C\">"
                    "        <d a=\"A\" b=\"B\"/>"
                    "    </e>"
                    "    <f>"
                    "        <e c=\"C0\">"
                    "            <d a=\"A0\" b=\"B0\"/>"
                    "        </e>"
                    "        <e c=\"C1\">"
                    "            <d a=\"A1\" b=\"B1\"/>"
                    "        </e>"
                    "    </f>"
                    "</root>";
    std::istringstream iss( s );
    boost::property_tree::ptree t;
    boost::property_tree::read_xml( iss, t );
    //comma::property_tree::to_path_value( std::cerr, t, '=', '\n' ); std::cerr << std::endl;
    comma::property_tree::from< test_xml_traits > v( t, "root" );
    c_t c;
    comma::visiting::apply( v, c );
    
    EXPECT_EQ( "A", c.e.d.a );
    EXPECT_EQ( "B", c.e.d.b );
    EXPECT_EQ( "C", c.e.c );
    
    ASSERT_EQ( 2, c.f.size() );
    
    EXPECT_EQ( "A0", c.f[0].d.a );
    EXPECT_EQ( "B0", c.f[0].d.b );
    EXPECT_EQ( "C0", c.f[0].c );
    
    EXPECT_EQ( "A1", c.f[1].d.a );
    EXPECT_EQ( "B1", c.f[1].d.b );
    EXPECT_EQ( "C1", c.f[1].c );
    
    // todo: more testing
}

} } // namespace comma { namespace test {

struct derived : public std::vector< std::string > {};

namespace comma { namespace visiting {
    
template <> struct traits< derived >
{
    template< typename K, typename V > static void visit( const K& k, derived& t, V& v )
    {
        v.apply( "", static_cast< std::vector< std::string >& >( t ) );
    }
};
    
} } // namespace comma { namespace visiting {

namespace comma { namespace test {
    
static const std::string json_string = "[ \"a\", \"b\" ]";

TEST ( name_value_ptree, vector_at_the_root )
{
    {
        std::stringstream iss( json_string );
        boost::property_tree::ptree p;
        boost::property_tree::read_json( iss, p );
        derived d;
        comma::from_ptree from_ptree( p, true );
        comma::visiting::apply( from_ptree ).to( d );
        ASSERT_EQ( d.size(), 2 );
        EXPECT_EQ( d[0], "a" );
        EXPECT_EQ( d[1], "b" );
    }
}

static void test_put_( boost::property_tree::ptree& p, const std::string& path, const std::string& value )
{
    property_tree::put( p, path, value );
    EXPECT_EQ( value, property_tree::get( p, path ) );
}

TEST ( name_value_ptree, put )
{
    {
        boost::property_tree::ptree p;
        test_put_( p, "b[0]", "0" );
        EXPECT_TRUE( bool( property_tree::get( p, "b[0]" ) ) );
        EXPECT_EQ( "0", *( property_tree::get( p, "b[0]" ) ) );
    }
    {
        boost::property_tree::ptree p;
        test_put_( p, "a/b/c", "0" );
        test_put_( p, "a/b/d[0]", "1" );
        test_put_( p, "a/b/d[1]", "2" );
        test_put_( p, "a/e[0]/x", "3" );
        test_put_( p, "a/e[1]/y", "4" );
        test_put_( p, "a/e[1]/z[0]", "5" );
        test_put_( p, "a/e[1]/z[1]", "6" );
        test_put_( p, "a/e[1]/f[0]/k", "7" );
        test_put_( p, "a/e[1]/f[1]/m", "8" );
        
        EXPECT_TRUE( bool( property_tree::get( p, "a/b/c" ) ) );
        EXPECT_TRUE( bool( property_tree::get( p, "a/e[0]/x" ) ) );
        EXPECT_FALSE( property_tree::get( p, "x/y/z" ) );
        EXPECT_FALSE( property_tree::get( p, "a/b/x" ) );
        EXPECT_FALSE( property_tree::get( p, "a/e[0]/z" ) );
    }
}

} } // namespace comma { namespace test {
