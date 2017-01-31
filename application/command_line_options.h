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

#ifndef COMMA_APPLICATION_COMMAND_LINE_OPTIONS_H_
#define COMMA_APPLICATION_COMMAND_LINE_OPTIONS_H_

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include "../base/exception.h"
#include "../string/string.h"
#include "verbose.h"

namespace comma {

/// a simple command line options class
class command_line_options
{
    public:
        /// constructor
        /// if --help,-h present, call usage()
        /// if --verbose,-v present, call usage( verbose )
        command_line_options( int argc, char ** argv, boost::function< void( bool ) > usage = NULL );

        /// constructor
        /// if --help,-h present, call usage()
        /// if --verbose,-v present, call usage( verbose )
        command_line_options( const std::vector< std::string >& argv, boost::function< void( bool ) > usage = NULL );
        
        /// constructor
        /// if --help,-h present, call usage()
        /// if --verbose,-v present, call usage( verbose )
        template< typename Iterator >
        command_line_options( Iterator begin, Iterator end, boost::function< void( bool ) > usage = NULL );

        /// constructor
        command_line_options( const command_line_options& rhs );

        /// return argv
        const std::vector< std::string >& argv() const;

        /// print all command line arguments, quoting all arguments and escaping them
        /// to make it transparently working in bash
        std::string string() const;

        /// escape double quotes
        /// to make it transparently working in bash
        static std::string escaped( const std::string& s );

        /// return true, if option exists (list, e.g.: "--binary,-b" is allowed)
        bool exists( const std::string& name ) const;

        /// return option value; throw, if it does not exist (list, e.g.: "--binary,-b" is allowed)
        template < typename T >
        T value( const std::string& name ) const;

        /// return optional option value (convenience method)
        template < typename T >
        boost::optional< T > optional( const std::string& name ) const;

        /// return option value; default, if option not specified (list, e.g.: "--binary,-b" is allowed)
        template < typename T >
        T value( const std::string& name, T default_value ) const;

        /// return option values
        template < typename T >
        std::vector< T > values( const std::string& name ) const;

        /// return option values; one default value, if option not specified
        template < typename T >
        std::vector< T > values( const std::string& name, T default_value ) const;

        /// return option names
        std::vector< std::string > names() const;

        /// return free-standing values; arguments are comma-separated lists of options
        /// @param valueless_options options that do not take any value
        /// @param options_with_values options that take a value
        /// both parameters can be posix regular expressions
        /// in this case, if command line parameter is found amoung valueless options
        /// it will not be considered as having no value
        /// example:
        ///    options.unnamed( "--verbose,--quiet", "-.*,--.*" );
        ///    here:
        ///        --verbose and --quiet
        ///        any other command line parameters starting with - or -- are considered options with values
        std::vector< std::string > unnamed( const std::string& valueless_options, const std::string& options_with_values = "-.*" ) const;
        
        /// throw, if more than one of given options exists
        void assert_mutually_exclusive( const std::string& comma_separated_names ) const;
        
        /// throw, if at least one option from each set is present
        void assert_mutually_exclusive( const std::string& first, const std::string& second ) const;

        /// description
        struct description
        {
            std::vector< std::string > names;
            bool is_optional;
            bool has_value;
            boost::optional< std::string > default_value; // todo: make strongly typed
            std::string help;

            /// default constructor
            description() : is_optional( false ), has_value( false ) {}

            /// if invalid, throw a meaningful exception
            void assert_valid( const command_line_options& options ) const;

            /// return true, if this option is correctly represented in options
            bool valid( const command_line_options& options ) const throw();

            /// construct from string
            static description from_string( const std::string& s );

            /// construct from string
            std::string as_string() const;

            /// return usage
            static std::string usage();
        };

        /// if invalid, throw a meaningful exception
        void assert_valid( const std::vector< description >& d, bool unknown_options_invalid = false );

    private:
        typedef std::map< std::string, std::vector< std::string > > map_type_;

        void fill_map_( const std::vector< std::string >& v );
        template < typename T > static T lexical_cast_( const std::string& s );

        std::vector< std::string > argv_;
        map_type_ map_;
        std::vector< std::string > names_;
};

template< typename Iterator > inline command_line_options::command_line_options( Iterator begin, Iterator end, boost::function< void( bool ) > usage )
{
    argv_.resize( std::distance( begin, end ) );
    for ( Iterator i = begin; i < end; ++i ) { argv_[i] = *i; }
    fill_map_( argv_ );
    if ( usage && exists( "--help,-h" ) )
    {
        bool v = exists( "--verbose,-v" );
        comma::verbose.init( v, *begin );
        usage( v ); exit( 1 );
    }
}

template < typename T > inline T command_line_options::lexical_cast_( const std::string& s ) { return boost::lexical_cast< T >( s ); }

template <> inline bool command_line_options::lexical_cast_< bool >( const std::string& s )
{
    if( s == "true" ) { return true; }
    if( s == "false" ) { return false; }
    return boost::lexical_cast< bool >( s );
}

template < typename T >
inline std::vector< T > command_line_options::values( const std::string& name ) const
{
    std::vector< T > v;
    std::vector< std::string > names = comma::split( name, ',' );
    for( std::size_t i = 0; i < names.size(); ++i )
    {
        typename map_type_::const_iterator it = map_.find( names[i] );
        if( it == map_.end() ) { continue; }
        for( std::size_t j = 0; j < it->second.size(); ++j ) { v.push_back( lexical_cast_< T >( it->second[j] ) ); }
    }
    return v;
}

template < typename T >
inline boost::optional< T > command_line_options::optional( const std::string& name ) const
{
    std::vector< T > v = values< T >( name );
    return v.empty() ? boost::optional< T >() : boost::optional< T >( v[0] );
}

template < typename T >
inline T command_line_options::value( const std::string& name ) const
{
    std::vector< T > v = values< T >( name );
    if( v.empty() ) { COMMA_THROW( comma::exception, "option \"" << name << "\" not specified" ); }
    return v[0];
}

template < typename T >
inline T command_line_options::value( const std::string& name, T default_value ) const
{
    std::vector< T > v = values< T >( name, default_value );
    return v[0];
}

template < typename T >
inline std::vector< T > command_line_options::values( const std::string& name, T default_value ) const
{
    std::vector< T > v = values< T >( name );
    if( v.empty() ) { v.push_back( default_value ); }
    return v;
}

} // namespace comma {

#endif // COMMA_APPLICATION_COMMAND_LINE_OPTIONS_H_
