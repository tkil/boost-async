#include <iostream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "tlog.hpp"

int main( int argc, char * argv [] )
{
    boost::asio::io_service io;

    typedef boost::scoped_ptr< boost::asio::io_service::work > work_ptr_t;
    work_ptr_t work_ptr( new boost::asio::io_service::work( io ) );

    const int n_threads = argc > 1 ? boost::lexical_cast< int >( argv[1] ) : 5;
    boost::thread_group threads;
    for ( int i = 0; i < n_threads; ++i )
        threads.create_thread( [&](){ io.run(); } );

    const int n_events = argc > 2 ? boost::lexical_cast< int >( argv[2] ) : 10;
    int events_seen = 0;
    for ( int i = 0; i < n_events; ++i )
        io.post( [&](){ ++events_seen; } );

    work_ptr.reset();

    if ( n_threads )
        threads.join_all();
    else
        io.run();

    tlog << "n=" << n_events << ", seen=" << events_seen << endl;
    return events_seen == n_events ? 0 : 1;
}
