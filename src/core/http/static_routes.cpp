#include "core/http/static_routes.h"

#include <iostream>

void registerStaticRoutes(crow::App<RequestTimerMiddleware>& app) {
    // Route: Serve all other static files (CSS, JS)
    CROW_ROUTE(app, "/<string>")
    ([](const std::string& path) {
        if (path.find("..") != std::string::npos || path.find('/') != std::string::npos) {
            return crow::response(400, "Invalid path");
        }
        crow::response res;
        res.set_static_file_info("static/" + path);
        if (res.code == 404) {
            std::cerr << "Static file not found: static/" << path << std::endl;
        }
        return res;
    });
}
