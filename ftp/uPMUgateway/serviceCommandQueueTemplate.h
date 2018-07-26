/*
 */

/* 
 * File:   serviceQueuesTemplate.h
 * Author: mcp
 *
 * Created on January 24, 2016, 8:45 PM
 */

#ifndef SERVICECOMMANDQUEUETEMPLATE_H
#define SERVICECOMMANDQUEUETEMPLATE_H

#include <queue>
#include <deque>
#include "serviceQueue.h"

//namespace serviceInstanceApp {
    
// Queue class that has thread synchronisation
template <typename T>
class SynchronisedCommandQueue
{
private:

    std::queue<T> m_queue; // Use STL queue to store data
    boost::mutex m_mutex; // The mutex to synchronise on
    //boost::condition_variable m_cond; // The condition to wait for
 
public:
    
    int queueLength;
    queueState_t queueState;
    
    SynchronisedCommandQueue(int len) {
        queueLength = len;
        queueState = QUEUE_EMPTY;
    }
    
    // Add data to the queue and notify others
    int Enqueue(const T& data) {
        if(m_queue.size() >= queueLength) {
            return 0;
        }
        // Acquire lock on the queue
        boost::unique_lock<boost::mutex> lock(m_mutex);
 
        // Add the data to the queue
        m_queue.push(data);
        if(m_queue.size() < queueLength) {
            queueState = QUEUE_AVAILABLE;
        }
        else {
            queueState = QUEUE_FULL;
        }
        // Notify others that data is ready
        //m_cond.notify_one();
        return m_queue.size();
        } // Lock is automatically released here
 
        // Get data from the queue. Wait for data if not available
    T Dequeue() {
 
    // Acquire lock on the queue
    boost::unique_lock<boost::mutex> lock(m_mutex);
 
    // When there is no data, wait till someone fills it.
        // Lock is automatically released in the wait and obtained
    // again after the wait
    //while (m_queue.size()==0) m_cond.wait(lock);
    if(m_queue.size() == 0) {
        return NULL;
    } 
    // Retrieve the data from the queue
    T result=m_queue.front(); m_queue.pop();
    if(m_queue.size() > 0) {
        queueState = QUEUE_AVAILABLE;
    }
    else {
        queueState = QUEUE_EMPTY;
    }
    return result;
 
    } // Lock is automatically released here
};
 
//}



#endif /* SERVICECOMMANDQUEUETEMPLATE_H */

