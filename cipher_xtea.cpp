#include "cipher_xtea.h"
#include "base64.h"
#include <fstream>
#include <vector>
#include <cstring>

static const unsigned char HARDCODED_KEY[16] = {
    0xB7, 0x4A, 0x92, 0xD3,
    0x6E, 0x5C, 0x8F, 0x13,
    0xA9, 0x22, 0xCF, 0x74,
    0x08, 0xB1, 0x3D, 0xE6
};

static void KeyToWords(const unsigned char key[16], unsigned int k[4]) {
    for (int i = 0; i < 4; ++i) {
        k[i] = (key[i*4] << 24) | (key[i*4+1] << 16) | (key[i*4+2] << 8) | key[i*4+3];
    }
}

static void EncryptBlock(unsigned int& v0, unsigned int& v1, const unsigned int* k) {
    unsigned int sum = 0, delta = 0x9E3779B9;
    for (int i = 0; i < 32; ++i) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    }
}

static void DecryptBlock(unsigned int& v0, unsigned int& v1, const unsigned int* k) {
    unsigned int sum = 0xC6EF3720, delta = 0x9E3779B9;
    for (int i = 0; i < 32; ++i) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
}

static void Pad(std::vector<unsigned char>& data) {
    size_t pad = 8 - (data.size() % 8);
    for (size_t i = 0; i < pad; ++i)
        data.push_back((unsigned char)pad);
}

static bool Unpad(std::vector<unsigned char>& data) {
    if (data.empty()) return false;
    unsigned char pad = data.back();
    if (pad == 0 || pad > 8 || pad > data.size()) return false;
    for (size_t i = 0; i < pad; ++i)
        if (data[data.size() - 1 - i] != pad) return false;
    data.resize(data.size() - pad);
    return true;
}

bool SavePassword_XTEA(const char* plaintext, std::vector<unsigned char>& out) {
    if (!plaintext) return false;

    std::vector<unsigned char> data(plaintext, plaintext + std::strlen(plaintext));
    Pad(data);

    unsigned int k[4];
    KeyToWords(HARDCODED_KEY, k);

    for (size_t i = 0; i < data.size(); i += 8) {
        unsigned int v0 = (data[i] << 24) | (data[i+1] << 16) | (data[i+2] << 8) | data[i+3];
        unsigned int v1 = (data[i+4] << 24) | (data[i+5] << 16) | (data[i+6] << 8) | data[i+7];
        EncryptBlock(v0, v1, k);
        data[i]     = (v0 >> 24) & 0xFF; data[i+1] = (v0 >> 16) & 0xFF;
        data[i+2]   = (v0 >> 8)  & 0xFF; data[i+3] =  v0        & 0xFF;
        data[i+4]   = (v1 >> 24) & 0xFF; data[i+5] = (v1 >> 16) & 0xFF;
        data[i+6]   = (v1 >> 8)  & 0xFF; data[i+7] =  v1        & 0xFF;
    }

    out.swap(data);
    return true;
}

bool LoadPassword_XTEA(const std::string& base64, char* out, size_t outSize) {
    if (base64.empty() || !out || outSize == 0)
        return false;

    std::vector<unsigned char> encrypted;
    if (DecodeBase64(base64, encrypted).empty())
        return false;

    if (encrypted.size() % 8 != 0)
        return false;

    unsigned int k[4];
    KeyToWords(HARDCODED_KEY, k);

    for (size_t i = 0; i < encrypted.size(); i += 8) {
        unsigned int v0 = (encrypted[i] << 24) | (encrypted[i+1] << 16) | (encrypted[i+2] << 8) | encrypted[i+3];
        unsigned int v1 = (encrypted[i+4] << 24) | (encrypted[i+5] << 16) | (encrypted[i+6] << 8) | encrypted[i+7];
        DecryptBlock(v0, v1, k);
        encrypted[i]     = (v0 >> 24) & 0xFF; encrypted[i+1] = (v0 >> 16) & 0xFF;
        encrypted[i+2]   = (v0 >> 8)  & 0xFF; encrypted[i+3] =  v0        & 0xFF;
        encrypted[i+4]   = (v1 >> 24) & 0xFF; encrypted[i+5] = (v1 >> 16) & 0xFF;
        encrypted[i+6]   = (v1 >> 8)  & 0xFF; encrypted[i+7] =  v1        & 0xFF;
    }

    if (!Unpad(encrypted))
        return false;

    size_t len = (encrypted.size() < outSize - 1) ? encrypted.size() : outSize - 1;
    std::memcpy(out, &encrypted[0], len);
    out[len] = '\0';

    return true;
}
