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


#ifndef WIN32
#include <sysexits.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <iostream>
#include <cctype>
#include <vector>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/exception.h"
#include "../../base/types.h"
#include "../../io/stream.h"
#include "../../io/select.h"
#include "../../string/string.h"
#include "../../csv/stream.h"

void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "buffer stdin data to synchronised output data to stdout using a file lock" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: io-buffer <in|out> --lock-file <file> [--size <size>] [--lines <num>] --buffer-size <size>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << " *  --lock-file,--lock=: an exiting or new filepath to be used as a file lock, content will be truncated if it exists." << std::endl;
    std::cerr << "    --lines,-n=[<num>]: for line based text input data, buffers number of lines before attempting to write to stdout, default is 1" << std::endl;
    std::cerr << "    --size,-s=[<bytes>]: for binary input data, where each message is of size x in bytes" << std::endl;
    std::cerr << "    --buffer-size,-b=[<size>]: for binary input data, binary storage to store multiple of x sized messages." << std::endl;
    std::cerr << "                                  see --size and buffer size suffixes below." << std::endl;
    std::cerr << "                                  if not specified, no buffering of binary messages, each is written to stdout." << std::endl;
    std::cerr << "    --strict; use with 'in' operation, exits with an error if a full message size (binary) is not found" << std::endl;
    std::cerr << std::endl;
    std::cerr << "operations" << std::endl;
    std::cerr << "    out: read in standard input, attempt to write to stdout when buffer is full." << std::endl;
    std::cerr << "    in: read in standard input, if buffer is full then write to standard output and exits" << std::endl;
    std::cerr << std::endl;
    std::cerr << std::endl;
    std::cerr << "<buffer size suffixes>" << std::endl;
    std::cerr << "    Mb: megabytes e.g. 5Mb" << std::endl;
    std::cerr << "    Kb: kilobytes e.g. 10Kb" << std::endl;
    std::cerr << "    no suffix: bytes e.g. 256" << std::endl;
    std::cerr << std::endl;
    std::cerr << "supported address types: tcp, udp, local (unix) sockets, named pipes, files, zmq (todo)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << std::endl;
    std::cerr << "out operation" << std::endl;
    std::cerr << "    text based input" << std::endl;
    std::cerr << "        io-buffer out --lock-file=/tmp/lockfile " << std::endl;
    std::cerr << "          Read each input line and write to standard output, no buffering" << std::endl;
    std::cerr << "        io-buffer out --lock-file=/tmp/lockfile --lines=3" << std::endl;
    std::cerr << "          Read each input line into the buffer that can store up to 3 lines" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    binary input" << std::endl;
    std::cerr << "        io-buffer out --lock-file=/tmp/lockfile --size 512 " << std::endl;
    std::cerr << "          Read each input message of 512 bytes and writes to standard output, no buffering" << std::endl;
    std::cerr << "        io-buffer out --lock-file=/tmp/lockfile --size 512 --buffer-size 10kb " << std::endl;
    std::cerr << "          Read each input message of 512 bytes and saves into 10Kb buffer (20 messages maximum). If buffer is full, output buffer." << std::endl;
    std::cerr << "in operation" << std::endl;
    std::cerr << "          See 'out' operation, in this mode, the program write to standard output and exits when buffer is full." << std::endl;
    std::cerr << "          Call io-buffer multiple times to read more input data." << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

const char* name() { return "io-buffer: "; }


typedef boost::optional< std::vector< std::string > > lines_buffer_type;
typedef std::vector< char >  char_buffer_t;
typedef boost::optional< char_buffer_t > binary_buffer_type;

lines_buffer_type lines_buffer;
binary_buffer_type binary_buffer;
static bool verbose = false;

static comma::uint64 get_buffer_size( const std::string& str )
{
    std::string type;
    std::string digits;
    // Get size type, todo use something smarter
    for( std::size_t i=0; i<str.size(); ++i) { 
        if( std::isdigit(str[i]) ) { digits += str[i]; } else  { type = str.substr( i ); break; }
    }
//     std::cerr << "digits: " << digits << " type: " << type << std::endl;
    
    comma::uint64 bytes = 0;
    try { bytes = boost::lexical_cast< comma::uint64 >( digits ); } 
    catch(...) { COMMA_THROW( comma::exception, "failed to parse buffer size: " << str ); }
    
    if( type.empty() ){ }
    else if ( boost::iequals(type, "mb") ) { bytes *= 1024000; }
    else if ( boost::iequals(type, "kb") ) { bytes *= 1024; }
    else { std::cerr << name() << "failed to parse buffer size argument: '" << str << "', please see --help" << std::endl;  }
    
    return bytes;
}

using boost::interprocess::file_lock;
using boost::interprocess::scoped_lock;

static bool strict = false;

static void output_binary( file_lock& lock )
{
    scoped_lock< file_lock > filelock( lock );  // Blocks until it has the lock
    std::cout.write( &(binary_buffer.get()[0]), binary_buffer->size() );
}

static void output_text( file_lock& lock )
{
    scoped_lock< file_lock > filelock( lock );  // Blocks until it has the lock
    for( std::size_t i=0; i<lines_buffer->size(); ++i ) { std::cout << lines_buffer.get()[i] << std::endl;  }
}

/// Because we need to use C code for IO input operations
// This structure help manage the C code, memory allocation and de-allocation
// ::getline() may actually re-allocate the buffer given to increase the size
struct memory_buffer
{
    char* buffer;
    size_t size;
    memory_buffer();
    memory_buffer( size_t size ) { allocate( size ); }
    ~memory_buffer();
    
    void allocate( size_t size );
    
    // if strict is true, fails when less data then expected is received
    comma::uint32 read_binary_records( comma::uint32 size, comma::uint32 num_record=1, bool strict=false );  
    
};
memory_buffer::memory_buffer() : buffer(NULL), size(0) {}
memory_buffer::~memory_buffer() { if( buffer != NULL ) { free( (void*) buffer );  } }

void memory_buffer::allocate(size_t size)
{
    this->size = size;
    buffer = (char*) ::malloc( size );
    if( buffer == NULL ) { std::cerr << name() << "failed to allocate memory buffer of size: " << size << std::endl; exit(1); }
}

comma::uint32 memory_buffer::read_binary_records( comma::uint32 size, comma::uint32 num_record, bool strict )
{
    static comma::uint32 one_record_size = size;
    comma::uint32 bytes_to_read = one_record_size * num_record;
    size_t bytes_read = 0;
    while ( bytes_read < bytes_to_read && !::feof( stdin ) && !::ferror(stdin) )
    {
        bytes_read +=  ::fread( &buffer[bytes_read], 1, bytes_to_read - bytes_read, stdin );
    }
    
    if ( bytes_read <= 0 ) { return bytes_read; } // signals end of stream, not an error
    
    if( bytes_read % one_record_size != 0 ) { std::cerr << name() << "expected " << one_record_size << " bytes, got only read: " << ( bytes_read % one_record_size ) << std::endl; exit( 1 ); } 
    if ( strict && (bytes_read < bytes_to_read) ) { std::cerr << name() << "expected " << bytes_to_read << " bytes (" << ( bytes_to_read / one_record_size  ) << " records); got only: " << bytes_read << " bytes (" << ( bytes_read / one_record_size ) << " records)" << std::endl; exit( 1 ); } 
    
    return bytes_read;
}

int main( int argc, char** argv )
{
    try
    {
        if( argc < 2 ) { usage(); }
        comma::command_line_options options( argc, argv, usage );
        comma::uint32 lines_num = options.value( "--lines,-n", 1 );
        std::string lockfile_path = options.value< std::string >( "--lock-file,--lock" );
        boost::optional< comma::uint32 > has_size = options.optional< comma::uint32 >( "--size,s" );
        boost::optional< std::string > buffer_size_string = options.optional< std::string >( "--buffer-size,-b" );
        const std::vector< std::string >& operation = options.unnamed( "--help,-h,--verbose,-v,--strict", "-.+" );
        
        strict = options.exists("--strict");
        verbose = options.value( "--verbose,-v", false );
        if( verbose ) { std::cerr << name() << ": called as: " << options.string() << std::endl; }

        bool in_operation = false;
        #ifdef WIN32
        if( has_size || operation.size() == 1 ) { _setmode( _fileno( stdout ), _O_BINARY ); }
        #endif
        if( operation.empty() ) { std::cerr << "io-buffer: please specify operation" << std::endl; return 1; }
        else if ( operation.front() == "out" ){ }
        else if ( operation.front() == "in" )
        { 
#ifdef WIN32
    std::cerr << "io-buffer: 'in' operation not implemented on windows" << std::endl; return 1;
#endif // #ifdef WIN32
            in_operation = true; 
        }
        else { std::cerr  << "unknown operation found: " << operation.front() << std::endl; return 1; }
        
        if( options.exists("--lines,-n") && options.exists("--size,-s") ) { std::cerr << name() << " option --lines(|-n) is not compatible with --size(|-s)." << std::endl; return 1; }
        
        
        bool output_last = options.exists("--no-last");
        
        comma::uint32 message_size = 0;
        comma::uint64 buffer_size = 0;
        if( has_size ) 
        { 
            if( buffer_size_string ){ buffer_size = get_buffer_size( buffer_size_string.get() ); } else { buffer_size = has_size.get(); }
            
            message_size = has_size.get();
            if( message_size == 0 ) { std::cerr << "io-buffer: message size cannot be zero" << std::endl; return 1; }
            if( message_size > buffer_size ) { std::cerr << "io-buffer: buffer size cannot be smaller than message size." << std::endl; return 1; }
            // make buffer size a multiple of message_size
            buffer_size = buffer_size - ( buffer_size % message_size );
            if( verbose) { std::cerr << "io-buffer: create binary buffer of size " << buffer_size << '(' << comma::uint32(buffer_size / message_size) << " messages)" << std::endl; }
        }
        
        // Check the lockfile can be opened and  truncated
        {
            std::ofstream  lockfile( lockfile_path.c_str(), std::ios::trunc | std::ios::out );
            if( !lockfile.is_open() ) { std::cerr << name() << "failed to open lockfile: " << lockfile_path << std::endl; return 1; }
            lockfile.close();
        }
        
        file_lock lock( lockfile_path.c_str() );  // Note lock file must exists or an exception with crytic message is thrown 
        if( operation.front() == "in" )
        {
            // Everything IO in this code block must use C function, because we turn stdin buffering off
            ::setvbuf( stdin, (char *)NULL, _IONBF, 0 ); // stdin kernel buffering is off
            
            /// Reading data from standard input is guarded by file lock
            {
                scoped_lock< file_lock > filelock(lock);
                if( has_size ) 
                {
                    memory_buffer memory(message_size);
                    while( !::feof( stdin ) && !::ferror(stdin) )
                    {
                        // Read one message
                        comma::uint32 bytes = memory.read_binary_records( message_size, 1, strict );
                        if( bytes == 0 ) { break; }
                        // Put message into buffer
                        binary_buffer->insert( binary_buffer->end(), memory.buffer, memory.buffer + memory.size );  // can't use emplace_back
                        // if buffer is filled
                        if( binary_buffer->size() >= buffer_size ) { break; }
                    }
                }
                else
                {
                    static const comma::uint32 max_size = 1024;
                    while( true )
                    {
                        memory_buffer memory( max_size );
                        std::stringstream sstream;
                        bool has_end_of_line = false;
                        
                        bool has_data = false;
                        while( !has_end_of_line && !::feof(stdin) )
                        {
                            // getline may actually changes the buffer with realloc and extend the size
                            ssize_t bytes_read = ::getline( &memory.buffer, &memory.size, stdin );
                            if ( bytes_read <= 0  ) { break; } // we are done, end of stream
                            has_data = true;
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
                        if( !has_data ) { break; } 
                        
                        lines_buffer->push_back( sstream.str() );
                        // if buffer is filled
                        if( lines_buffer->size() >= lines_num ) { break; }
                    }
                }
            }
            /// Output side is not guarded by file lock
            if( has_size && !binary_buffer->empty() )  { std::cout.write( &(binary_buffer.get()[0]), binary_buffer->size() ); return 0; }
            else
            {
                if( !lines_buffer->empty() ) {
                    for( std::size_t i=0; i<lines_buffer->size(); ++i ) { std::cout << lines_buffer.get()[i] << std::endl;  }
                    return 0;
                }
                else { exit( EX_NOINPUT ); }
            }
        }
        else
        {
            if( has_size ) 
            { 
                binary_buffer = char_buffer_t();
                binary_buffer->reserve( buffer_size );
                while( true )
                {
                    char_buffer_t message( message_size );
                    std::cin.read( &message[0], message_size );
                    if( !std::cin.good() || std::cin.eof() ) { break; }
                    binary_buffer->insert( binary_buffer->end(), message.begin(), message.end() );  // can't use emplace_back
                    // If buffer is filled
                    if( binary_buffer->size() >= buffer_size ) { output_binary(lock); binary_buffer->clear(); }
                }
                
                if( !binary_buffer->empty() ) { output_binary(lock); }
            }
            else
            {
                lines_buffer = std::vector< std::string >();
                lines_buffer->reserve( lines_num );
                while( std::cin.good() && !std::cin.eof() )
                {
                    std::string line;
                    std::getline( std::cin, line );
                    if( !line.empty() ) { lines_buffer->push_back( line ); }
                    // If buffer is filled
                    if( lines_buffer->size() >= lines_num ) { output_text(lock); lines_buffer->clear(); }
                }
                
                if( !output_last && !lines_buffer->empty() ) { output_text(lock); }
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "io-buffer: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "io-buffer: unknown exception" << std::endl; }
    return 1;
}
