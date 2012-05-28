#ifndef TLOG_HPP
#define TLOG_HPP 1

namespace
{

struct threaded_logger_t
{
    boost::thread_specific_ptr< std::ostringstream > stream_ptr;
    std::ostream & os;
    boost::mutex mutex;

    explicit threaded_logger_t( std::ostream & os ) : os( os ) {}

    template < typename T >
    threaded_logger_t & emit( const T & val )
    {
        if ( ! stream_ptr.get() )
        {
            stream_ptr.reset( new std::ostringstream );
            *stream_ptr << boost::this_thread::get_id() << ": ";
        }
        *stream_ptr << val;
        return *this;
    }

    threaded_logger_t & eol()
    {
        if ( stream_ptr.get() )
        {
            boost::unique_lock< boost::mutex > lock( mutex );
            os << stream_ptr->str() << std::endl;
            stream_ptr->str( "" );
            *stream_ptr << boost::this_thread::get_id() << ": ";
        }
        return *this;
    }

    struct endl_t {};

};

threaded_logger_t tlog( std::clog );
threaded_logger_t::endl_t endl;

template < typename T >
threaded_logger_t &
operator << ( threaded_logger_t & tlt,
              const T & val )
{
    return tlt.emit( val );
}

threaded_logger_t &
operator << ( threaded_logger_t & tlt,
              const threaded_logger_t::endl_t & /* endl */ )
{
    return tlt.eol();
}

}

#endif // TLOG_HPP
