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

#ifndef COMMA_CSV_BINARY_HEADER_GUARD_
#define COMMA_CSV_BINARY_HEADER_GUARD_

#include <boost/optional.hpp>
#include "../string/string.h"
#include "names.h"
#include "options.h"
#include "impl/binary_visitor.h"
#include "impl/from_binary.h"
#include "impl/to_binary.h"

namespace comma { namespace csv {

template < typename S >
class binary
{
    public:
        /// constructor
        binary( const std::string& f = "", const std::string& column_names = "", bool full_path_as_name = true, const S& sample = S() );

        /// constructor from options
        binary( const options& o, const S& sample = S() );

        /// get value (returns reference pointing to the parameter)
        const S& get( S& s, const char* buf ) const;

        /// put value at the right place in the buffer
        char* put( const S& s, char* buf ) const;

        /// allocate buffer and put value in it (convenience function)
        std::vector< char > put( const S& s ) const;

        /// return format
        const csv::format& format() const { return format_; }

    private:
        csv::format format_;
        boost::optional< impl::binary_visitor > binary_;
};

template < typename S >
inline binary< S >::binary( const std::string& f, const std::string& column_names, bool full_path_as_name, const S& sample )
    : format_( f == "" ? csv::format::value( sample ) : f )
{
    if( format_.size() == sizeof( S ) && format_.string() == csv::format::value( sample ) && join( csv::names( column_names, full_path_as_name, sample ), ',' ) == join( csv::names( full_path_as_name ), ',' ) ) { return; }
    binary_ = impl::binary_visitor( format_, join( csv::names( column_names, full_path_as_name, sample ), ',' ), full_path_as_name );
    visiting::apply( *binary_, sample );
    //if( binary_ && binary_->offsets().size() == 0 ) { COMMA_THROW( comma::exception, "expected at least one field of \"" << comma::join( csv::names< S >( full_path_as_name ), ',' ) << "\"; got \"" << column_names << "\"" ); }
}

template < typename S >
inline binary< S >::binary( const options& o, const S& sample )
    : format_( o.format().string() == "" ? csv::format::value( sample ) : o.format().string() )
{
    if( format_.size() == sizeof( S ) && format_.string() == csv::format::value( sample ) && join( csv::names( o.fields, o.full_xpath, sample ), ',' ) == join( csv::names( o.full_xpath ), ',' ) ) { return; }
    binary_ = impl::binary_visitor( format_, join( csv::names( o.fields, o.full_xpath, sample ), ',' ), o.full_xpath );
    visiting::apply( *binary_, sample );
    //if( binary_ && binary_->offsets().size() == 0 ) { COMMA_THROW( comma::exception, "expected at least one field of \"" << comma::join( csv::names< S >( o.full_xpath ), ',' ) << "\"; got \"" << o.fields << "\"" ); }
}

template < typename S >
inline const S& binary< S >::get( S& s, const char* buf ) const
{
    if( binary_ )
    {
        impl::from_binary_ f( binary_->offsets(), binary_->optional(), buf );
        visiting::apply( f, s );
    }
    else // quick and dirty for better performance
    {
        ::memcpy( reinterpret_cast< char* >( &s ), buf, sizeof( S ) );
    }
    return s;
}

template < typename S >
inline char* binary< S >::put( const S& s, char* buf ) const
{
    if( binary_ )
    {
        impl::to_binary f( binary_->offsets(), buf );
        visiting::apply( f, s );
    }
    else // quick and dirty for better performance
    {
        ::memcpy( buf, reinterpret_cast< const char* >( &s ), sizeof( S ) );
    }
    return buf;
}

template < typename S >
inline std::vector< char > binary< S >::put( const S& s ) const
{
    std::vector< char > buf( format_.size() );
    put( s, &buf[0] );
    return buf;
}

} } // namespace comma { namespace csv {

#endif // #ifndef COMMA_CSV_BINARY_HEADER_GUARD_
