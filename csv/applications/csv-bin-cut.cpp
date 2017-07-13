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


/// @author dmitry mikhin

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <stdlib.h>
#include <fstream>
#include <numeric>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../csv/format.h"
#include "../../csv/options.h"
#include "../../string/string.h"

using namespace comma;

// todo
// - help
//   - move to examples or verbose
//     - --output-fields description
//     - lengthy implementation details -> examples or verbose
//     - Semantics for input
//     - Semantics for output
//     - '-' + files: add an example

namespace {

    void brief_examples( )
    {
        std::cerr << "Brief examples (run --help --verbose for more):" << std::endl;
        std::cerr << "    csv-bin-cut input.bin --binary=t,s[1000000] --fields=t,s --output-fields=t" << std::endl;
        std::cerr << "    csv-bin-cut input.bin --binary=t,s[1000000] --fields=1" << std::endl;
        std::cerr << "    cat input.bin | csv-bin-cut --binary=t,s[1000000] --fields=t, --output-fields=t" << std::endl;
        std::cerr << "    cat input.bin | csv-bin-cut t,s[1000000] --fields=1" << std::endl;
        std::cerr << "all versions output the same field 't'; the first two variants are faster" << std::endl;
        std::cerr << std::endl;
    }

    void usage( bool verbose )
    {
        std::cerr << std::endl;
        std::cerr << "Do to fixed-size binary records same as linux utility cut (1) for csv data" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << "    1) faster version reading from files" << ( verbose ? "" : "; run --help --verbose for details" ) << std::endl;
        std::cerr << "       csv-bin-cut file.bin [file2.bin ...] [<options>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "    2) slower version reading from stdin" << ( verbose ? "" : "; run --help --verbose for details" ) << std::endl;
        std::cerr << "       cat file.bin [file2.bin ...] | csv-bin-cut [<options>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "    --help,-h: help; --help --verbose: more help" << std::endl;
        std::cerr << "    --fields,-f=<fields>: input field names if '--output-fields' are given; otherwise output field numbers starting from 1" << std::endl;
        std::cerr << "    --binary,-b=<format>: input binary format" << std::endl;
        std::cerr << "    --output-fields,--output,-o=[<fields>]: output fields" << std::endl;
        std::cerr << "    --skip=[<N>]; skip the first N records (applied once if multiple input files are given); default: 0" << std::endl;
        std::cerr << "    --count=[<N>]; output no more than N records" << std::endl;
        std::cerr << "    --read-all,--force-read; do not use the seek form of the algorithm, read entire records at once; " << ( verbose ? "see below" : "run --help --verbose for details" ) << std::endl;
        std::cerr << "    --flush; flush after every output record; less efficient, use it if you need to process records in real time to avoid buffering" << std::endl;
        std::cerr << "    --verbose,-v: chat more" << std::endl;
        std::cerr << std::endl;
        if ( !verbose ) {
            brief_examples();
        } else {
            std::cerr << "Semantics for input:" << std::endl;
            std::cerr << "    csv-bin-cut a.bin b.bin --binary=<format> ...: read data from files, potentially fast" << std::endl;
            std::cerr << "    csv-bin-cut --binary=<format> ...: read data from stdin" << std::endl;
            std::cerr << "    csv-bin-cut - --binary=<format> ...: also read data from stdin" << std::endl;
            std::cerr << "    csv-bin-cut a.bin - b.bin --binary=<format> ...: read data from a.bin (fast), then read data from stdin (full records," << std::endl;
            std::cerr << "        may be slow), then read data from b.bin (again, fast)" << std::endl;
            std::cerr << "    csv-bin-cut <format> ...: read data from stdin, specify format as the first argument (deprecated, left for backward compatibility)" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Semantics for output:" << std::endl;
            std::cerr << "    csv-bin-cut ... --fields=foo,bar,baz --output-fields=foo,baz: input and output fields specified by names" << std::endl;
            std::cerr << "    csv-bin-cut ... --fields=1,3: fields to output are specified by numbers as in cut (1) utility; numbers start with 1" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Algorithm:" << std::endl;
            std::cerr << "    When csv-bin-cut reads a file (as opposed to stdin), it seeks to the position of the next data to be output as opposed to" << std::endl;
            std::cerr << "    reading entire records; this improves performance for large record sizes as most of the input is skipped. For small records," << std::endl;
            std::cerr << "    however, the improvement is lost because seek-read-seek-read would repeatedly read the same disk block. Therefore, for small" << std::endl;
            std::cerr << "    records it is advised to turn off the seeking algorithm by the '--read-all' option." << std::endl;
            std::cerr << std::endl;
            std::cerr << "Examples:" << std::endl;
            std::cerr << "    csv-bin-cut input.bin --binary=t,s[1000000] --fields=t,s --output-fields=t" << std::endl;
            std::cerr << "        output only the time field from a binary file; more efficient form of" << std::endl;
            std::cerr << "            cat input.bin | csv-shuffle --binary=t,s[1000000] --fields=t,s --output-fields=t" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    csv-bin-cut input.bin --binary=t,s[1000000] --fields=1" << std::endl;
            std::cerr << "        same but uses positional number to specify output of the 't' field" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    cat input.bin | csv-bin-cut --binary=t,s[1000000] --fields=1" << std::endl;
            std::cerr << "        take input on stdin; same output as above but less efficient because has to read the whole file" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    csv-bin-cut input.bin --binary=t,s[2000] --fields=1 --read-all" << std::endl;
            std::cerr << "        record size is small, force reading the entire record at once; thus, identical to" << std::endl;
            std::cerr << "            cat input.bin | csv-shuffle --binary=t,s[2000] --fields=t,s --output-fields=t" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    csv-bin-cut input.bin --skip=1000 --binary=t,s[1000000] --fields=t,s --output-fields=t | csv-from-bin t" << std::endl;
            std::cerr << "        output only the time field starting with 1000th record; more efficient form of" << std::endl;
            std::cerr << "            cat input.bin | csv-shuffle --binary=t,s[1000000] --fields=t,s --output-fields=t | csv-from-bin t | tail -n+1001" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    csv-bin-cut 1.bin 2.bin 3.bin --skip=1000 --count=2 --binary=t,s[1000000] --fields=t,s ..." << std::endl;
            std::cerr << "        assuming each of the binary files holds 500 records, output data from" << std::endl;
            std::cerr << "        records 0 and 1 of file '3.bin'; records 0-499 in the file '1.bin' skipped, records 0-499 in" << std::endl;
            std::cerr << "        the file '2.bin' skipped (total of 1000 skipped records), then 2 records output" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    cat 2.bin | csv-bin-cut 1.bin - 3.bin --binary=t,s[1000000] --fields=1" << std::endl;
            std::cerr << "        read file 1.bin (fast), then stdin (file 2.bin; slower), then file 3.bin (again, fast)" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    cat 2.bin | csv-bin-cut t,s[1000000] --fields=1" << std::endl;
            std::cerr << "        specify format as command-line argument; supported for backward compatibility; no further file arguments" << std::endl;
            std::cerr << "        are taken, mandatory read stdin" << std::endl;
            std::cerr << std::endl;
            std::cerr << "    echo 1,2,3,4,5,6,7 | csv-to-bin 2ub,3uw,2ui | csv-bin-cut --fields=1-3,5,6-7 --binary=2ub,3uw,2ui | csv-from-bin 2ub,2uw,2ui" << std::endl;
            std::cerr << "        numeric fields can be specified as ranges (inclusive)" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Format specifications:" << std::endl;
            std::cerr << csv::format::usage() << std::endl;
        }
        std::cerr << comma::contact_info << std::endl;
        std::cerr << std::endl;
        exit( 0 );
    }

    struct field
    {
        std::string name;
        unsigned int index;
        unsigned int offset;
        boost::optional< unsigned int > input_index;
        unsigned int input_offset;
        unsigned int size;
        field( const std::string& name, unsigned int index ) : name( name ), index( index ) {}
    };

    std::vector< field > setup_fields( const comma::command_line_options & options, const comma::csv::options & csv )
    {
        std::vector< std::string > input_fields = comma::split( csv.fields, ',' );
        std::vector< field > fields;
        if ( options.exists( "--output-fields,--output,-o" ) )
        {
            // input and output fields must be names
            std::vector< std::string > output_fields = comma::split( options.value< std::string >( "--output-fields,--output,-o" ), ',' );
            for( unsigned int i = 0; i < output_fields.size(); ++i )
            {
                if( output_fields[i].empty() ) { continue; }
                fields.push_back( field( output_fields[i], i ) );
            }
            for( unsigned int i = 0; i < input_fields.size(); ++i )
            {
                for( unsigned int j = 0; j < fields.size(); ++j )
                {
                    if( fields[j].name != input_fields[i] ) { continue; }
                    fields[j].input_index = i;
                    fields[j].input_offset = csv.format().offset( i ).offset;
                    fields[j].size = csv.format().offset( i ).size;
                }
            }
        } else {
            // input fields must be numbers or ranges of numbers
            for( unsigned int i = 0; i < input_fields.size(); ++i )
            {
                if ( input_fields[i].empty() ) { continue; }
                std::vector< std::string > r = comma::split( input_fields[i], '-' );
                if( r.size() == 2 )
                {
                    comma::int32 begin = boost::lexical_cast< comma::int32 >( r[0] );
                    if( begin <= 0 ) { std::cerr << "csv-bin-cut: field numbers start with 1 (to keep it consistent with linux cut utility)" << std::endl; exit( 1 ); }
                    comma::int32 end = boost::lexical_cast< comma::int32 >( r[1] );
                    if( end <= begin ) { std::cerr << "csv-bin-cut: expected range, got: " << input_fields[i] << std::endl; exit( 1 ); }
                    ++end;
                    for( int k = begin; k < end; ++k ) {
                        int pos = k;
                        fields.push_back( field( boost::lexical_cast< std::string >( pos ), fields.size() ) );
                        fields.back().input_index = --pos;
                        fields.back().input_offset = csv.format().offset( pos ).offset;
                        fields.back().size = csv.format().offset( pos ).size;
                    }
                }
                else
                {
                    int pos = boost::lexical_cast< int >( input_fields[i] );
                    if ( pos < 1 || pos > int( csv.format().count() ) ) { std::cerr << "csv-bin-cut: field number " << input_fields[i] << " out of [1," << csv.format().count() << "] bounds" << std::endl; exit( 1 ); }
                    fields.push_back( field( boost::lexical_cast< std::string >( pos ), fields.size() ) );
                    fields.back().input_index = --pos;
                    fields.back().input_offset = csv.format().offset( pos ).offset;
                    fields.back().size = csv.format().offset( pos ).size;
                }
            }
        }
        if( fields.empty() ) { std::cerr << "csv-bin-cut: please define at least one output field" << std::endl; exit( 1 ); }
        for( unsigned int i = 0; i < fields.size(); ++i )
        {
            if( !fields[i].input_index ) { std::cerr << "csv-bin-cut: field '" << fields[i].name << "' not found in input fields '" << csv.fields << "'" << std::endl; exit( 1 ); }
            fields[i].offset = ( i == 0 ? 0 : fields[i - 1].offset + fields[i - 1].size );
        }
        return fields;
    }

    class seeker
    {
        public:
            seeker( const std::vector< field > & fields, const comma::csv::options & csv, unsigned int skip, long int count_max, bool flush, bool force_read )
                : fields_( fields )
                , orecord_size_( std::accumulate( fields.begin(), fields.end(), (size_t)0, seeker::add_size ) )
                , obuf_( orecord_size_ )
                , ibuf_( 0 )
                , irecord_size_( csv.format().size() )
                , skip_( skip )
                , count_( 0 )
                , count_max_( count_max )
                , flush_( flush )
                , force_read_( force_read )
                {}

            int process( const std::vector< std::string > & files );

        private:
            int read_fields( std::ifstream & ifs, const std::string & fname );
            int read_all( std::istream & is );

            const std::vector< field > & fields_;
            size_t orecord_size_;
            std::vector< char > obuf_;
            std::vector< char > ibuf_;
            size_t irecord_size_;
            unsigned int skip_;
            long int count_;
            long int count_max_;
            bool flush_;
            bool force_read_;

            static size_t add_size( size_t i, const field & f ){ return i + f.size; }
    };

    int seeker::read_all( std::istream & is )
    {
        if ( count_max_ >= 0 && count_ >= count_max_ ) { return 0; }
        ibuf_.resize( irecord_size_ );
        while( is.good() && !is.eof() )
        {
            is.read( &ibuf_[0], irecord_size_ );
            if( is.gcount() == 0 ) { continue; }
            if( is.gcount() < int( irecord_size_ ) ) { std::cerr << "csv-bin-cut: expected " << irecord_size_ << " bytes, got only " << is.gcount() << std::endl; exit( 1 ); }
            if ( skip_ ) { --skip_; continue; }
            for( unsigned int i = 0; i < fields_.size(); ++i ) { std::cout.write( &ibuf_[0] + fields_[i].input_offset, fields_[i].size ); }
            if( flush_ ) { std::cout.flush(); }
            if ( count_max_ >= 0 && ++count_ >= count_max_ ) { return 0; }
        }
        return 0;
    }

    int seeker::read_fields( std::ifstream & ifs, const std::string & fname )
    {
        // algorithm summary:
        // - seekg to the beginning of record
        // - iterate over fields by doing seekg to the beginning of each field, and read into buffer
        // - check for read errors / end of input
        // - write the buffer to stdout
        std::streampos record_start = 0;
        if ( skip_ ) {
            ifs.seekg( 0, std::ios_base::end );
            std::streampos fsize = ifs.tellg();
            unsigned int nrecords = fsize / irecord_size_;
            if ( nrecords * irecord_size_ != static_cast< unsigned int >( fsize ) ) { std::cerr << "csv-bin-cut: size of file '" << fname << "' is not a multiple of the record size" << std::endl; exit( 1 ); }
            if ( nrecords < skip_ ) { skip_ -= nrecords; return 0; }
            record_start = irecord_size_ * skip_;
            ifs.seekg( record_start, std::ios_base::beg );
            if ( ifs.fail() ) { std::cerr << "csv-bin-cut: cannot skip " << skip_ << " records in the file '" << fname << "'" << std::endl; exit( 1 ); }
            skip_ = 0;
        }
        while( ifs.good() && !ifs.eof() )
        {
            for( unsigned int i = 0; i < fields_.size(); ++i )
            {
                std::streamoff off = fields_[i].input_offset - ( i == 0 ? 0 : fields_[i - 1].input_offset + fields_[i - 1].size );
                ifs.seekg( off, std::ios_base::cur );
                ifs.read( &obuf_[ fields_[i].offset ], fields_[i].size );
                if ( ifs.eof() ) {
                    if ( i == 0 ) { break; }
                    std::cerr << "csv-bin-cut: encountered eof mid-record in '" << fname << "'" << std::endl; exit( 1 );
                }
                if ( ifs.fail() ) { std::cerr << "csv-bin-cut: reading field '" << fields_[i].name << "' at position " << record_start + off << " failed" << std::endl; exit( 1 ); }
            }
            if ( !ifs.eof() )
            {
                std::cout.write( &obuf_[0], orecord_size_ );
                if ( flush_ ) { std::cout.flush(); }
                if ( std::cout.fail() ) { std::cerr << "csv-bin-cut: std::cout output failed" << std::endl; exit( 1 ); }
                if ( count_max_ >= 0 && ++count_ >= count_max_ ) { return 0; }  // count not incremented if no limit imposed, do not care
                // go to the start of the next record
                record_start += irecord_size_;
                unsigned int i = fields_.size() - 1;
                std::streamoff off = irecord_size_ - ( fields_[i].input_offset + fields_[i].size );
                ifs.seekg( off, std::ios_base::cur );
            }
        }
        return 0;
    }

    int seeker::process( const std::vector< std::string > & files )
    {
        if ( files.empty() ) { return read_all( std::cin ); }
        for ( std::vector< std::string >::const_iterator ifile = files.begin(); ifile < files.end(); ++ifile ) {
            if ( count_max_ >= 0 && count_ >= count_max_ ) { return 0; }
            if ( *ifile == "-" ) {
                int rv = read_all( std::cin );
                if ( rv != 0 ) { return rv; }
            } else {
                std::ifstream ifs( &( *ifile )[0], std::ifstream::binary );
                if ( !ifs.is_open() ) { std::cerr << "csv-bin-cut: cannot open '" << *ifile << "' for reading" << std::endl; exit( 1 ); }
                int rv = ( force_read_ ? read_all( ifs ) : read_fields( ifs, *ifile ) );
                if ( rv != 0 ) { return rv; }
            }
        }
        return 0;
    }

} // anonymous

int main( int ac, char** av )
{
    #ifdef WIN32
    _setmode( _fileno( stdin ), _O_BINARY ); /// @todo move to a library
    _setmode( _fileno( stdout ), _O_BINARY );
    #endif
    try
    {
        command_line_options options( ac, av, usage );
        comma::csv::options csv( options );
        std::vector< std::string > files = options.unnamed( "--help,-h,--verbose,-v,--flush,--read-all,--force-read", "--fields,-f,--output-fields,-o,--binary,-b,--skip,--count" );
        if( !csv.binary() )
        {
            if( files.size() == 1 && files[0] != "-" ) // deprecated, left for backward compatibility
            {
                try
                {
                    csv.format( comma::csv::format( files[0] ) );
                    files.clear();
                }
                catch ( comma::exception & )
                {
                    // it's not a format string
                }
            }
        }
        if( !csv.binary() ) { std::cerr << "csv-bin-cut: please specify --binary" << std::endl; exit( 1 ); }

        const std::vector< field >& fields = setup_fields( options, csv );

        unsigned int skip = options.value< unsigned int >( "--skip", 0 );
        long int count_max = options.value< long int >( "--count", -1 );
        bool flush = options.exists( "--flush" );
        bool force_read = options.exists( "--read-all,--force-read" );

        seeker seek( fields, csv, skip, count_max, flush, force_read );
        return seek.process( files );
    }
    catch( std::exception& ex ) { std::cerr << "csv-bin-cut: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-bin-cut: unknown exception" << std::endl; }
    return 1;
}
