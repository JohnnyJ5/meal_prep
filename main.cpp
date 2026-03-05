#include <algorithm>
#include <boost/operators.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "ingredient.h"
#include "meal.h"
#include "meal_factory.h"
#include <map>
#include <functional>
#include <sstream>

#include <crow.h>

#include <curl/curl.h>
#include <cstring>
#include <vector>


// Helper function to read configuration from ~/.meal_prep.conf
std::map<std::string, std::string> read_config() {
    std::map<std::string, std::string> config;
    const char* homeDir = std::getenv("HOME");
    std::string config_path = homeDir ? std::string(homeDir) + "/.meal_prep.conf" : "/home/johnnyj/.meal_prep.conf";
    
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Warning: Config file not found at " << config_path << std::endl;
        std::cerr << "Create ~/.meal_prep.conf with contents:" << std::endl;
        std::cerr << "  email=your-email@gmail.com" << std::endl;
        std::cerr << "  password=your-app-password" << std::endl;
        return config;
    }
    
    std::string line;
    while (std::getline(config_file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        size_t delimiter = line.find('=');
        if (delimiter != std::string::npos) {
            std::string key = line.substr(0, delimiter);
            std::string value = line.substr(delimiter + 1);
            config[key] = value;
        }
    }
    
    config_file.close();
    return config;
}

struct UploadStatus {
    const char* data;
    size_t length;
    size_t pos;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp) {
    UploadStatus *upload_ctx = static_cast<UploadStatus*>(userp);
    size_t room = size * nmemb;
    
    if(room < 1) return 0;

    size_t to_copy = std::min(room, upload_ctx->length - upload_ctx->pos);
    if(to_copy > 0) {
        memcpy(ptr, upload_ctx->data + upload_ctx->pos, to_copy);
        upload_ctx->pos += to_copy;
    }
    return to_copy;
}

void SendEmail(const std::string& toAddress, const std::string& subject, const std::string& body) 
{
    try {
        // Read credentials from config file
        auto config = read_config();
        
        if (config.find("email") == config.end() || config.find("password") == config.end()) {
            std::cerr << "Error: 'email' and 'password' not found in ~/.meal_prep.conf" << std::endl;
            return;
        }
        
        std::string email = config["email"];
        std::string password = config["password"];

        std::string payload = 
            "To: " + toAddress + "\r\n"
            "From: " + email + "\r\n"
            "Subject: " + subject + "\r\n"
            "\r\n" + 
            body;

        UploadStatus upload_ctx;
        upload_ctx.data = payload.c_str();
        upload_ctx.length = payload.length();
        upload_ctx.pos = 0;

        CURL *curl;
        CURLcode res = CURLE_OK;

        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
            curl_easy_setopt(curl, CURLOPT_USERNAME, email.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());

            curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email.c_str());
            
            struct curl_slist *recipients = NULL;
            recipients = curl_slist_append(recipients, toAddress.c_str());
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
        }
    } catch (const std::exception &e) {
        std::cerr << "Email send error: " << e.what() << std::endl;
    }
}

void ConsolidateAllIngredients(std::map<std::string, Ingredient>& allIngredients, const std::vector<std::reference_wrapper<Meal>>& meals) {
    for (const auto& mealRef : meals) {
        const Meal& meal = mealRef.get();
        for (const auto& ingredient : meal.getIngredients()) {
            auto it = allIngredients.find(ingredient.getName());
            if (it != allIngredients.end()) {
                it->second += ingredient;  
            } else {
                allIngredients[ingredient.getName()] = ingredient;
            }
        }
    }
}

void SendEmail(const std::map<std::string, Ingredient>& allIngredients,const std::vector<std::reference_wrapper<Meal>>& meals)
{
    std::stringstream ss;
    ss << "Combined ingredients for: ";
    for (const auto& mealRef : meals) {
        ss << mealRef.get().getName() << "; ";
    }
    ss << std::endl;
    for (const auto& pair : allIngredients) {
        ss << "  [] " <<pair.second << std::endl;
    }
    // Prepare body with CRLF line endings (SMTP requires CRLF)
    std::string body = ss.str();
    std::string crlf;
    crlf.reserve(body.size() * 2);
    for (char c : body) {
        if (c == '\n') crlf += "\r\n";
        else crlf += c;
    }
    try {
        std::vector<std::pair<std::string, std::string>> recipients = {
            {"Michael Coffey", "michaelcoffey5@gmail.com"},
            // {"Suzanne Coffey", "suzcoffey22@gmail.com"}
        };
        std::for_each(recipients.begin(), recipients.end(), [&](const std::pair<std::string, std::string>& addr) {
            std::cout << "Sending email to: " << addr.second << std::endl;
            SendEmail(addr.second, "Weekly Meal Prep Ingredients", crlf);
        });
        
    } catch (const std::exception &e) {
        std::cerr << "Email error: " << e.what() << std::endl;
    }
}

void PrintWeeklySchedule(const std::map<std::string, Ingredient>& allIngredients, const std::vector<std::reference_wrapper<Meal>>& meals) 
{
    std::cout << "\nWeekly Meal Prep Schedule:" << std::endl;
    std::cout << "Monday:" << std::endl;
    std::cout << "  - Plan meals for the week." << std::endl;
    std::cout << "Tuesday:" << std::endl;
    std::cout << "  - Place Whole Food Order for the week" << std::endl;
    std::cout << "  - Claudia 9-1." << std::endl;
    std::cout << "Wednesday:" << std::endl;
    std::cout << "  - Suz - Pick up any unavailable items from Whole Foods." << std::endl;
    std::cout << "Thursday:" << std::endl;
    std::cout << "  - Claudia 11-5." << std::endl;
    std::cout << "  - Prepare 1 meal." << std::endl;
    std::cout << "  - Meal prep for Sunday." << std::endl;
    std::cout << "Friday:" << std::endl;
    std::cout << "  - Relax and enjoy!" << std::endl;
    std::cout << "Saturday:" << std::endl;
    std::cout << "  - Free day or prep for next week." << std::endl;

    //Sunday should have enough food until Thursday when Claudia makes more.
    std::cout << "Sunday:" << std::endl;
    std::cout << "  - Slow cooker meal." << std::endl;
    std::cout << "  - Prepare sides like potatoes and green beans and mac and cheese etc." << std::endl;
}

int main(int argc, char** argv) 
{
    try
    {      
        curl_global_init(CURL_GLOBAL_DEFAULT);
        // Simple command line parsing for now
        std::vector<std::string> mealNames;
        bool listMeals = false;
        bool serveWeb = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--list" || arg == "-l") {
                listMeals = true;
            } else if ((arg == "--meal" || arg == "-m") && i + 1 < argc) {
                mealNames.push_back(argv[++i]);
            } else if (arg == "--serve" || arg == "-s") {
                serveWeb = true;
            }
        }

        MealFactory factory;

        if (serveWeb) {
            crow::SimpleApp app;

            // Route: Get all available meals
            CROW_ROUTE(app, "/api/meals")([&factory]() {
                std::vector<std::string> meals;
                factory.getAvailableMeals(meals);
                crow::json::wvalue res;
                for (size_t i = 0; i < meals.size(); ++i) {
                    res[i] = meals[i];
                }
                return res;
            });

            // Route: Plan selected meals and trigger email
            CROW_ROUTE(app, "/api/plan").methods(crow::HTTPMethod::POST)([&factory](const crow::request& req) {
                auto body = crow::json::load(req.body);
                if (!body) return crow::response(400, "Invalid JSON");

                std::vector<std::unique_ptr<Meal>> createdMeals;
                std::vector<std::reference_wrapper<Meal>> mealRefs;
                std::vector<std::string> failedMeals;

                for (const auto& mealJson : body) {
                    std::string mealName = mealJson.s();
                    if (auto meal = factory.createMeal(mealName)) {
                        createdMeals.push_back(std::move(meal));
                    } else {
                        failedMeals.push_back(mealName);
                    }
                }

                if (createdMeals.empty()) {
                    return crow::response(400, "No valid meals selected.");
                }

                for (auto& m : createdMeals) {
                    mealRefs.push_back(*m);
                }

                std::map<std::string, Ingredient> allIngredients;
                ConsolidateAllIngredients(allIngredients, mealRefs);

                // Re-route console output to capture schedule response for frontend
                std::stringstream scheduleOutput;
                std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
                std::cout.rdbuf(scheduleOutput.rdbuf());

                PrintWeeklySchedule(allIngredients, mealRefs);

                std::cout.rdbuf(oldCoutStreamBuf); // Restore standard output

                // Send Email
                SendEmail(allIngredients, mealRefs);

                crow::json::wvalue res;
                res["status"] = "success";
                res["schedule"] = scheduleOutput.str();
                if (!failedMeals.empty()) {
                    for (size_t i = 0; i < failedMeals.size(); ++i) {
                        res["failed_meals"][i] = failedMeals[i];
                    }
                }
                return crow::response(std::move(res));
            });

            // Route: Serve index.html at root
            CROW_ROUTE(app, "/")([]() {
                crow::response res;
                res.set_static_file_info("static/index.html");
                return res;
            });

            // Route: Serve all other static files (CSS, JS)
            CROW_ROUTE(app, "/<string>")([](std::string path) {
                crow::response res;
                res.set_static_file_info("static/" + path);
                return res;
            });

            // Start the server
            std::cout << "Starting Meal Prep API on http://0.0.0.0:8080" << std::endl;
            app.port(8080).multithreaded().run();
            return 0;
        }

        if (listMeals) {
            std::cout << "Available meals:" << std::endl;
            std::vector<std::string> mealNames;
            factory.getAvailableMeals(mealNames);
            for (const auto& mealName : mealNames) {
                std::cout << "-m " << mealName << std::endl;
            }
            return 0;
        }

        // Create meals using factory
        std::vector<std::unique_ptr<Meal>> meals;
        if (!mealNames.empty()) {
            for (const auto& mealName : mealNames) {
                auto meal = factory.createMeal(mealName);
                if (meal) {
                    meals.push_back(std::move(meal));
                } else {
                    std::cout << "Unknown meal: " << mealName << std::endl;
                    return 1;
                }
            }
        } else {
            std::cout << "No meals to create" << std::endl;
            return 0;
        }

        // Convert to vector of references for consolidation
        std::vector<std::reference_wrapper<Meal>> mealRefs;
        for (auto& meal : meals) {
            mealRefs.push_back(*meal);
        }

        std::map<std::string, Ingredient> allIngredients;
        ConsolidateAllIngredients(allIngredients, mealRefs);
        SendEmail(allIngredients, mealRefs);
        PrintWeeklySchedule(allIngredients, mealRefs);
        
        curl_global_cleanup();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}