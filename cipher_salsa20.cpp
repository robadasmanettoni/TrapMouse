#include "cipher_salsa20.h"
#include "base64.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>

typedef unsigned char u8;
typedef unsigned int  u32;

static const u8 key[32] = {
    '0','x','A','5','B','C','3','F',
    'T','r','p','M','s','_','K','e',
    'Y','_','K','d','A','1','9','0', 
    'X','_','S','t','a','b','l','e' 
};

static inline u32 ROTL(u32 v, int c) {
    return (v << c) | (v >> (32 - c));
}

static void U32TO8_LE(u8* p, u32 v) {
    p[0] = (u8)(v);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}

static u32 U8TO32_LE(const u8* p) {
    return ((u32)p[0]) |
           ((u32)p[1] << 8) |
           ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static void salsa20_block(const u8 nonce[8], u32 counter, u8 output[64]) {
    static const u8 sigma[16] = {
        'e','x','p','a','n','d',' ','3','2','-','b','y','t','e',' ','k'
    };
    u32 x[16], in[16];
    for (int i = 0; i < 16; ++i) {
        if      (i < 1)  in[i] = U8TO32_LE(sigma + 0);
        else if (i < 5)  in[i] = U8TO32_LE(key   + (i-1)*4);
        else if (i < 6)  in[i] = U8TO32_LE(sigma + 4);
        else if (i < 8)  in[i] = U8TO32_LE(nonce + (i-6)*4);
        else if (i == 8) in[i] = counter;
        else if (i == 9) in[i] = 0;
        else if (i < 11) in[i] = U8TO32_LE(sigma + 8);
        else             in[i] = U8TO32_LE(key   + (i-11)*4);
        x[i] = in[i];
    }
    for (int round = 0; round < 10; ++round) {
        x[ 4] ^= ROTL(x[ 0]+x[12], 7);
        x[ 8] ^= ROTL(x[ 4]+x[ 0], 9);
        x[12] ^= ROTL(x[ 8]+x[ 4],13);
        x[ 0] ^= ROTL(x[12]+x[ 8],18);

        x[ 9] ^= ROTL(x[ 5]+x[ 1], 7);
        x[13] ^= ROTL(x[ 9]+x[ 5], 9);
        x[ 1] ^= ROTL(x[13]+x[ 9],13);
        x[ 5] ^= ROTL(x[ 1]+x[13],18);

        x[14] ^= ROTL(x[10]+x[ 6], 7);
        x[ 2] ^= ROTL(x[14]+x[10], 9);
        x[ 6] ^= ROTL(x[ 2]+x[14],13);
        x[10] ^= ROTL(x[ 6]+x[ 2],18);

        x[ 3] ^= ROTL(x[15]+x[11], 7);
        x[ 7] ^= ROTL(x[ 3]+x[15], 9);
        x[11] ^= ROTL(x[ 7]+x[ 3],13);
        x[15] ^= ROTL(x[11]+x[ 7],18);

        x[ 1] ^= ROTL(x[ 0]+x[ 3], 7);
        x[ 2] ^= ROTL(x[ 1]+x[ 0], 9);
        x[ 3] ^= ROTL(x[ 2]+x[ 1],13);
        x[ 0] ^= ROTL(x[ 3]+x[ 2],18);

        x[ 6] ^= ROTL(x[ 5]+x[ 4], 7);
        x[ 7] ^= ROTL(x[ 6]+x[ 5], 9);
        x[ 4] ^= ROTL(x[ 7]+x[ 6],13);
        x[ 5] ^= ROTL(x[ 4]+x[ 7],18);

        x[11] ^= ROTL(x[10]+x[ 9], 7);
        x[ 8] ^= ROTL(x[11]+x[10], 9);
        x[ 9] ^= ROTL(x[ 8]+x[11],13);
        x[10] ^= ROTL(x[ 9]+x[ 8],18);

        x[12] ^= ROTL(x[15]+x[14], 7);
        x[13] ^= ROTL(x[12]+x[15], 9);
        x[14] ^= ROTL(x[13]+x[12],13);
        x[15] ^= ROTL(x[14]+x[13],18);
    }
    for (int i = 0; i < 16; ++i)
        U32TO8_LE(output + 4*i, x[i] + in[i]);
}

bool SavePassword_SALSA20(const char* plaintext, std::vector<unsigned char>& out) {
    if (!plaintext) return false;

    std::srand((unsigned)std::time(NULL));
    u8 nonce[8];
    for (int i = 0; i < 8; ++i)
        nonce[i] = static_cast<u8>(std::rand() & 0xFF);

    std::vector<u8> input(plaintext, plaintext + std::strlen(plaintext));
    std::vector<u8> output(input.size());

    for (size_t i = 0; i < input.size(); i += 64) {
        u8 block[64];
        salsa20_block(nonce, static_cast<u32>(i / 64), block);
        size_t chunk = std::min(input.size() - i, size_t(64));
        for (size_t j = 0; j < chunk; ++j)
            output[i + j] = input[i + j] ^ block[j];
    }

    // Costruisci risultato: nonce (8 byte) + dati cifrati
    out.resize(8 + output.size());
    std::memcpy(&out[0], nonce, 8);
    std::memcpy(&out[8], &output[0], output.size());

    return true;
}

bool LoadPassword_SALSA20(const std::string& base64, char* out, size_t outSize) {
    if (base64.empty() || !out || outSize == 0) return false;

    std::vector<unsigned char> data;
	std::string decoded = DecodeBase64(base64, data);
    if (decoded.empty()) return false;

    if (data.size() < 8) return false;

    u8 nonce[8];
    std::memcpy(nonce, &data[0], 8);
    size_t dataLen = data.size() - 8;

    std::vector<u8> ciphertext(data.begin() + 8, data.end());
    std::vector<u8> plaintext(dataLen);

    for (size_t i = 0; i < dataLen; i += 64) {
        u8 block[64];
        salsa20_block(nonce, static_cast<u32>(i / 64), block);
        size_t chunk = (dataLen - i < 64) ? dataLen - i : 64;
        for (size_t j = 0; j < chunk; ++j) {
            plaintext[i + j] = ciphertext[i + j] ^ block[j];
        }
    }

    size_t len = (plaintext.size() < outSize - 1) ? plaintext.size() : outSize - 1;
    std::memcpy(out, &plaintext[0], len);
    out[len] = '\0';

    return true;
}

