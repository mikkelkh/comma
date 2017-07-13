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

/// @author dewey nguyen

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <iostream>
#include <deque>

#ifndef WIN32
#include <sysexits.h>
#endif

#include <boost/unordered_map.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../csv/impl/unstructured.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static const char* name() { return "csv-blocks: "; }

struct input_t
{
    comma::csv::impl::unstructured key;
    input_t()  {}
    input_t( const comma::csv::impl::unstructured& key ): key( key ) {}
};

struct input_with_block
{
    comma::uint32 block;
    input_with_block() : block( 0 ) {}
};

struct input_with_index
{
    comma::uint32 index;
    input_with_index() : index( 0 ) {}
};

struct appended_column
{
    comma::uint32 value;
    appended_column( comma::uint32 val = 0 ) : value( val ) {}
};

struct accumulate_input
{
    comma::uint32 block;
    comma::csv::impl::unstructured key;
    accumulate_input() : block( 0 ) {}
    typedef boost::unordered_map< comma::csv::impl::unstructured, std::string, comma::csv::impl::unstructured::hash > unordered_map;
};

namespace comma { namespace visiting {

template <> struct traits< input_t >
{
    template < typename K, typename V > static void visit( const K&, const input_t& p, V& v ) { v.apply( "key", p.key ); }
    template < typename K, typename V > static void visit( const K&, input_t& p, V& v ) { v.apply( "key", p.key ); }
};

template <> struct traits< input_with_block >
{
    template < typename K, typename V > static void visit( const K&, const input_with_block& p, V& v ) { v.apply( "block", p.block ); }
    template < typename K, typename V > static void visit( const K&, input_with_block& p, V& v ) { v.apply( "block", p.block ); }
};

template <> struct traits< input_with_index >
{
    template < typename K, typename V > static void visit( const K&, const input_with_index& p, V& v ) { v.apply( "index", p.index ); }
    template < typename K, typename V > static void visit( const K&, input_with_index& p, V& v ) { v.apply( "index", p.index ); }
};

template <> struct traits< appended_column >
{
    template < typename K, typename V > static void visit( const K&, const appended_column& p, V& v ) { v.apply( "value", p.value ); }
    template < typename K, typename V > static void visit( const K&, appended_column& p, V& v ) { v.apply( "value", p.value ); }
};

template <> struct traits< accumulate_input >
{
    template < typename K, typename V > static void visit( const K&, const accumulate_input& p, V& v )
    { 
        v.apply( "block", p.block );
        v.apply( "key", p.key );
    }
    
    template < typename K, typename V > static void visit( const K&, accumulate_input& p, V& v )
    { 
        v.apply( "block", p.block );
        v.apply( "key", p.key );
    }
};

} } // namespace comma { namespace visiting {

static void usage( bool more )
{
    std::cerr << std::endl;
    std::cerr << "operations contiguous blocks of csv data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat some-data.csv | csv-blocks <operation> [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "operations" << std::endl;
    std::cerr << "    accumulate" << std::endl;
    std::cerr << "        accumulate records from block to block, keeping the last seen record for each id" << std::endl;
    std::cerr << "        attention: output does not preserve input order, since there is no reasonable tradeof there" << std::endl;
    std::cerr << "                   use csv-sort for post-processing, if required" << std::endl;
    std::cerr << "    group|make-blocks" << std::endl;
    std::cerr << "        cat something.csv | csv-blocks group --fields=,id, " << std::endl;
    std::cerr << "            appends group's block field base on specified id key or keys" << std::endl;
    std::cerr << "    head" << std::endl;
    std::cerr << "        reads records from first block to stdout, if --num-of-blocks=<num> specified, read more than one blocks" << std::endl;
    std::cerr << "        requires the index from 'index' mode in the inputs" << std::endl;
    std::cerr << "            cat something.csv | csv-blocks index --fields=,index " << std::endl;
    std::cerr << "    increment" << std::endl;
    std::cerr << "        increments specified field value, must be uint32 type - any such field can be used as a block:" << std::endl;
    std::cerr << "            cat something.csv | csv-blocks increment --fields=,,block" << std::endl;
    std::cerr << "            cat something.csv | csv-blocks increment --fields=,,block --step 2" << std::endl;
    std::cerr << "            cat something.csv | csv-blocks increment --fields=,,block --step -1" << std::endl;
    std::cerr << "    index" << std::endl;
    std::cerr << "        appends an index field counting the number of records for each block. Use --reverse for count down." << std::endl;
    std::cerr << "            cat something.csv | csv-blocks index --fields=,block " << std::endl;
    std::cerr << "    read-until" << std::endl;
    std::cerr << "        reads records until a given index value, e.g:" << std::endl;
    std::cerr << "            cat something.csv | csv-blocks read-until --fields=,index --index=5" << std::endl;
    std::cerr << "        options" << std::endl;
    std::cerr << "            --index=<index>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --help,-h: help; --help --verbose: more help" << std::endl;
    std::cerr << "    --reverse; use with 'index' operation, output the indices in descending order instead of ascending" << std::endl;
    std::cerr << "    --from,--starting-block; use with 'group' operation, the starting block number to use, default is 1" << std::endl;
    std::cerr << "    --step; use with 'increment' operation, the number of increment/decrement for specified field, default is 1" << std::endl;
    std::cerr << "    --lines,--num-of-blocks,-n; use with 'head' operation, outputs only the first specified number of blocks, default is 1" << std::endl;
    std::cerr << "    --strict; use with 'head' operation, exits with an error if a full block (as indicated by the index number) is not found" << std::endl;
    if( more ) { std::cerr << comma::csv::options::usage() << std::endl << std::endl; }
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << "    block_csv=block.csv" << std::endl;
    std::cerr << "    ( echo \"a,1,2,3\"; echo \"a,4,2,3\"; echo \"b,5,5,6\"; echo \"c,7,5,6\"; echo \"c,7,8,9\"; echo \"c,7,8,9\" ) >$block_csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    group|make-blocks" << std::endl;
    std::cerr << "        cat $block_csv | csv-blocks group --fields=id" << std::endl;
    std::cerr << "            unique ascending block number are assigned based on one id field" << std::endl;
    std::cerr << "        cat $block_csv | csv-blocks group --fields=id,,id" << std::endl;
    std::cerr << "            unique ascending block number are assigned based on two id fields" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    index" << std::endl;
    std::cerr << "        cat $block_csv | csv-blocks group --fields=id | csv-blocks index --fields=,,,,block" << std::endl;
    std::cerr << "            Append will add block field at the end, 'index' will append how many lines/records are left in the block" << std::endl;
    std::cerr << "            See 'head operation' below" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    increment" << std::endl;
    std::cerr << "        cat $block_csv | csv-blocks increment --fields=,block " << std::endl;
    std::cerr << "            Given an integer field (any field), mark it as a block field so the field value is incremented." << std::endl;
    std::cerr << "            This increments the second field" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    head" << std::endl;
    std::cerr << "        cat $block_csv | csv-blocks group --fields=id | csv-blocks index --fields=,,,,block | csv-blocks head --fields=,,,,,index " << std::endl;
    std::cerr << "            After appending the block field, then the block reverse index field, reading a single block from the input is possible" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    index and head together" << std::endl;
    std::cerr << "        echo -e \"0\\n0\\n1\\n1\\n1\\n0\\n0\\n0\\n0\" | csv-blocks index --fields=block --reverse | while true ; do" << std::endl;
    std::cerr << "            block=$( csv-blocks head --fields block,index ) || break ; echo \"---\" ; echo \"$block\"" << std::endl;
    std::cerr << "        done" << std::endl;
    std::cerr << "            After indexing the input in reverse order, the stream is read block-by-block, with the first line given the maximal" << std::endl;
    std::cerr << "            element id in the block, essentially, the block size" << std::endl;
    std::cerr << std::endl;
    std::cerr << "contact info: " << comma::contact_info <<std::endl;
    std::cerr << std::endl;
    exit(0);
}

static bool verbose;
static bool strict = false;
static comma::csv::options csv;
static bool reverse_index = false;
// All the data for this block
static std::deque< std::string > block_records;
static comma::csv::impl::unstructured keys;
static comma::uint32 current_block = 1;
static comma::int32 increment_step = 1;

static void flush_indexing( std::deque< std::string >& block_records, bool is_binary, char delimiter )
{
    std::size_t size = block_records.size();
    for( std::size_t i=0; i<size; ++i ) 
    {
        const std::string& input = block_records[i];
        comma::uint32 index = reverse_index ? size - (i+1) : i ;
        
        if( is_binary ) 
        { 
            std::cout.write( &input[0], input.size() );
            std::cout.write( (const char *) &index, sizeof(comma::uint32)); 
        }
        else 
        { 
            std::cout << input << delimiter << index << std::endl; 
        }
    }
    block_records.clear();
}

template < typename T > static void set_fields( const comma::command_line_options& options, std::string& first_line, T& default_input )
{
    std::vector< std::string > v = comma::split( csv.fields, ',' );
    comma::csv::format f;
    if( csv.binary() ) { f = csv.format(); }
    else if( options.exists( "--format,--binary" ) ) { f = comma::csv::format( options.value< std::string >( "--format,--binary" ) ); }
    else
    {
        while( std::cin.good() && first_line.empty() ) { std::getline( std::cin, first_line ); }
        if( first_line.empty() ) { std::cerr << name() << "--format= is missing however the first line is empty" ; exit( 1 ); }
        f = comma::csv::impl::unstructured::guess_format( first_line, csv.delimiter );
        if( verbose ) { std::cerr << name() << "guessed format: " << f.string() << std::endl; }
    }
    // This is to load the keys into input_t structure
    unsigned int size = f.count();
    for( std::size_t i = 0; i < size; ++i ) { if( i < v.size() ) { if( v[i] == "id" ) { v[i] = "key/" + default_input.key.append( f.offset( i ).type ); continue; } } }
    csv.fields = comma::join( v, ',' );
}

#ifndef WIN32

struct memory_buffer
{
    char* buffer;
    size_t size;
    memory_buffer();
    memory_buffer( size_t size ) { allocate( size ); }
    ~memory_buffer();
    
    void allocate( size_t size );
    
    // if strict is true, fails when less data then expected is received
    comma::uint32 read_binary_records( comma::uint32 num_record=1, bool strict=false );  
    
};
memory_buffer::memory_buffer() : buffer(NULL), size(0) {}
memory_buffer::~memory_buffer()
{
    if( buffer != NULL ) {  
//         std::cerr  << "freeing head ascii memory, size: " << size << std::endl;
        free( (void*) buffer ); 
    }
}

void memory_buffer::allocate(size_t size)
{
    this->size = size;
    buffer = (char*) ::malloc( size );
    if( buffer == NULL ) { std::cerr << name() << "failed to allocate memory buffer of size: " << size << std::endl; exit(1); }
}

comma::uint32 memory_buffer::read_binary_records( comma::uint32 num_record, bool strict )
{
    static comma::uint32 one_record_size = csv.format().size();
    comma::uint32 bytes_to_read = one_record_size * num_record;
    size_t bytes_read = 0;
    while ( bytes_read < bytes_to_read && !::feof( stdin ) && !::ferror(stdin) )
    {
        bytes_read +=  ::fread( &buffer[bytes_read], 1, bytes_to_read - bytes_read, stdin );
    }
    
    if ( bytes_read == 0 ) { exit( EX_NOINPUT ); } // signals end of stream, not an error
    
    if( bytes_read % one_record_size != 0 ) { std::cerr << name() << "expected " << one_record_size << " bytes, got only read: " << ( bytes_read % one_record_size ) << std::endl; exit( 1 ); } 
    if ( strict && (bytes_read < bytes_to_read) ) { std::cerr << name() << "expected " << bytes_to_read << " bytes (" << ( bytes_to_read / one_record_size  ) << " records); got only: " << bytes_read << " bytes (" << ( bytes_read / one_record_size ) << " records)" << std::endl; exit( 1 ); } 
    
    return bytes_read;
}

static comma::uint32 extended_buffer_size = 65536; // 64Kb, default, overwritten on demand if a record is longer

static comma::uint32 read_and_write_binary_record()
{
    static memory_buffer memory( csv.format().size() );
    static comma::csv::binary< input_with_index > binary( csv );
    // Reads from stdin, the exact number of bytes to be memory.size
    //  It should blocks and keep reading until we have the entire message,
    //  This is to reassemble the message if it is broken into pieces (e.g. TCP input piped into stdin)
    comma::uint32 num_of_bytes = memory.read_binary_records(1, strict);
    std::cout.write( memory.buffer, num_of_bytes ); // send data to stdout, expect it to read one record
    
    static input_with_index record;
    // fill 'record' param
    binary.get( record, memory.buffer );
    return record.index;
}

/// Read a record and  fills out param 'record', the binary data is immediately send to stdout
static void read_and_write_binary_block()
{
    comma::uint32 records_to_read = read_and_write_binary_record();
    if( records_to_read == 0 ) { return; }
    static const comma::uint32 one_record_size = csv.format().size();
    extended_buffer_size = std::max( extended_buffer_size, one_record_size );
    static memory_buffer extended_buffer( extended_buffer_size );
    // maximum number that will fit in the extened_buffer
    comma::uint32 records_in_buffer = comma::uint32(extended_buffer_size / one_record_size) ;
    // only read up to the number of bytes we can hold
    comma::uint32 read_per_loop = (records_to_read <= records_in_buffer) ? records_to_read : records_in_buffer;
    comma::uint32 records_read = 0;
    while( records_read < records_to_read  && !::feof( stdin ) && !::ferror(stdin) )
    {
        if( read_per_loop > records_to_read  - records_read ) { read_per_loop = records_to_read  - records_read; }
        comma::uint32 read_chunk_size = extended_buffer.read_binary_records( read_per_loop, strict);
        std::cout.write( extended_buffer.buffer, read_chunk_size );
        records_read += read_per_loop;
    }
}

/// Read a record and  fills out param 'record', the binary data is immediately send to stdout
// when return value is false, 'record' is not filled because line read is empty
static boost::optional< comma::uint32 > read_and_write_ascii_record()
{
    static memory_buffer memory( 1024 );
    static comma::csv::ascii< input_with_index > ascii( csv );
    while( true )
    {
        input_with_index record;
        std::stringstream sstream;
        bool has_end_of_line = false;
        while( !has_end_of_line && !::feof(stdin) )
        {
            // getline may actually changes the buffer with realloc and extend the size
            ssize_t bytes_read = ::getline( &memory.buffer, &memory.size, stdin );
            if ( bytes_read <= 0 ) { return boost::none; } 
#ifndef WIN32
            has_end_of_line = ( memory.buffer[bytes_read-1] == '\n' );
            sstream.write( memory.buffer, has_end_of_line ? bytes_read-1 : bytes_read );
#else
            has_end_of_line = ( bytes_read > 1 
                                && memory.buffer[bytes_read-2] == '\r' 
                                && memory.buffer[bytes_read-1] == '\n' );
            sstream.write( memory.buffer, has_end_of_line ? bytes_read-2 : bytes_read );
#endif
        }        
        std::string line = sstream.str();
        if( line.empty() ) { continue; }
        // filling 'record' param
        ascii.get( record, line );
        // send line to stdout
        std::cout << line << std::endl;
        return record.index; // 'record' filled
    }
}

#endif // #ifndef WIN32

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        verbose = options.exists( "--verbose,-v" );
        strict = options.exists( "--strict" );
        csv = comma::csv::options( options );
        csv.full_xpath = true;
        csv.quote.reset();
        comma::csv::options csv_out;
        if( csv.binary() ) { csv_out.format( comma::csv::format("ui") ); }
        std::vector< std::string > unnamed = options.unnamed( "--help,-h,--reverse,--verbose,-v", "-.*" );
        if( unnamed.empty() ) { std::cerr << name() << "please specify operation" << std::endl; return 1; }
        const std::string  operation = unnamed.front();
        
        if( verbose ) { std::cerr << name() << "csv fields: " << csv.fields << std::endl; }
        
        if( operation == "accumulate" )
        {
            std::string first_line;
            accumulate_input default_input;
            set_fields( options, first_line, default_input );
            if( verbose ) { std::cerr << name() << "csv fields: " << csv.fields << std::endl; }
            if ( default_input.key.empty() ) { std::cerr << name() << "please specify at least one id field" << std::endl; return 1; }
            accumulate_input::unordered_map map;
            comma::csv::input_stream< accumulate_input > istream( std::cin, csv, default_input );
            comma::uint32 block = 0;
            if( !first_line.empty() ) 
            { 
                accumulate_input p = comma::csv::ascii< accumulate_input >( csv, default_input ).get( first_line ); 
                block = p.block;
                map[ p.key ] = first_line;
            }
            std::vector< std::string > fields = comma::split( csv.fields, ',' );
            for( unsigned int i = 0; i < fields.size(); ++i ) { if( fields[i] != "block" ) { fields[i] = ""; } }
            comma::csv::options block_csv = csv;
            block_csv.fields = comma::join( fields, ',' );
            boost::scoped_ptr< comma::csv::ascii< accumulate_input > > ascii;
            boost::scoped_ptr< comma::csv::binary< accumulate_input > > binary;
            if( csv.binary() ) { binary.reset( new comma::csv::binary< accumulate_input >( block_csv, default_input ) ); }
            else { ascii.reset( new comma::csv::ascii< accumulate_input >( block_csv, default_input ) ); }            
            while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
            {
                const accumulate_input* p = istream.read();
                if( !p || block != p->block )
                {
                    for( accumulate_input::unordered_map::iterator it = map.begin(); it != map.end(); ++it )
                    {
                        std::cout.write( &( it->second[0] ), it->second.size() );
                        if( !csv.binary() ) { std::cout << std::endl; }
                    }
                    if( csv.flush ) { std::cout.flush(); }
                    if( p )
                    {
                        block = p->block;
                        for( accumulate_input::unordered_map::iterator it = map.begin(); it != map.end(); ++it ) { if( binary ) { binary->put( *p, &it->second[0] ); } else { ascii->put( *p, it->second ); } }
                    }
                }
                if( !p ) { break; }
                map[ p->key ] = istream.last();
            }
            return 0;
        }
        if( operation == "group" || operation == "make-blocks" )
        {
            current_block = options.value< comma::uint32 >( "--starting-block,--from", 0 ); // default is 0
            
            std::string first_line;
            input_t default_input;
            set_fields( options, first_line, default_input );
            if( verbose ) { std::cerr << name() << "csv fields: " << csv.fields << std::endl; }
            if ( default_input.key.empty() ) { std::cerr << name() << "please specify at least one id field" << std::endl; return 1; }
            
            comma::csv::input_stream< input_t > istream( std::cin, csv, default_input );
            comma::csv::output_stream< appended_column > ostream( std::cout, csv_out );
            comma::csv::tied< input_t, appended_column > tied( istream, ostream );
            
            if( !first_line.empty() ) 
            { 
                input_t p = comma::csv::ascii< input_t >( csv, default_input ).get( first_line ); 
                if( !(keys == p.key) ) { ++current_block; }
                keys = p.key;
                // This is needed because the record wasnt read in by istream
                // Write it out
                if( istream.is_binary() ) { std::cout.write( (char*)&p, istream.binary().size() ); }
                else { std::cout << first_line << istream.ascii().ascii().delimiter(); }
                ostream.write( appended_column( current_block ) );
            }
            while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
            {
                const input_t* p = istream.read();
                if( !p ) { break; }
                if( !(keys == p->key) ) { ++current_block; }
                keys = p->key;
                tied.append( appended_column( current_block ) );
            }
            
            return 0;
        }
        else if( operation == "head" )
        {
#ifdef WIN32
            std::cerr << "csv-blocks: head: not implemented on windows" << std::endl; return 1;
#else // #ifdef WIN32
            ::setvbuf( stdin, (char *)NULL, _IONBF, 0 );
            #ifdef WIN32
                if( csv.binary() ) { _setmode( _fileno( stdin ), _O_BINARY ); } // for the time, when windows actually will be supported... maybe...
            #endif
            comma::uint32 num_of_blocks = options.value< comma::uint32 >( "--lines,--num-of-blocks,-n", 1 );
            
            if( !csv.binary() )
            {
                comma::uint32 index = 0;    // set as 0 for previous_index at start
                while( num_of_blocks > 0 ) 
                {
                    boost::optional< comma::uint32 > i = read_and_write_ascii_record();
                    if( !i )
                    {
                        if( index != 0 )
                        { 
                            std::cerr << name() << "failed to read a full block, last index read is:" << index <<  ", hence " << index << " more record/s expected " << strict << std::endl;
                            if( strict ) { return 1; }
                        }
                        return EX_NOINPUT; // we are done, end of stream
                    }
                    index = *i;
                    if( index == 0 ) { --num_of_blocks; } 
                }
            }
            else
            {
                for( std::size_t n=0; n <num_of_blocks; ++n ) { read_and_write_binary_block(); }
            }
            return 0;
#endif // #ifdef WIN32
        }
        else if( operation == "index" )
        {
            reverse_index = options.exists("--reverse");
            
            comma::csv::input_stream< input_with_block > istream( std::cin, csv );
            
            char delimiter = istream.is_binary() ? ',' : istream.ascii().ascii().delimiter();
            comma::uint32 block = 0;
            std::string buffer;
            if( istream.is_binary() ) { buffer.resize( istream.binary().size() ); }
            
            while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
            {
                const input_with_block* p = istream.read();
                if( !p ) { break; }
                
                if( block != p->block && !block_records.empty() ) { flush_indexing( block_records, istream.is_binary(), delimiter  ); }
                block = p->block;
                
                // Put the input into the buffer
                if( istream.is_binary() )  
                { 
                    ::memcpy( &buffer[0], istream.binary().last(),  istream.binary().size() ); 
                    block_records.push_back( buffer );
                }
                else { block_records.push_back( comma::join( istream.ascii().last(), delimiter ) ); }
            }
            
            // flushes the last block
            flush_indexing( block_records, istream.is_binary(), delimiter  );
            
            return 0;
        }
        else if( operation == "increment" )    // operation is head
        {
            increment_step = options.value< comma::int32 >( "--step", 1 );
            
            comma::csv::input_stream< input_with_block > istream( std::cin, csv );
            comma::csv::output_stream< appended_column > ostream( std::cout, csv_out );
            comma::csv::tied< input_with_block, appended_column > tied( istream, ostream );
            
            appended_column incremented;
            while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
            {
                const input_with_block* p = istream.read();
                if( !p ) { break; }
                
                incremented.value = p->block + increment_step;
                tied.append( incremented );
            }
            
            return 0;
        }
        else if( operation == "read-until" )
        {
#ifdef WIN32
            std::cerr << "csv-blocks: read-until: not implemented on windows" << std::endl; return 1;
#else // #ifdef WIN32
            ::setvbuf( stdin, (char *)NULL, _IONBF, 0 );
            #ifdef WIN32
                if( csv.binary() ) { _setmode( _fileno( stdin ), _O_BINARY ); } // for the time, when windows actually will be supported... maybe...
            #endif
            comma::uint32 index = options.value< comma::uint32 >( "--index" );
            while( true ) 
            {
                boost::optional< comma::uint32 > i = csv.binary() ? read_and_write_binary_record() : read_and_write_ascii_record();
                if( !i || *i == index ) { break; }
            }
            return 0;
#endif // #ifdef WIN32
        }
        else { std::cerr << name() << "unrecognised operation '" << operation << "'" << std::endl; }
        
        return 1;
    }
    catch( std::exception& ex ) { std::cerr << name() << ex.what() << std::endl; }
    catch( ... ) { std::cerr << name() << "unknown exception" << std::endl; }
    return 1;
}
