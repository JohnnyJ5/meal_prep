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

#include <vmime/vmime.hpp>
#include <vmime/platforms/posix/posixHandler.hpp>



// Helper function to read configuration from ~/.meal_prep.conf
std::map<std::string, std::string> read_config() {
    std::map<std::string, std::string> config;
    std::string config_path = "/home/johnnyj/meal_prep/.meal_prep.conf";
    
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
        
       vmime::platform::setHandler<vmime::platforms::posix::posixHandler>();

        // // --- Build message ---
        // vmime::messageBuilder builder;
        // builder.setExpeditor(vmime::mailbox("Michael Coffey", email));
        // builder.addRecipient(vmime::make_shared<vmime::mailbox>(toAddress));
        // builder.setSubject(vmime::text("Hello from vmime"));
        // builder.getTextPart()->setText(
        //     vmime::make_shared<vmime::stringContentHandler>(
        //         "This email was sent from C++ using vmime 🚀"
        //     )
        // );

        // vmime::shared_ptr<vmime::message> msg = builder.construct();

        // // --- SMTP session ---
        // vmime::utility::url url("smtp://smtp.gmail.com:587");
        // vmime::shared_ptr<vmime::net::session> session =
        // vmime::net::session::create();

        // vmime::shared_ptr<vmime::net::transport> transport =
        // session->getTransport(url);

        // // Auth
        // transport->setProperty("connection.tls", true);
        // transport->setProperty("auth", true);
        // transport->setProperty("options.need-authentication", true);

        // transport->connect(
        //     vmime::make_shared<vmime::net::authentication::basicAuthenticator>(email, password)
        // );

        // // --- Send ---
        // transport->send(msg);
        // transport->disconnect();
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
        // Simple command line parsing for now
        std::vector<std::string> mealNames;
        bool listMeals = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--list" || arg == "-l") {
                listMeals = true;
            } else if ((arg == "--meal" || arg == "-m") && i + 1 < argc) {
                mealNames.push_back(argv[++i]);
            } 
        }

        MealFactory factory;

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

    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}