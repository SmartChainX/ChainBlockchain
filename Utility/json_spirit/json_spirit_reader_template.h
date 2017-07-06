include "json_spirit_value.h"

#include "json_spirit_error_position.h"



//#define BOOST_SPIRIT_THREADSAFE  // uncomment for multithreaded use, requires linking to boost.thread



#include <boost/bind.hpp>

#include <boost/function.hpp>

#include <boost/version.hpp>



#if BOOST_VERSION >= 103800

    #include <boost/spirit/include/classic_core.hpp>

    #include <boost/spirit/include/classic_confix.hpp>

    #include <boost/spirit/include/classic_escape_char.hpp>

    #include <boost/spirit/include/classic_multi_pass.hpp>

    #include <boost/spirit/include/classic_position_iterator.hpp>

    #define spirit_namespace boost::spirit::classic

#else

    #include <boost/spirit/core.hpp>

    #include <boost/spirit/utility/confix.hpp>

    #include <boost/spirit/utility/escape_char.hpp>

    #include <boost/spirit/iterator/multi_pass.hpp>

    #include <boost/spirit/iterator/position_iterator.hpp>

    #define spirit_namespace boost::spirit

#endif



namespace json_spirit

{

    const spirit_namespace::int_parser < boost::int64_t >  int64_p  = spirit_namespace::int_parser < boost::int64_t  >();

    const spirit_namespace::uint_parser< boost::uint64_t > uint64_p = spirit_namespace::uint_parser< boost::uint64_t >();



    template< class Iter_type >

    bool is_eq( Iter_type first, Iter_type last, const char* c_str )

    {

        for( Iter_type i = first; i != last; ++i, ++c_str )

        {

            if( *c_str == 0 ) return false;



            if( *i != *c_str ) return false;

        }



        return true;

    }



    template< class Char_type >

    Char_type hex_to_num( const Char_type c )

    {

        if( ( c >= '0' ) && ( c <= '9' ) ) return c - '0';

        if( ( c >= 'a' ) && ( c <= 'f' ) ) return c - 'a' + 10;

        if( ( c >= 'A' ) && ( c <= 'F' ) ) return c - 'A' + 10;

        return 0;

    }



    template< class Char_type, class Iter_type >

    Char_type hex_str_to_char( Iter_type& begin )

    {

        const Char_type c1( *( ++begin ) );

        const Char_type c2( *( ++begin ) );



        return ( hex_to_num( c1 ) << 4 ) + hex_to_num( c2 );

    }       



    template< class Char_type, class Iter_type >

    Char_type unicode_str_to_char( Iter_type& begin )

    {

        const Char_type c1( *( ++begin ) );

        const Char_type c2( *( ++begin ) );

        const Char_type c3( *( ++begin ) );

        const Char_type c4( *( ++begin ) );



        return ( hex_to_num( c1 ) << 12 ) + 

               ( hex_to_num( c2 ) <<  8 ) + 

               ( hex_to_num( c3 ) <<  4 ) + 

               hex_to_num( c4 );

    }



    template< class String_type >

    void append_esc_char_and_incr_iter( String_type& s, 

                                        typename String_type::const_iterator& begin, 

                                        typename String_type::const_iterator end )

    {

        typedef typename String_type::value_type Char_type;

             

        const Char_type c2( *begin );



        switch( c2 )

        {

            case 't':  s += '\t'; break;

            case 'b':  s += '\b'; break;

            case 'f':  s += '\f'; break;

            case 'n':  s += '\n'; break;

            case 'r':  s += '\r'; break;

            case '\\': s += '\\'; break;

            case '/':  s += '/';  break;

            case '"':  s += '"';  break;

            case 'x':  

            {

                if( end - begin >= 3 )  //  expecting "xHH..."

                {

                    s += hex_str_to_char< Char_type >( begin );  

                }

                break;

            }

            case 'u':  

            {

                if( end - begin >= 5 )  //  expecting "uHHHH..."

                {

                    s += unicode_str_to_char< Char_type >( begin );  

                }

                break;

            }

        }

    }



    template< class String_type >

    String_type substitute_esc_chars( typename String_type::const_iterator begin, 

                                   typename String_type::const_iterator end )

    {

        typedef typename String_type::const_iterator Iter_type;



        if( end - begin < 2 ) return String_type( begin, end );



        String_type result;

        

        result.reserve( end - begin );



        const Iter_type end_minus_1( end - 1 );



        Iter_type substr_start = begin;

        Iter_type i = begin;



        for( ; i < end_minus_1; ++i )

        {

            if( *i == '\\' )

            {

                result.append( substr_start, i );



                ++i;  // skip the '\'

             

                append_esc_char_and_incr_iter( result, i, end );



                substr_start = i + 1;

            }

        }



        result.append( substr_start, end );



        return result;

    }



    template< class String_type >

    String_type get_str_( typename String_type::const_iterator begin, 

                       typename String_type::const_iterator end )

    {

        assert( end - begin >= 2 );



        typedef typename String_type::const_iterator Iter_type;



        Iter_type str_without_quotes( ++begin );

        Iter_type end_without_quotes( --end );



        return substitute_esc_chars< String_type >( str_without_quotes, end_without_quotes );

    }



    inline std::string get_str( std::string::const_iterator begin, std::string::const_iterator end )

    {

        return get_str_< std::string >( begin, end );

    }



    inline std::wstring get_str( std::wstring::const_iterator begin, std::wstring::const_iterator end )

    {

        return get_str_< std::wstring >( begin, end );

    }

    

    template< class String_type, class Iter_type >

    String_type get_str( Iter_type begin, Iter_type end )

    {

        const String_type tmp( begin, end );  // convert multipass iterators to string iterators



        return get_str( tmp.begin(), tmp.end() );

    }



    // this class's methods get called by the spirit parse resulting

    // in the creation of a JSON object or array

    //

    // NB Iter_type could be a std::string iterator, wstring iterator, a position iterator or a multipass iterator

    //

    template< class Value_type, class Iter_type >

    class Semantic_actions 

    {

    public:



        typedef typename Value_type::Config_type Config_type;

        typedef typename Config_type::String_type String_type;

        typedef typename Config_type::Object_type Object_type;

        typedef typename Config_type::Array_type Array_type;

        typedef typename String_type::value_type Char_type;



        Semantic_actions( Value_type& value )

        :   value_( value )

        ,   current_p_( 0 )

        {

        }



        void begin_obj( Char_type c )

        {

            assert( c == '{' );



            begin_compound< Object_type >();

        }



        void end_obj( Char_type c )

        {

            assert( c == '}' );



            end_compound();

        }



        void begin_array( Char_type c )

        {

            assert( c == '[' );

     

            begin_compound< Array_type >();

        }



        void end_array( Char_type c )

        {

            assert( c == ']' );



            end_compound();

        }



        void new_name( Iter_type begin, Iter_type end )

        {

            assert( current_p_->type() == obj_type );



            name_ = get_str< String_type >( begin, end );

        }



        void new_str( Iter_type begin, Iter_type end )

        {

            add_to_current( get_str< String_type >( begin, end ) );

        }



        void new_true( Iter_type begin, Iter_type end )

        {

            assert( is_eq( begin, end, "true" ) );



            add_to_current( true );

        }



        void new_false( Iter_type begin, Iter_type end )

        {

            assert( is_eq( begin, end, "false" ) );



            add_to_current( false );

        }



        void new_null( Iter_type begin, Iter_type end )

        {

            assert( is_eq( begin, end, "null" ) );



            add_to_current( Value_type() );

        }



        void new_int( boost::int64_t i )

        {

            add_to_current( i );

        }



        void new_uint64( boost::uint64_t ui )

        {

            add_to_current( ui );

        }



        void new_real( double d )

        {

            add_to_current( d );

        }



    private:



        Semantic_actions& operator=( const Semantic_actions& ); 

                                    // to prevent "assignment operator could not be generated" warning



        Value_type* add_first( const Value_type& value )

        {

            assert( current_p_ == 0 );



            value_ = value;

            current_p_ = &value_;

            return current_p_;

        }



        template< class Array_or_obj >

        void begin_compound()

        {

            if( current_p_ == 0 )

            {

                add_first( Array_or_obj() );

            }

            else

            {

                stack_.push_back( current_p_ );



                Array_or_obj new_array_or_obj;   // avoid copy by building new array or object in place



                current_p_ = add_to_current( new_array_or_obj );

            }

        }



        void end_compound()

        {

            if( current_p_ != &value_ )

            {

                current_p_ = stack_.back();

                

                stack_.pop_back();

            }    

        }



        Value_type* add_to_current( const Value_type& value )

        {

            if( current_p_ == 0 )

            {

                return add_first( value );

            }

            else if( current_p_->type() == array_type )

            {

                current_p_->get_array().push_back( value );



                return &current_p_->get_array().back(); 

            }

            

            assert( current_p_->type() == obj_type );



            return &Config_type::add( current_p_->get_obj(), name_, value );

        }



        Value_type& value_;             // this is the object or array that is being created

        Value_type* current_p_;         // the child object or array that is currently being constructed



        std::vector< Value_type* > stack_;   // previous child objects and arrays



        String_type name_;              // of current name/value pair

    };

