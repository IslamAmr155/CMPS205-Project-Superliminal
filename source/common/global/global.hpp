#pragma once
#include <chrono>

// This function returns the time in seconds since the first time it was called.
namespace our 
{
    inline float getMyGameTime()
    {
        // Record the start time when the function is first called
        static auto start = std::chrono::high_resolution_clock::now();
        static float delta = 0.0f;

        // Get the current time
        auto now = std::chrono::high_resolution_clock::now();

        // Calculate the elapsed time in seconds
        std::chrono::duration<float> elapsed = now - start;

        // Update delta with the change in seconds
        delta = elapsed.count();

        return delta;
    }
}
