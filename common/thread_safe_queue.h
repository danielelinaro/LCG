/*=========================================================================
 *
 *   Program:     lcg
 *   Filename:    thread_safe_queue.h
 *
 *   Copyright (C) 2012,2013,2014 Daniele Linaro
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *=========================================================================*/

#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <algorithm>

#include <boost/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/noncopyable.hpp>

namespace lcg {

template <typename T>
class ThreadSafeQueue : private boost::noncopyable
{

        typedef typename boost::call_traits<T>::param_type param_type;
        typedef typename std::deque<T>::value_type value_type;
        typedef typename std::deque<T>::size_type size_type;

public:
        size_type size()
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                return m_queue.size();
        }

        bool empty()
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                return m_queue.size() == 0;
        }
    
        void push_back(param_type elem)
        {
                // In a block to release the mutex before notifying condition variable
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        m_queue.push_back(elem);
                }
                m_cv.notify_all();
        }

        void push_front(param_type elem)
        {
                // In a block to release the mutex before notifying condition variable
                {
                        boost::unique_lock<boost::mutex> lock(m_mutex);
                        m_queue.push_front(elem);
                }
                m_cv.notify_all();
        }

        value_type pop_front()
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_queue.empty())
                        m_cv.wait(lock);
                T elem = m_queue.front();
                m_queue.pop_front();
                return elem;
        }

        value_type pop_back()
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_queue.empty())
                        m_cv.wait(lock);
                T elem = m_queue.back();
                m_queue.pop_back();
                return elem;
        }

        value_type front()
        {
                // Returns front element by value and not by reference because the queue could be 
                // modified by another thread.
                boost::unique_lock<boost::mutex> lock(m_mutex);
                while (m_queue.empty())
                        m_cv.wait(lock);
                return m_queue.front();
        }

        template <typename F>
        void for_each(F func)
        {
                boost::unique_lock<boost::mutex> lock(m_mutex);
                std::for_each(m_queue.begin(), m_queue.end(), func);
        }

private:
        std::deque<T> m_queue;
        boost::mutex m_mutex;
        boost::condition_variable m_cv;
};

}

#endif

