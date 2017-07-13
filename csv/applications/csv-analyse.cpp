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

/// @author james underwood

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <iostream>
#include <map>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"

using namespace comma;

class histogram
{
public:
    histogram() : last_seen_index_(256,0), seen_(256,false) {}

    void observe( unsigned char value, std::size_t location )
    {
        if( seen_[value] )
        {
            //assert( (i-last_seen_index[data[i]]) > 0 && (i-last_seen_index[data[i]]) < bytes_read );
            assert( location > last_seen_index_[value] );
            ++histogram_[ location - last_seen_index_[value] ];
        }
        last_seen_index_[value] = location;
        seen_[value] = true;
    }

    std::ostream& print_sorted(std::ostream& os) const
    {
        //sort, ugly
        std::multimap< std::size_t, std::size_t > sorted;
        std::size_t sum=0;
        
        for(std::map< std::size_t, std::size_t >::const_iterator it=histogram_.begin(), end=histogram_.end(); it!=end; ++it )
        {
            sorted.insert( std::make_pair(it->second,it->first) );
            sum += it->second;
        }
        
        for(std::multimap< std::size_t, std::size_t >::const_reverse_iterator it=sorted.rbegin(), end=sorted.rend(); it!=end; ++it )
        {
            os << it->second << "," << it->first << "," << (double)((double)(it->first)/(double)sum) << std::endl;
        }
    
        return os;
    }

private:
    std::vector< std::size_t > last_seen_index_;
    std::vector< bool > seen_;
    std::map< std::size_t, std::size_t > histogram_; //length, count
};

std::ostream& operator<<(std::ostream& os, const histogram & h)
{
    return h.print_sorted(os);
}

static void usage()
{
    std::cerr << std::endl;
    std::cerr << "Analyse binary data to guess message lengths in unknown binary stream: output candidate lengths, repeat counts and normalised probabilities" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: cat file.bin | csv-analyse" << std::endl;
    std::cerr << std::endl;
    std::cerr << "e.g. use the entire file.bin for calculation and display the five most likely binary message lengths" << std::endl;
    std::cerr << std::endl;
    std::cerr << "     cat file.bin | csv-analyse | head -n 5 " << std::endl;
    std::cerr << std::endl;
    std::cerr << "e.g  use the first 100 bytes of file.bin for calculation and display all message length candidates" << std::endl;
    std::cerr << std::endl;
    std::cerr << "     cat file.bin | head --bytes=100 | csv-analyse" << std::endl;
    std::cerr << std::endl;
    std::cerr << "note: algorithm is most efficient for relatively small message size (<<1MB), because large messages tend to contain all byte values within each message" << std::endl;
    std::cerr << "      to tease large message sizes, provide alot of data and filter results" << std::endl;
    std::cerr << std::endl;
    std::cerr << "e.g.  use 20MB of file.bin (hopefully that contains multiple messages) and filter only the top ten candidates with message length > 100K" << std::endl;
    std::cerr << std::endl;
    std::cerr << "      cat large.bin | head --bytes=20000000 | csv-analyse | csv-select --fields=length,, \"length;from=100000\" | head -n 10" << std::endl;
    std::cerr << std::endl;
    std::cerr << "To test a length hypothesis, perhaps there is a timestamp (8 bytes) in first position?" << std::endl;
    std::cerr << std::endl;
    std::cerr << "e.g. for a message length candidate of 1234567890, subtracting 8 for time leaves 123456782 bytes of unknown data, so try:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "       cat file.bin | csv-bin-cut t,123456782ub --fields=1 | csv-from-bin t | head -n 5" << std::endl;
    std::cerr << std::endl;
    std::cerr << "       if the formatting is correct, then all 5 timestamps should appear valid and in succession" << std::endl;
    std::cerr << std::endl;
    std::cerr << "See also: \"csv-size\", \"csv-bin-cut\"" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( -1 );
}

int main( int ac, char** av )
{
    try
    {
        #ifdef WIN32
            _setmode( _fileno( stdin ), _O_BINARY );
        #endif

        command_line_options options( ac, av );
        if( ac > 1 || options.exists( "--help" ) || options.exists( "-h" ) ) { usage(); } //could just say ac > 1... but leave for future args

        histogram h;

        const std::size_t read_size=65535; //todo: better way?
        std::vector< unsigned char > data( read_size );
        std::size_t offset=0;

        //read as many bytes as available on stdin
        while( std::cin.good() && !std::cin.eof() )
        {
            int bytes_read = ::read( 0, &data[0], read_size );
            if( bytes_read <= 0 ) { break; }
                        
            for( int i=0; i<bytes_read; ++i )
            {
                h.observe(data[i],offset+i);
            }
            offset+=bytes_read;
        }
       
        std::cout << h;
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-analyse: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-analyse: unknown exception" << std::endl; }
    usage();
}
