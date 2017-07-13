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

#include <string.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <boost/array.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/graph/graph_concepts.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../csv/impl/unstructured.h"
#include "../../io/stream.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static void usage( bool more )
{
    std::cerr << std::endl;
    std::cerr << "update csv files or streams by one or several keys (integer only for now)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat something.csv | csv-update \"update.csv\" [<options>] > updated.csv" << std::endl;
    std::cerr << "       cat something.csv | csv-update [<options>] > updated.csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "       if update input specified (update.csv above), update" << std::endl;
    std::cerr << "       records from stdin by the ones in the update file" << std::endl;
    if( more )
    {
        std::cerr << "       todo" << std::endl;
    }
    else
    {
        std::cerr << "       ... use --help --verbose for more" << std::endl;
    }
    std::cerr << std::endl;
    std::cerr << "       if update input not specified, apply updates received" << std::endl;
    std::cerr << "       on stdin itself" << std::endl;
    if( more )
    {
        std::cerr << "       todo" << std::endl;
    }
    else
    {
        std::cerr << "       ... use --help --verbose for more" << std::endl;
    }
    std::cerr << std::endl;
    std::cerr << "    fields:" << std::endl;
    std::cerr << "        block: block number" << std::endl;
    std::cerr << "        id: key to match, multiple id fields allowed" << std::endl;
    std::cerr << "        any other field names: fields to update, if none given, update " << std::endl;
    std::cerr << "                               all the non-id fields" << std::endl;
    std::cerr << "                               todo: only ascii supported, binary: to implement" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << "    --help,-h: help; --help --verbose: more help" << std::endl;
    std::cerr << "    --empty=<field values>; what field value stands for an empty field" << std::endl;
    std::cerr << "        ascii: default: empty string" << std::endl;
    std::cerr << "        binary: no reasonable default, thus --empty is mandatory" << std::endl;
    std::cerr << "        e.g: --empty=,,empty,,0: for the 3rd field, \"empty\" indicates it has empty value, for the 5th: 0" << std::endl;
    std::cerr << "    --format=<format>; in ascii mode, a hint of data format, e.g. --format=3ui,2d" << std::endl;
    std::cerr << "    --last-block: for each id, output only the last block" << std::endl;
    std::cerr << "                  only for single stdin input. if no --update-non-empty given, it fully overwrite previous values" << std::endl;
    std::cerr << "                  if no --update-non-empty given, it fully overwrite previous values" << std::endl;
    std::cerr << "                  if --update-non-empty is given, keep values from previous block or previous line, see below" << std::endl;
    std::cerr << "    --last-only,--last: output only the result of the last update" << std::endl;
    std::cerr << "                        only for single stdin input" << std::endl;
    std::cerr << "                        default: output updated line on each update" << std::endl;
    std::cerr << "    --matched-only,--matched,-m: output only updates present on stdin" << std::endl;
    std::cerr << "    --remove,--reset,--unset,--erase=<field values>; what field value indicates that previous value should be replaced with empty value" << std::endl;
    std::cerr << "        e.g: --remove=,,remove,,0: for the 3rd field, \"empty\" indicates it has empty value, for the 5th: 0" << std::endl;
    std::cerr << "        the type of reset values has to be correct: number for numeric fields, time for time fields, etc" << std::endl;
    std::cerr << "    --update-line,--line=[<line>]; a one-line update (see examples); a convenience option" << std::endl;
    std::cerr << "        does not output unmatched lines, i.e. behaves as if --matched-only specified" << std::endl;
    std::cerr << "    --update-non-empty-fields,--update-non-empty,-u:" << std::endl;
    std::cerr << "        if update has empty fields, use the field value from stdin (for binary, empty fields must be defined with --empty)" << std::endl;
    std::cerr << "    --verbose,-v: more output to stderr" << std::endl;
    if( more ) { std::cerr << std::endl << "csv options:" << std::endl << comma::csv::options::usage() << std::endl; }
    std::cerr << std::endl;
    if( more )
    {
        std::cerr << "examples" << std::endl;
        std::cerr << "    using update file" << std::endl;
        std::cerr << "        single key" << std::endl;
        std::cerr << "            cat entries.csv | csv-update updates.csv --fields=id" << std::endl;
        std::cerr << "        multiple keys" << std::endl;
        std::cerr << "            cat entries.csv | csv-update updates.csv --fields=id,id" << std::endl;
        std::cerr << "        keys are strings" << std::endl;
        std::cerr << "            cat entries.csv | csv-update updates.csv --fields=id --string" << std::endl;
        std::cerr << "        output only matched entries from update.csv" << std::endl;
        std::cerr << "            cat entries.csv | csv-update updates.csv --fields=id --matched-only" << std::endl;
        std::cerr << "        update only non-empty fields in update.csv" << std::endl;
        std::cerr << "        e.g. if an entry in update.csv is: 0,,1 only the 1st and 3rd fields will be updated" << std::endl;
        std::cerr << "            cat entries.csv | csv-update updates.csv --fields=id --update-non-empty-fields" << std::endl;
        std::cerr << std::endl;
        std::cerr << "    using update line; same semantics as for using update file" << std::endl;
        std::cerr << "        update 1st, 2nd, and 5th fields in all records from entries.csv" << std::endl;
        std::cerr << "            cat entries.csv | csv-update -u --line=1,2,,,3" << std::endl;
        std::cerr << "        update 2nd, and 5th fields in records with 1st column equals 111 from entries.csv" << std::endl;
        std::cerr << "            cat entries.csv | csv-update -u --line=111,2,,,3 --fields=id" << std::endl;
        std::cerr << std::endl;
        std::cerr << "    without update file" << std::endl;
        std::cerr << "        single key" << std::endl;
        std::cerr << "            cat entries.csv | csv-update --fields=id" << std::endl;
        std::cerr << "        output only the results of the last update" << std::endl;
        std::cerr << "            cat entries.csv | csv-update --fields=id --last-only" << std::endl;
        std::cerr << "    last block option " << std::endl;
        std::cerr << "        no updating values" << std::endl;
        std::cerr << "            ( echo 0,1,a,a; echo 0,1,,f; echo 0,2,,b1; echo 0,2,g1, ; echo 0,2,j3,m3 ) | csv-update --fields=id,block --last-block" << std::endl;
        std::cerr << "        with updating values" << std::endl;
        std::cerr << "            ( echo 0,1,a,a; echo 0,1,,f; echo 0,2,,b1; echo 0,2,g1, ) | csv-update --fields=id,block --last-block --update-non-empty" << std::endl;
        std::cerr << std::endl;
        std::cerr << "    erasing values" << std::endl;
        std::cerr << "        echo -e 0,1,a,20140101T000000\\\\n0,-1,b,19000101T000000 | csv-update --fields=id -u --erase=,-1,,19000101T000000" << std::endl;
        std::cerr << "        0,1,a,20140101T000000" << std::endl;
        std::cerr << "        0,,b," << std::endl;
    }
    else
    {
        std::cerr << "examples ... use --help --verbose" << std::endl;
    }
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

struct input_t
{
    comma::csv::impl::unstructured key;
    comma::csv::impl::unstructured value;
    comma::uint32 block;

    input_t() : block( 0 ) {}
    input_t( comma::csv::impl::unstructured key, comma::csv::impl::unstructured value, comma::uint32 block ): key( key ), value( value ), block( block ) {}

    typedef comma::csv::input_stream< input_t > input_stream_t;
};

struct map_t
{
    struct value_type
    {
        unsigned int index;
        input_t value;
        std::string string;

        value_type() {}
        value_type( unsigned int index, const input_t& value, const std::string& string ) : index( index ), value( value ), string( string ) {}
    };
    typedef boost::unordered_map< comma::csv::impl::unstructured, std::vector< value_type >, comma::csv::impl::unstructured::hash > type;
};

namespace comma { namespace visiting {

template <> struct traits< input_t >
{
    template < typename K, typename V > static void visit( const K&, const input_t& p, V& v )
    {
        v.apply( "key", p.key );
        v.apply( "value", p.value );
        v.apply( "block", p.block );
    }
    template < typename K, typename V > static void visit( const K&, input_t& p, V& v )
    {
        v.apply( "key", p.key );
        v.apply( "value", p.value );
        v.apply( "block", p.block );
    }
};

} } // namespace comma { namespace visiting {

static bool verbose;
static comma::csv::options csv;
static boost::scoped_ptr< comma::io::istream > filter_transport;
static std::string filter_line;
static bool has_filter;
static comma::uint32 block = 0;
static bool last_block = false;
static bool last_only = false;
static bool matched_only = false;
static bool update_non_empty = false;
static input_t default_input;
static comma::csv::impl::unstructured empty;
static boost::optional< comma::csv::impl::unstructured > erase;
static map_t::type filter_map;
static map_t::type unmatched;
static map_t::type values;

static void output_unmatched_all()
{
    if( matched_only || !filter_transport ) { return; }
    std::string s;
    for( std::getline( **filter_transport, s ); ( **filter_transport ).good() && !( **filter_transport ).eof(); std::getline( **filter_transport, s ) )
    {
        std::cout << s << std::endl;
    }
}

static void output_and_clear( map_t::type& map, bool do_output, comma::csv::output_stream< input_t >* ostream = NULL )
{
    if( do_output )
    {
        typedef std::map< unsigned int, const map_t::value_type* > output_map_t;
        output_map_t m;
        for( map_t::type::const_iterator it = map.begin(); it != map.end(); ++it )
        {
            for( unsigned int i = 0; i < it->second.size(); m[ it->second[i].index ] = &it->second[i], ++i );
        }
        for( output_map_t::const_iterator it = m.begin(); it != m.end(); ++it )
        {   
            if( ostream ) { ostream->write( it->second->value, it->second->string ); }
            else { std::cout << it->second->string; }
        }
    }
    map.clear();
}

static input_t::input_stream_t* make_filter_stream()
{
    if( filter_transport ) { return new input_t::input_stream_t( **filter_transport, csv, default_input ); }
    if( filter_line.empty() ) { return NULL; }
    comma::csv::options c;
    c.full_xpath = true;
    c.fields = csv.fields;
    static std::istringstream iss( filter_line );
    return new input_t::input_stream_t( iss, c, default_input );
}

static void read_filter_block()
{
    if( !has_filter ) { return; }
    output_and_clear( unmatched, !matched_only );
    static boost::scoped_ptr< input_t::input_stream_t > filter_stream( make_filter_stream() );
    static const input_t* last = filter_stream->read();
    if( !last ) { return; }
    block = last->block;
    filter_map.clear();
    unsigned int count = 0;
    while( last->block == block )
    {
        std::string s;
        if( filter_line.empty() ) // super quick and dirty
        {
            if( csv.binary() ) { s.resize( csv.format().size() ); ::memcpy( &s[0], filter_stream->binary().last(), csv.format().size() ); }
            else { s = comma::join( filter_stream->ascii().last(), csv.delimiter ) + '\n'; }
        }
        filter_map[ last->key ].push_back( map_t::value_type( count++, *last, s ) );
        //if( d.size() > 1 ) {}
        if( verbose ) { if( count % 10000 == 0 ) { std::cerr << "csv-update: reading block " << block << "; loaded " << count << " point[s]; hash map size: " << filter_map.size() << std::endl; } }
        last = filter_stream->read();
        if( !last ) { break; }
    }
    unmatched = filter_map;
    if( verbose ) { std::cerr << "csv-update: read block " << block << " of " << count << " point[s]; got " << filter_map.size() << " different key(s)" << std::endl; }
}

static void output_last( const comma::csv::input_stream< input_t >& istream )
{
    if( istream.is_binary() ) { std::cout.write( istream.binary().last(), csv.format().size() ); }
    else { std::cout << comma::join( istream.ascii().last(), csv.delimiter ) << std::endl; }
}

template < typename V > static void update( V& values, const V& updates, const V& empty_values, const V& erase_values )
{
    for( std::size_t i = 0; i < values.size(); ++i )
    {
        if( updates[i] != empty_values[i] ) { values[i] = ( erase_values.empty() || updates[i] != erase_values[i] ? updates[i] : empty_values[i] ); }
    }
}

static void update( comma::csv::impl::unstructured& value, const comma::csv::impl::unstructured& value_update, bool non_empty_only )
{
    if( !non_empty_only ) { value = value_update; return; }
    static comma::csv::impl::unstructured dummy;
    update( value.longs, value_update.longs, empty.longs, erase ? erase->longs : dummy.longs );
    update( value.strings, value_update.strings, empty.strings, erase ? erase->strings : dummy.strings );
    update( value.doubles, value_update.doubles, empty.doubles, erase ? erase->doubles : dummy.doubles );
    update( value.time, value_update.time, empty.time, erase ? erase->time : dummy.time );
}

static void update( const input_t& v, const comma::csv::input_stream< input_t >& istream, comma::csv::output_stream< input_t >& ostream, const std::string& last = std::string() )
{
    static unsigned int index = 0;
    if( last_block ) 
    {
        std::string s;
        if( csv.binary() ) { s.resize( csv.format().size() ); ::memcpy( &s[0], istream.binary().last(), csv.format().size() ); }
        else { s = last.empty() ? comma::join( istream.ascii().last(), csv.delimiter ) : last; }
        std::vector< map_t::value_type >& e = values[ v.key ];
        
        input_t current = v;
        if( !e.empty() ) 
        { 
            comma::csv::impl::unstructured prev = e.back().value.value;
            update( prev, current.value, update_non_empty );
            if( e.front().value.block != v.block ) {  e.clear();  } 
            current.value = prev;
        }
        e.push_back( map_t::value_type( index++, current, s ) );
    }
    else if( has_filter )
    {
        map_t::type::const_iterator it = filter_map.find( v.key );
        if( it == filter_map.end() || it->second.empty() )
        {
            if( last.empty() ) { output_last( istream ); }
            else { std::cout << last << std::endl; }
            return;
        }
        input_t current = v;
        for( std::size_t i = 0; i < it->second.size(); ++i ) // todo: output last only
        {
            update( current.value, it->second[i].value.value, update_non_empty );
            if( last.empty() ) { ostream.write( current, istream ); }
            else { ostream.write( current, last ); }
        }
        unmatched.erase( it->first );
    }
    else
    {
        if( v.block != block ) { output_and_clear( values, last_only, &ostream ); }
        block = v.block;
        if( last.empty() )
        {
            std::string s;
            if( csv.binary() ) { s.resize( csv.format().size() ); ::memcpy( &s[0], istream.binary().last(), csv.format().size() ); }
            else { s = comma::join( istream.ascii().last(), csv.delimiter ); }
            map_t::type::iterator it = values.find( v.key );
            if( it == values.end() )
            {
                values[ v.key ].push_back( map_t::value_type( index++, v, s ) );
                if( !last_only ) { ostream.write( v, s ); }
            }
            else
            {
                update( it->second[0].value.value, v.value, update_non_empty );
                it->second[0].index = index++;
                it->second[0].string = s;
                if( !last_only ) { ostream.write( it->second[0].value, s ); }
            }
        }
        else
        {
            values[ v.key ].push_back( map_t::value_type( index++, v, last ) );
            if( !last_only ) { ostream.write( v, last ); }
        }
    }
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        verbose = options.exists( "--verbose,-v" );
        csv = comma::csv::options( options );
        csv.full_xpath = true;
        csv.quote.reset();
        last_only = options.exists( "--last-only,--last" );
        last_block = options.exists( "--last-block" );
        options.assert_mutually_exclusive( "--last-block,--last,--last-only" );
        options.assert_mutually_exclusive( "--last-block,--matched-only,--matched,-m" );
        options.assert_mutually_exclusive( "--last-block,--remove,--reset,--unset,--erase" );
        //options.assert_mutually_exclusive( "--last-block,--empty" );
        update_non_empty = options.exists( "--update-non-empty-fields,--update-non-empty,-u" );
        std::vector< std::string > unnamed = options.unnamed( "--last-block,--last-only,--last,--matched-only,--matched,-m,--string,-s,--update-non-empty-fields,--update-non-empty,-u,--verbose,-v", "-.*" );
        if( unnamed.size() > 1 ) { std::cerr << "csv-update: expected one file or stream to join, got " << comma::join( unnamed, ' ' ) << std::endl; return 1; }
        if( !unnamed.empty() ) { filter_transport.reset( new comma::io::istream( unnamed[0], options.exists( "--binary,-b" ) ? comma::io::mode::binary : comma::io::mode::ascii ) ); }
        filter_line = options.value< std::string >( "--update-line,--line", "" );
        has_filter = filter_transport || !filter_line.empty();
        matched_only = options.exists( "--matched-only,--matched,-m" ) || !filter_line.empty();
        std::vector< std::string > v = comma::split( csv.fields, ',' );
        bool has_value_fields = false;
        for( std::size_t i = 0; !has_value_fields && i < v.size(); has_value_fields = !v[i].empty() && v[i] != "block" &&  v[i] != "id", ++i );
        std::string first_line;
        comma::csv::format f;
        if( csv.binary() ) { f = csv.format(); }
        else if( options.exists( "--format" ) ) { f = comma::csv::format( options.value< std::string >( "--format" ) ); }
        else
        {
            while( std::cin.good() && first_line.empty() ) { std::getline( std::cin, first_line ); }
            if( first_line.empty() ) { output_unmatched_all(); return 0; }
            f = comma::csv::impl::unstructured::guess_format( first_line, csv.delimiter );
            if( verbose ) { std::cerr << "csv-update: guessed format: " << f.string() << std::endl; }
        }
        unsigned int size = f.count();
        for( std::size_t i = 0; i < size; ++i )
        {
            if( i < v.size() )
            {
                if( v[i] == "block" ) { continue; }
                if( v[i] == "id" ) { v[i] = "key/" + default_input.key.append( f.offset( i ).type ); continue; }
            }
            if( !has_value_fields || !v[i].empty() )
            {
                v.resize( size );
                v[i] = "value/" + default_input.value.append( csv.binary() ? f.offset( i ).type : comma::csv::format::fixed_string ); // quick and dirty
            }
        }
        //if( default_input.key.empty() ) { std::cerr << "csv-update: please specify at least one id field" << std::endl; return 1; }
        csv.fields = comma::join( v, ',' );
        if( verbose ) { std::cerr << "csv-update: csv fields: " << csv.fields << std::endl; }
        comma::csv::input_stream< input_t > istream( std::cin, csv, default_input );
        comma::csv::output_stream< input_t > ostream( std::cout, csv, default_input );
        if( csv.binary() && update_non_empty && !options.exists( "--empty" ) ) { std::cerr << "csv-update: in binary mode with --update-non-empty, please specify --empty" << std::endl; return 1; }
        empty = default_input.value; // todo: handle --empty
        for( unsigned int i = 0; i < empty.longs.size(); ++i ) { empty.longs[i] = std::numeric_limits< comma::int64 >::max(); } // quick and dirty
        for( unsigned int i = 0; i < empty.doubles.size(); ++i ) { empty.doubles[i] = std::numeric_limits< double >::max(); } // quick and dirty
        if( options.exists( "--empty" ) )
        {
            if( !update_non_empty ) { std::cerr << "csv-update: --empty implemented only in combination with --update-non-empty" << std::endl; return 1; }
            std::string s = options.value< std::string >( "--empty" ) + std::string( f.count(), ',' );
            std::istringstream iss( s );
            comma::csv::options c;
            c.full_xpath = true;
            c.fields = csv.fields;
            comma::csv::input_stream< input_t > isstream( iss, c, default_input );
            empty = ( isstream.read() )->value;
        }
        default_input.value = empty;
        if( options.exists( "--remove,--reset,--unset,--erase" ) )
        {
            if( !update_non_empty ) { std::cerr << "csv-update: --erase implemented only in combination with --update-non-empty" << std::endl; return 1; }
            std::string s = options.value< std::string >( "--remove,--reset,--unset,--erase" ) + std::string( f.count(), ',' );
            std::istringstream iss( s );
            comma::csv::options c;
            c.full_xpath = true;
            c.fields = csv.fields;
            comma::csv::input_stream< input_t > isstream( iss, c, default_input );
            erase = ( isstream.read() )->value;
        }
        read_filter_block();
        if( !first_line.empty() ) { update( comma::csv::ascii< input_t >( csv, default_input ).get( first_line ), istream, ostream, first_line ); }
        while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
        {
            const input_t* p = istream.read();
            if( !p ) { break; }
            if( block != p->block ) { read_filter_block(); }
            update( *p, istream, ostream );
        }
        if( has_filter ) { output_and_clear( unmatched, !matched_only ); }
        else { output_and_clear( values, last_only || last_block, &ostream ); }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-update: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-update: unknown exception" << std::endl; }
    return 1;
}
