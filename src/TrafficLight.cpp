#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // Lock mutex
    std::unique_lock uLock(_mutex);

    // Check if there are new messages
    if (_messages.empty())
    {
        _cond.wait(uLock);
    }

    T msg = std::move(_messages.front());
    _messages.pop_front();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    
    // Lock mutex
    std::unique_lock uLock(_mutex);

    // Add message to the queue
    _messages.emplace_back(std::move(msg));

    // Notify listening threads
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true)
    {
        TrafficLightPhase oldestReceivedPhase = _queue.receive();

        if (oldestReceivedPhase == TrafficLightPhase::green)
        {
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    std::thread thread(&TrafficLight::cycleThroughPhases, this);
    _threads.emplace_back(std::move(thread));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // Generate random number between 4s and 6s
    std::mt19937 randonNumber = std::mt19937(std::random_device{}());
    std::uniform_real_distribution<> distr(4000.0, 6000.0);
    double timeToWait = distr(randonNumber);
    
    std::unique_lock<std::mutex> uLock(_mtx);
    std::cout << "Thread '" << std::this_thread::get_id() << "' has to wait for: " << timeToWait << " ms." << std::endl;
    uLock.unlock();

    // Init stop watch
    auto lastUpdate = std::chrono::system_clock::now();

    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Compute time difference to stop watch
        auto currentTime = std::chrono::system_clock::now();
        double timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdate).count();

        if (timeSinceLastUpdate >= timeToWait)
        {
            // Toggle between red and green
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ? TrafficLightPhase::green : TrafficLightPhase::red;
            
            std::string color = (_currentPhase == TrafficLightPhase::red) ? "red" : "green";
            uLock.lock();
            std::cout << "Thread '" << std::this_thread::get_id() << "' has changed to color: " << color << std::endl;
            uLock.unlock();
            
            // Send update message to the queue
            _queue.send(std::move(_currentPhase));

            // Generate new random number
            randonNumber = std::mt19937(std::random_device{}());
            timeToWait = distr(randonNumber);

            // Reset stop watch
            lastUpdate = std::chrono::system_clock::now();
        }
    }
}