#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

std::string EncodeBase64(const unsigned char* data, size_t len);
std::string DecodeBase64(const std::string& encoded, std::vector<unsigned char>& out);

#endif

