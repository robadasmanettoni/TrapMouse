#include "cipher_xor.h"
#include "base64.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <iomanip>

static const char* key = "pl4nkC0nst4nt#42";

static void xorCipher(std::vector<unsigned char>& data, const char* key) {
    size_t keyLen = std::strlen(key);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= key[i % keyLen];
}

static std::string toHex(const std::vector<unsigned char>& data) {
    std::ostringstream oss;
    for (size_t i = 0; i < data.size(); ++i)
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

static std::vector<unsigned char> fromHex(const std::string& hex) {
    std::vector<unsigned char> result;
    size_t len = hex.length();
    for (size_t i = 0; i + 1 < len; i += 2) {
        unsigned int byte;
        std::istringstream(hex.substr(i, 2)) >> std::hex >> byte;
        result.push_back((unsigned char)byte);
    }
    return result;
}

bool SavePassword_XOR(const char* plaintext, std::vector<unsigned char>& out) {
    if (!plaintext) return false;

    out.assign(plaintext, plaintext + std::strlen(plaintext));
    xorCipher(out, key);
    return true;
}

bool LoadPassword_XOR(const std::string& base64, char* out, size_t outSize) {
    if (base64.empty() || !out || outSize == 0)
        return false;

    std::vector<unsigned char> data;
    if (DecodeBase64(base64, data).empty())
        return false;

    const char* key = "pl4nkC0nst4nt#42";
    size_t keyLen = strlen(key);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= key[i % keyLen];

    size_t len = (data.size() < outSize - 1) ? data.size() : outSize - 1;
    std::memcpy(out, &data[0], len);
    out[len] = '\0';

    return true;
}

