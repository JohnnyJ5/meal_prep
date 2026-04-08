#pragma once

#include <crow.h>

#include <chrono>
#include <iostream>
#include <string>

/**
 * @brief Middleware for logging the duration of each request.
 */
struct RequestTimerMiddleware {
    struct context {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    };

    void before_handle(crow::request& /*req*/, crow::response& /*res*/, context& ctx) {
        ctx.start_time = std::chrono::high_resolution_clock::now();
    }

    // cppcheck-suppress constParameterReference -- Crow's trait detection requires non-const
    // context&
    void after_handle(crow::request& req, crow::response& /*res*/, context& ctx) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - ctx.start_time)
                .count();

        // Standard Crow logs have a specific format, we'll try to follow it or make it distinct
        std::cout << "[TIMER] " << crow::method_name(req.method) << " " << req.url << " - "
                  << duration << "ms" << std::endl;
    }
};
