#pragma once

#include <string>

// Send an email via SMTP
void SendEmail(const std::string &toAddress, const std::string &subject,
               const std::string &body, const std::string &senderEmail,
               const std::string &senderPassword);
