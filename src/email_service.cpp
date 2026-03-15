#include "email_service.h"
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace {
// Helper function to read configuration from ~/.meal_prep.conf

struct UploadStatus {
  const char *data;
  size_t length;
  size_t pos;
};

size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp) {
  UploadStatus *upload_ctx = static_cast<UploadStatus *>(userp);
  size_t room = size * nmemb;

  if (room < 1)
    return 0;

  size_t to_copy = std::min(room, upload_ctx->length - upload_ctx->pos);
  if (to_copy > 0) {
    memcpy(ptr, upload_ctx->data + upload_ctx->pos, to_copy);
    upload_ctx->pos += to_copy;
  }
  return to_copy;
}
} // namespace

void SendEmail(const std::string &toAddress, const std::string &subject,
               const std::string &body, const std::string &senderEmail,
               const std::string &senderPassword) {
  if (senderEmail.empty() || senderPassword.empty()) {
    std::cerr << "Warning: Email credentials are not configured. Skipping email send.\n";
    return;
  }
  try {

    std::string payload = "To: " + toAddress +
                          "\r\n"
                          "From: " +
                          senderEmail +
                          "\r\n"
                          "Subject: " +
                          subject +
                          "\r\n"
                          "\r\n" +
                          body;

    UploadStatus upload_ctx;
    upload_ctx.data = payload.c_str();
    upload_ctx.length = payload.length();
    upload_ctx.pos = 0;

    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
      curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
      curl_easy_setopt(curl, CURLOPT_USERNAME, senderEmail.c_str());
      curl_easy_setopt(curl, CURLOPT_PASSWORD, senderPassword.c_str());

      curl_easy_setopt(curl, CURLOPT_MAIL_FROM, senderEmail.c_str());

      struct curl_slist *recipients = NULL;
      recipients = curl_slist_append(recipients, toAddress.c_str());
      curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

      curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
      curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                  << std::endl;
      }

      curl_slist_free_all(recipients);
      curl_easy_cleanup(curl);
    }
  } catch (const std::exception &e) {
    std::cerr << "Email send error: " << e.what() << std::endl;
  }
}
