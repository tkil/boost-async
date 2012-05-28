/* compile line:

   g++ -std=c++0x -Wall -pthread \
     async-signals.cpp -o async-signals \
     -lboost_thread-mt -lpthread -lrt

*/

#include <unistd.h>

#include <iostream>
#include <vector>
#include <deque>
#include <sstream>

#include <boost/signals2.hpp>

#include <boost/thread.hpp>

const int nap = 1;

typedef boost::signals2::signal< void () > sig_t;

// =====================================================================

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

// =====================================================================

class thread_pool
{
public:
    typedef boost::function< void () > work_t;

    thread_pool( int n = 5 );
    ~thread_pool();

    void add_work( work_t work );

private:

    typedef boost::thread thread_t;
    typedef std::vector< thread_t > thread_vec_t;

    typedef std::deque< work_t > work_queue_t;

    typedef boost::mutex mutex_t;
    typedef boost::unique_lock< mutex_t > lock_t;
    typedef boost::condition_variable cond_t;

    bool running;

    thread_vec_t thread_vec;

    work_queue_t work_queue;
    mutex_t work_mutex;
    cond_t work_cond;

    void do_work();
};

thread_pool::thread_pool( int n )
    : running( true ),
      thread_vec(),
      work_queue(),
      work_mutex(),
      work_cond()
{
    for ( int i = 0; i < n; ++i )
    {
        thread_vec.push_back( thread_t( &thread_pool::do_work, this ) );
        // tlog << "created thread " << thread_vec.back().get_id() << endl;
    }
}

thread_pool::~thread_pool()
{
    {
        lock_t work_lock( work_mutex );
        running = false;
    }

    work_cond.notify_all();

    for ( thread_t & t : thread_vec )
    {
        // boost::thread::id tid( t.get_id() );
        // tlog << "joining thread " << tid << endl;
        t.join();
        // tlog << "joined thread " << tid << endl;
    }
}

void
thread_pool::add_work( work_t work )
{
    // tlog << "adding work " << endl;
    lock_t work_lock( work_mutex );
    work_queue.push_back( work );
    work_cond.notify_one();
}

void
thread_pool::do_work()
{
    while ( true )
    {
        work_t work;
        {
            lock_t work_lock( work_mutex );

            if ( running )
            {
                // tlog << "waiting, running=" << running << endl;
                work_cond.wait( work_lock );
                // tlog << "woke up" << endl;
            }

            // tlog << "size=" << work_queue.size()
            //      << ", running=" << running << endl;

            if ( work_queue.empty() )
            {
                if ( running )
                    continue;
                else
                    break;
            }

            work = work_queue.front();
            work_queue.pop_front();
        }

        work();
    }
}

typedef boost::shared_ptr< thread_pool > thread_pool_ptr;

// =====================================================================

struct async_combiner_t
{
    typedef void result_type;

    async_combiner_t() : pool_ptr( new thread_pool ) {}
    async_combiner_t( const async_combiner_t & ) = default;

    thread_pool_ptr pool_ptr;

    template < typename InputIter >
    void operator()( InputIter first, InputIter last )
    {
        for ( InputIter i = first; i != last; ++i )
            pool_ptr->add_work( [=](){ *i; } );
    }

};

typedef boost::signals2::signal< void (), async_combiner_t > async_sig_t;

// =====================================================================

struct slot1_t
{
    void operator()()
    {
        tlog << "  slot1_t: start" << endl;
        sleep( nap );
        tlog << "  slot1_t: end" << endl;
    }
};

struct slot2_t
{
    void operator()()
    {
        tlog << "  slot2_t: start" << endl;
        sleep( nap );
        tlog << "  slot2_t: end" << endl;
    }
};

// =====================================================================

int main( int /* argc */, char * /* argv */ [] )
{
   slot1_t slot1;
   slot2_t slot2;

   tlog << "main: sync call, sync exec" << endl;
   sig_t sig;
   sig.connect( slot1 );
   sig.connect( slot2 );
   sig();
   tlog << "main: done" << endl;

   tlog << "========================================" << endl;
   tlog << "main: async call, sync exec" << endl;
   boost::thread t1( [&](){ sig(); } );
   tlog << "main: done" << endl;
   t1.join();

   tlog << "========================================" << endl;
   tlog << "main: sync call, async exec" << endl;
   async_sig_t async_sig;
   async_sig.connect( slot1 );
   async_sig.connect( slot2 );
   async_sig();
   tlog << "main: done" << endl;

   return 0;
}
