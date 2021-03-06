#pragma once

#include <mutex>
#include <condition_variable>
#include <cassert>

namespace nix {

/* This template class ensures synchronized access to a value of type
   T. It is used as follows:

     struct Data { int x; ... };

     Sync<Data> data;

     {
       auto data_(data.lock());
       data_->x = 123;
     }

   Here, "data" is automatically unlocked when "data_" goes out of
   scope.
*/

template<class T>
class Sync
{
private:
    std::mutex mutex;
    T data;

public:

    Sync() { }
    Sync(const T & data) : data(data) { }

    class Lock
    {
    private:
        Sync * s;
        std::unique_lock<std::mutex> lk;
        friend Sync;
        Lock(Sync * s) : s(s), lk(s->mutex) { }
    public:
        Lock(Lock && l) : s(l.s) { abort(); }
        Lock(const Lock & l) = delete;
        ~Lock() { }
        T * operator -> () { return &s->data; }
        T & operator * () { return s->data; }

        void wait(std::condition_variable & cv)
        {
            assert(s);
            cv.wait(lk);
        }

        template<class Rep, class Period, class Predicate>
        bool wait_for(std::condition_variable & cv,
            const std::chrono::duration<Rep, Period> & duration,
            Predicate pred)
        {
            assert(s);
            return cv.wait_for(lk, duration, pred);
        }

        template<class Clock, class Duration>
        std::cv_status wait_until(std::condition_variable & cv,
            const std::chrono::time_point<Clock, Duration> & duration)
        {
            assert(s);
            return cv.wait_until(lk, duration);
        }
    };

    Lock lock() { return Lock(this); }
};

}
