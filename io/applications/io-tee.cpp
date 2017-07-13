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

#include <iostream>
#include <cstdio>   // for popen()
#include <cstdlib>
#include <string>
#include <cstring>  // for strerror()
#include <cerrno>   // for errno
#include <iostream>
#include <csignal>
#include <boost/array.hpp>
#include <boost/optional.hpp>
#include "../../application/command_line_options.h"
#include "../../io/select.h"
#include "../../io/stream.h"

static const char *app_name = "io-tee";

static void show_usage()
{
    std::cerr << "Usage: " << app_name << " <output file> [options ... --] <command ...>\n";
}

static void show_help( bool verbose = false )
{
    show_usage();
    std::cerr
        << std::endl
        << "Similar to \"tee >( command ... > file )\" in bash, but ensures that the command is complete before continuing." << std::endl
        << std::endl
        << "Options:" << std::endl
        << "    --dry-run,--dry: print command that will be piped and exit, debug option" << std::endl
        << "    --append,-a: append to output file instead of overwriting" << std::endl
        << "    --unbuffered,-u: unbuffered input and output" << std::endl
        << "    --debug: extra debug output (not for normal use)" << std::endl
        << "    --verbose,-v: more output" << std::endl
        << std::endl
        << "Note that only single commands are supported; to run multiple commands (or a pipeline), put them inside a bash function:" << std::endl
        << "*** IMPORTANT *** use \"export -f function_name\" to make the function visible to " << app_name << "." << std::endl
        << "Remember that " << app_name << " will not have access to the unexported variables, so pass any required values as function arguments." << std::endl
        << "On Ubuntu 16.04, io-tee might fail to discover a bash function even if it is exported." << std::endl
        << "In this case, comma_tee_function defined in comma-application-util should be used." << std::endl
        << std::endl
        << "If any options are used (such as --unbuffered), \"--\" must precede the command." << std::endl
        << std::endl
        << "A note about using \"grep\": be aware grep returns 1 if the pattern is not found, which will make " << app_name << " think the command failed." << std::endl
        << "To avoid this, call grep inside a function like this: grep (pattern) || true." << std::endl
        << std::endl
        << "Example 1:" << std::endl
        << "    ( echo one two ; echo three ) | io-tee outfile grep \"one two\" | grep \"three\" > outfile2" << std::endl
        << std::endl
        << "    - which is equivalent to:" << std::endl
        << std::endl
        << "    ( echo one two ; echo three ) | grep \"one two\" > outfile" << std::endl
        << "    ( echo one two ; echo three ) | grep \"three\" > outfile2" << std::endl
        << std::endl
        << "Example 2:" << std::endl
        << "    function do_something() { local pattern=$1; grep \"$pattern\"; }" << std::endl
        << "    export -f do_something" << std::endl
        << "    ( echo one two ; echo three ) | io-tee outfile do_something \"one two\" | grep \"three\" > outfile2" << std::endl
        << std::endl
        << "    - exactly the same as Example 1, but using a function (note the \"export -f\")." << std::endl
        << std::endl;
}

static bool file_is_writable( const std::string &filename, bool append )
{
    FILE *file = fopen( &filename[0], ( append ? "a" : "w" ) );
    if ( file == NULL ) { return false; }
    fclose( file );
    return true;
}

static std::string escape_quotes( const char *s )
{
    std::string result;
    for ( ; *s; ++s )
    {
        if ( *s == '"' ) { result += "\""; }          //if ( *s == '"' ) { result += "\\\""; }          // double quote becomes \"
        else if ( *s == '\'' ) { result += "'\\''"; }   // single quote becomes '\'' (since the whole command is single quoted)
        else { result += *s; }
    }
    return result;
}

int main( int ac, char **av )
{
    FILE *pipe = NULL;
    signal( SIGPIPE, SIG_IGN );  // we detect pipe errors a different way, e.g. the return value of fflush(pipe)
    try
    {
        // command line handling is tricky because we don't want to confuse the pipeline command options
        // with io-tee's options (which must all coma before "--", except for "--help|-h")
        bool debug = false;
        int dashdash_pos = -1;
        for ( int n = 1; n < ac; ++n )
        {
            if ( av[n] == std::string( "--" ) ) { dashdash_pos = n; break; }
            else if ( av[n] == std::string( "--debug" ) ) { debug = true; }
            else if ( av[n] == std::string( "--help" ) || av[n] == std::string( "-h" ) ) { show_help(); exit( 0 ); }
        }
        int command_offset = ( dashdash_pos == -1 ? 2 : dashdash_pos + 1 );
        if ( command_offset >= ac ) { std::cerr << app_name << ": missing command; "; show_usage(); exit( 1 ); }
        // if there is no "--", there can be no command line options, just the output filename
        int options_ac = ( dashdash_pos == -1 ? 2 : dashdash_pos );
        if ( debug )
        {
            std::cerr << app_name << ": options_ac=" << options_ac << "; command line: " << app_name;
            for ( int m = 1; m < ac; ++m ) { std::cerr << ' ' << av[m]; }
            std::cerr << std::endl;
        }
        comma::command_line_options options( options_ac, av );
        const std::vector< std::string >& unnamed = options.unnamed( "--unbuffered,-u,--verbose,-v,--debug,--dry-run,--dry,--append,-a", "-.*" );
        if( unnamed.empty() ) { std::cerr << app_name << ": please specify output file name" << std::endl; return 1; }
        if( unnamed.size() > 1 ) { std::cerr << app_name << ": expected one output filename, got: " << comma::join( unnamed, ' ' ) << std::endl; return 1; }
        std::string outfile = unnamed[0];
        // bash -c only takes a single argument, so put the whole command in single quotes, then double quote each individual argument
        std::string command = "bash -c '" + escape_quotes( av[command_offset] );
        for( int i = command_offset + 1; i < ac; ++i ) { command += " \""; command += escape_quotes( av[i] ); command += "\""; }
        bool append_to_outfile = options.exists( "--append,-a" );
        if( append_to_outfile ) { command += " >> "; }
        else { command += " > "; }
        command += outfile;
        command += "'";
        bool unbuffered = options.exists( "--unbuffered,-u" );
        bool verbose = options.exists( "--verbose,-v" );
        if ( debug ) { verbose = true; }
        if( !file_is_writable( outfile, append_to_outfile ) ) { std::cerr << app_name << ": cannot write to " << outfile << std::endl; exit( 1 ); }
        if( options.exists( "--dry-run,--dry" ) ) { std::cout << command << std::endl; return 0; }
        if( verbose ) { std::cerr << app_name << ": will run command: " << command << std::endl; }
        std::cout.flush();
        pipe = ::popen( &command[0], "w" );
        if( pipe == NULL ) { std::cerr << app_name << ": failed to open pipe; command: " << command << std::endl; return 1; }
        boost::array< char, 0xffff > buffer;
        if ( debug ) { std::cerr << app_name << ": created buffer" << std::endl; }
        comma::io::select stdin_select;
        if ( debug ) { std::cerr << app_name << ": constructed comma::io::select" << std::endl; }
        if( unbuffered ) { stdin_select.read().add( 0 ); if ( debug ) { std::cerr << app_name << ": did initial unbuffered read" << std::endl; } }
        comma::io::istream is( "-", comma::io::mode::binary );
        if ( debug ) { std::cerr << app_name << ": opened input stream" << std::endl; }
        if( unbuffered )
        {
            std::ios_base::sync_with_stdio( false ); // unsync to make rdbuf()->in_avail() working
            std::cin.tie( NULL ); // std::cin is tied to std::cout by default
        }
        while( std::cin.good() )
        {
            if ( debug ) { std::cerr << app_name << ": loop" << std::endl; }
            std::size_t bytes_to_read = buffer.size();
            if( unbuffered )
            {
                if ( debug ) { std::cerr << app_name << ": calling stdin_select.wait(1)" << std::endl; }
                if( stdin_select.wait( boost::posix_time::seconds( 1 ) ) == 0 ) { continue; }
                if ( debug ) { std::cerr << app_name << ": after stdin_select.wait" << std::endl; }
                std::size_t available = is.available_on_file_descriptor();
                if ( debug ) { std::cerr << app_name << ": " << available << " bytes available" << std::endl; }
                bytes_to_read = std::min( available, buffer.size() );
            }
            if ( debug ) { std::cerr << app_name << ": bytes_to_read = " << bytes_to_read << std::endl; }
            std::cin.read( &buffer[0], bytes_to_read );
            if ( debug ) { std::cerr << app_name << ": cin.gcount is " << std::cin.gcount() << std::endl; }
            if( std::cin.gcount() <= 0 ) { break; }
            std::size_t gcount = std::cin.gcount();
            if ( debug ) { std::cerr << app_name << ": writing " << gcount << " bytes to stdout" << std::endl; }
            std::cout.write( &buffer[0], gcount );
            if ( debug ) { std::cerr << app_name << ": writing " << gcount << " bytes to pipe" << std::endl; }
            int r = ::fwrite( &buffer[0], sizeof( char ), gcount, pipe );
            if ( debug ) { std::cerr << app_name << ": fwrite to pipe returned " << r << std::endl; }
            if( r != (int) gcount )
            { 
                std::cerr << app_name << ": error on pipe: " << std::strerror( errno ) <<  std::endl;
                ::pclose( pipe );
                return 1;
            }
            if( unbuffered )
            { 
                if ( debug ) { std::cerr << app_name << ": flushing stdout" << std::endl; }
                std::cout.flush();
                if ( debug ) { std::cerr << app_name << ": flushing pipe" << std::endl; }
                if ( ::fflush( pipe ) != 0 ) { std::cerr << app_name << ": flushing pipe failed: " << std::strerror( errno ) << "; command was: " << command << std::endl; ::pclose( pipe ); exit( 1 ); }
                if ( debug ) { std::cerr << app_name << ": flushed stdout and pipe " << std::endl; }
            }
        }
        std::cout.flush();
        ::fflush( pipe );
        int result = ::pclose( pipe );
        if ( result == -1 ) { std::cerr << app_name << ": pipe error: " << std::strerror( errno ) << "; command was: " << command << std::endl; exit( 1 ); }
        else if ( result != 0 ) { std::cerr << app_name << ": command failed: " << command << std::endl; return 1; }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << app_name << ": " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << app_name << ": unknown exception" << std::endl; }
    if( pipe ) { ::pclose( pipe ); }
    return 1;
}

