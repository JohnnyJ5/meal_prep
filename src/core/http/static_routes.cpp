#include "core/http/static_routes.h"

#include <iostream>
#include <string>

void registerStaticRoutes(crow::App<RequestTimerMiddleware>& app) {
    // Page routes: each page is its own HTML document. The root redirects
    // to the planner so existing bookmarks keep working.
    CROW_ROUTE(app, "/planner")
    ([]() {
        crow::response res;
        res.set_static_file_info("static/pages/planner/index.html");
        return res;
    });

    CROW_ROUTE(app, "/workouts")
    ([]() {
        crow::response res;
        res.set_static_file_info("static/pages/workouts/index.html");
        return res;
    });

    // Static asset routes. <path> captures the remainder of the URL so
    // nested paths like /shared/base.css or /pages/planner/planner.js work.
    auto serveStaticPath = [](const std::string& subpath, const std::string& rel) {
        if (subpath.find("..") != std::string::npos) {
            return crow::response(400, "Invalid path");
        }
        crow::response res;
        res.set_static_file_info("static/" + rel + subpath);
        if (res.code == 404) {
            std::cerr << "Static file not found: static/" << rel << subpath << std::endl;
        }
        return res;
    };

    CROW_ROUTE(app, "/shared/<path>")
    ([serveStaticPath](const std::string& subpath) { return serveStaticPath(subpath, "shared/"); });

    CROW_ROUTE(app, "/pages/<path>")
    ([serveStaticPath](const std::string& subpath) { return serveStaticPath(subpath, "pages/"); });

    // Top-level single-segment files (e.g., /favicon.ico if someone moves it back).
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
