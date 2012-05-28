#include <unistd.h>

#include <iostream>
#include <vector>
#include <deque>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#include "tlog.hpp"

const int nap = 1;

typedef boost::signals2::signal< void () > sync_sig_t;

// =====================================================================

struct async_combiner_t
{
    typedef void result_type;

    async_combiner_t( boost::asio::io_service * io_ptr ) : io_ptr( io_ptr ) {}
    async_combiner_t( const async_combiner_t & ) = default;

    boost::asio::io_service * io_ptr;

    template < typename InputIter >
    void operator()( InputIter first, InputIter last )
    {
        for ( InputIter i = first; i != last; ++i )
        {
            tlog << "enqueuing" << endl;
            io_ptr->post( [=](){ *i; } );
            // tlog << "... done" << endl;
        }
    }

};

typedef boost::signals2::signal< void (), async_combiner_t > async_sig_t;

// =====================================================================

struct slot_t
{
    int n;

    slot_t( int n ) : n( n ) {}

    void operator()()
    {
        tlog << "  slot " << n << ": start" << endl;
        sleep( nap );
        tlog << "  slot " << n << ": end" << endl;
    }
};

// =====================================================================

void
run_io( boost::asio::io_service & io )
{
    tlog << "starting io_service.run" << endl;
    io.run();
    tlog << "ending io_service.run" << endl;
}

int main( int argc, char * argv [] )
{
    slot_t slot1( 1 );
    slot_t slot2( 2 );

#if 0
    {
        tlog << "main: sync call, sync exec" << endl;
        sync_sig_t sig;
        sig.connect( slot1 );
        sig.connect( slot2 );
        sig();
        tlog << "main: done" << endl;
    }

    tlog << "========================================" << endl;

    {
        sync_sig_t sig;
        sig.connect( slot1 );
        sig.connect( slot2 );

        tlog << "main: async call, sync exec" << endl;
        boost::thread t1( [&](){ sig(); } );
        tlog << "main: done" << endl;
        t1.join();
    }

    tlog << "========================================" << endl;

#endif

    {
        boost::asio::io_service io;

        typedef boost::scoped_ptr< boost::asio::io_service::work > work_ptr_t;
        work_ptr_t work_ptr( new boost::asio::io_service::work( io ) );

        const int n_threads = argc > 1 ? boost::lexical_cast< int >( argv[1] ) : 5;
        boost::thread_group threads;
        for ( int i = 0; i < n_threads; ++i )
        {
            threads.create_thread( [&](){ run_io( io ); } );
            // threads.create_thread( [&](){ io.run(); } );
        }

        async_combiner_t async_combiner( &io );
        async_sig_t async_sig( async_combiner );
        async_sig.connect( slot1 );
        async_sig.connect( slot2 );

        tlog << "main: sync call, async exec" << endl;
        async_sig();
        tlog << "main: done" << endl;
        work_ptr.reset();

        if ( n_threads )
            threads.join_all();
        else
            run_io( io );
    }

    return 0;
}
