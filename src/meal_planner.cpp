#include "meal_planner.h"
#include "email_service.h"
#include <algorithm>
#include <iostream>
#include <sstream>

void ConsolidateAllIngredients(
    std::map<std::string, Ingredient> &allIngredients,
    const std::vector<std::reference_wrapper<Meal>> &meals) {
  for (const auto &mealRef : meals) {
    const Meal &meal = mealRef.get();
    for (const auto &ingredient : meal.getIngredients()) {
      auto it = allIngredients.find(ingredient.getName());
      if (it != allIngredients.end()) {
        it->second += ingredient;
      } else {
        allIngredients[ingredient.getName()] = ingredient;
      }
    }
  }
}

void SendPlanEmail(
    const std::map<std::string, Ingredient> &allIngredients,
    const std::map<std::string, std::vector<std::string>> &schedule,
    const Config &config) {
  std::stringstream ss;
  ss << "Weekly Meal Schedule:\n";
  std::vector<std::string> days = {"Monday",   "Tuesday", "Wednesday",
                                   "Thursday", "Friday",  "Saturday",
                                   "Sunday"};
  for (const auto &day : days) {
    ss << day << ":\n";
    auto it = schedule.find(day);
    if (it != schedule.end() && !it->second.empty()) {
      for (const auto &m : it->second) {
        ss << "  - " << m << "\n";
      }
    } else {
      ss << "  - (Nothing planned)\n";
    }
  }
  ss << "\nCombined ingredients for shopping:\n";
  for (const auto &pair : allIngredients) {
    ss << "  [] " << pair.second << std::endl;
  }
  // Prepare body with CRLF line endings (SMTP requires CRLF)
  std::string body = ss.str();
  std::string crlf;
  crlf.reserve(body.size() * 2);
  for (char c : body) {
    if (c == '\n')
      crlf += "\r\n";
    else
      crlf += c;
  }
  try {
    if (config.email_recipients.empty()) {
      std::cerr << "No email recipients configured." << std::endl;
      return;
    }

    std::for_each(config.email_recipients.begin(),
                  config.email_recipients.end(), [&](const std::string &addr) {
                    std::cout << "Sending email to: " << addr << std::endl;
                    SendEmail(addr, "Weekly Meal Prep Ingredients", crlf,
                              config.sender_email, config.sender_password);
                  });

  } catch (const std::exception &e) {
    std::cerr << "Email error: " << e.what() << std::endl;
  }
}

void PrintWeeklySchedule(
    std::ostream &os,
    const std::map<std::string, std::vector<std::string>> &schedule) {
  os << "\nWeekly Meal Prep Schedule:\n";
  std::vector<std::string> days = {"Monday",   "Tuesday", "Wednesday",
                                   "Thursday", "Friday",  "Saturday",
                                   "Sunday"};
  for (const auto &day : days) {
    os << day << ":\n";
    auto it = schedule.find(day);
    if (it != schedule.end() && !it->second.empty()) {
      for (const auto &m : it->second) {
        os << "  - " << m << "\n";
      }
    } else {
      os << "  - (Nothing planned)\n";
    }
  }
}
