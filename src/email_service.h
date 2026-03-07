#pragma once

#include <string>

// Send an email via SMTP using credentials from ~/.meal_prep.conf
void SendEmail(const std::string &toAddress, const std::string &subject,
               const std::string &body);
