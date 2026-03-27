#ifndef CURL_UTILS_H
#define CURL_UTILS_H

#include <cstdlib>
#include <string>

namespace curl_utils {

inline size_t writeCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

} // namespace curl_utils

#endif // CURL_UTILS_H
