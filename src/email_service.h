#pragma once

#include <string>

/**
 * @brief Sends an email via SMTP.
 *
 * @param toAddress The recipient's email address.
 * @param subject The email subject line.
 * @param body The main content of the email.
 * @param senderEmail The sender's email address.
 * @param senderPassword The sender's email password or app-specific password.
 */
void SendEmail(const std::string &toAddress, const std::string &subject,
               const std::string &body, const std::string &senderEmail,
               const std::string &senderPassword);
