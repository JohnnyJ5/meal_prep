#pragma once

#include <curl/curl.h>
#include <cstdlib>
#include <string>

namespace curl_utils {

inline size_t writeCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
  return size * nmemb;
}

inline std::string urlEncode(const std::string &value) {
  CURL *curl = curl_easy_init();
  if (!curl) return value;
  char *output = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
  if (!output) {
    curl_easy_cleanup(curl);
    return value;
  }
  std::string res(output);
  curl_free(output);
  curl_easy_cleanup(curl);
  return res;
}

} // namespace curl_utils
