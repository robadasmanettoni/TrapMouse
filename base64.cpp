#include "base64.h"
#include <string>
#include <vector>

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string EncodeBase64(const unsigned char* data, size_t len) {
    std::string out;
    out.reserve((len + 2) / 3 * 4);

    for (size_t i = 0; i < len; i += 3) {
        unsigned char input[3] = {0, 0, 0};
        size_t chunk = len - i < 3 ? len - i : 3;

        for (size_t j = 0; j < chunk; ++j)
            input[j] = data[i + j];

        out += base64_table[(input[0] & 0xFC) >> 2];
        out += base64_table[((input[0] & 0x03) << 4) | ((input[1] & 0xF0) >> 4)];
        out += (chunk > 1) ? base64_table[((input[1] & 0x0F) << 2) | ((input[2] & 0xC0) >> 6)] : '=';
        out += (chunk > 2) ? base64_table[input[2] & 0x3F] : '=';
    }

    return out;
}

static int Base64CharValue(char c) {
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::string DecodeBase64(const std::string& encoded, std::vector<unsigned char>& out) {
    out.clear();
    size_t len = encoded.size();
    unsigned char in[4];
    int val[4];

    for (size_t i = 0; i < len;) {
        size_t filled = 0;
        for (; filled < 4 && i < len; ++i) {
            char c = encoded[i];
            if (c == '=' || Base64CharValue(c) >= 0) {
                in[filled++] = c;
            }
        }
        if (filled < 4) break;

        for (int j = 0; j < 4; ++j)
            val[j] = (in[j] == '=') ? 0 : Base64CharValue(in[j]);

        out.push_back((unsigned char)((val[0] << 2) | (val[1] >> 4)));
        if (in[2] != '=') out.push_back((unsigned char)((val[1] << 4) | (val[2] >> 2)));
        if (in[3] != '=') out.push_back((unsigned char)((val[2] << 6) | val[3]));
    }

    return std::string(out.begin(), out.end());
}

