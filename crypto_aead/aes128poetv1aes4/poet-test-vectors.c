/*
// @author Eik List
// @last-modified 2015-08-07
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
*/
#include <stdio.h>
#include <string.h>
#include "poet.h"
#include "api.h"

// ---------------------------------------------------------------------

static void print_hex(const char *message, 
                      const unsigned char *x, 
                      const int len)
{
    int i;
    puts(message);

    for (i = 0; i < len; i++) {
        if ((i != 0) && (i % 16 == 0)) {
            puts("");
        }
        printf("%02x ", x[i]);
    }

    printf("     %d (octets)\n\n", len);
}

// ---------------------------------------------------------------------

#ifdef NI_ENABLED
#include <emmintrin.h>

// ---------------------------------------------------------------------

static void store(const unsigned char* p, __m128i x)
{
    _mm_store_si128((__m128i*)p, x);
}

// ---------------------------------------------------------------------

static void print128(char* label, __m128i var)
{
    unsigned char val[BLOCKLEN];
    store(val, var);
    printf("%s\n", label);
    int i;

    for (i = 0; i < BLOCKLEN; ++i) {
        printf("%02x ", val[i]);
    }

    puts("\n");
}

// ---------------------------------------------------------------------

static void print_context(const poet_ctx_t *ctx)
{
    print128("K:   ", ctx->aes_enc[0]);
    print128("L:   ", ctx->l);
    print128("L_Top:", ctx->aes_lt[0]);
    print128("L_Bot:", ctx->aes_lb[0]);
    print128("L_T:", ctx->tm);
    print128("Tau: ", ctx->tau);
}

#else

static void print_context(const poet_ctx_t *ctx)
{
    print_hex("K:", ctx->k, BLOCKLEN);
    print_hex("L:", ctx->l, BLOCKLEN);
    print_hex("L_Top:", ctx->lt, BLOCKLEN);
    print_hex("L_Bot:", ctx->lb, BLOCKLEN);
    print_hex("L_T:", ctx->tm, BLOCKLEN);
    print_hex("Tau:", ctx->tau, TAGLEN);
}

#endif 

// ---------------------------------------------------------------------

static void test_output(const poet_ctx_t *ctx,
                        const unsigned char *k, const unsigned long long klen,
                        const unsigned char *h, const unsigned long long hlen,
                        const unsigned char *m, const unsigned long long mlen,
                        const unsigned char *c, const unsigned long long clen,
                        const unsigned char *t, const unsigned long long tlen)

{
    print_hex("SK: ", k, klen);
    print_context(ctx);
    print_hex("Header/Nonce: ", h, hlen);
    print_hex("Plaintext:", m, mlen);
    print_hex("Ciphertext:", c, clen);
    print_hex("Tag:", t, tlen);
    puts("\n\n");
}

// ---------------------------------------------------------------------

static int run_test(const unsigned char *k,
                    const unsigned char *h,
                    const unsigned long long hlen,
                    const unsigned char *expected_m,
                    unsigned long long mlen,
                    const unsigned char *expected_c,
                    const unsigned char *expected_t)
{
    poet_ctx_t ctx;
    unsigned char* c = (unsigned char*)malloc((size_t)mlen);
    unsigned char* m = (unsigned char*)malloc((size_t)mlen);
    unsigned long long clen = mlen;
    unsigned char t[TAGLEN];

    keysetup(&ctx, k);
    process_header(&ctx, h, hlen);
    encrypt_final(&ctx, expected_m, mlen, c, t);

    if (memcmp(expected_c, c, clen) || memcmp(expected_t, t, TAGLEN)) {
        test_output(&ctx, k, KEYLEN, h, hlen, expected_m, mlen, c, clen, t, TAGLEN);
        puts("Encryption produced incorrect result");
        free(m);
        free(c);
        return -1;
    }

    keysetup(&ctx, k);
    process_header(&ctx, h, hlen);

    const int result = decrypt_final(&ctx, c, clen, t, m);
    test_output(&ctx, k, KEYLEN, h, hlen, m, mlen, c, clen, t, TAGLEN);

    if (memcmp(expected_m, m, mlen)) {
        puts("Decryption produced incorrect result");
        free(m);
        free(c);
        return -1;
    }

    if (result != 0) {
        puts("Verification failed");
    }
    
    free(m);
    free(c);
    return result;
}

// ---------------------------------------------------------------------

static int test1()
{
    unsigned long long mlen = BLOCKLEN;
    const unsigned long long hlen = 0;
    const unsigned char k[KEYLEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    const unsigned char m[BLOCKLEN] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    const unsigned char c[BLOCKLEN] = {
        0x88, 0x68, 0x17, 0x90, 0x68, 0xb2, 0xee, 0x6c, 
        0x60, 0x7b, 0x9e, 0xad, 0xe4, 0x24, 0x84, 0xa4
    };
    const unsigned char t[TAGLEN] = {
        0x13, 0x99, 0x00, 0x8f, 0x05, 0xa1, 0x9f, 0x31, 
        0xb6, 0x31, 0xc9, 0x1b, 0x36, 0xc1, 0x82, 0x3a
    };
    return run_test(k, NULL, hlen, m, mlen, c, t);
}

// ---------------------------------------------------------------------

static int test2()
{
    unsigned long long mlen = 0;
    const unsigned long long hlen = 24;
    const unsigned char k[KEYLEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    const unsigned char h[24] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xaf, 0xba, 0xbe
    };
    const unsigned char t[TAGLEN] = {
        0x1b, 0x8f, 0xa8, 0xb9, 0x60, 0xe3, 0x1e, 0x1d, 
        0x9c, 0x1c, 0x9d, 0xbb, 0x68, 0x19, 0x95, 0x22
    };
    return run_test(k, h, hlen, NULL, mlen, NULL, t);
}

// ---------------------------------------------------------------------

static int test3()
{
    unsigned long long mlen = 8;
    const unsigned long long hlen = BLOCKLEN;
    const unsigned char k[KEYLEN] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    const unsigned char h[BLOCKLEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    const unsigned char m[8] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    const unsigned char c[8] = {
        0xa9, 0x7b, 0xbe, 0xff, 0xda, 0xcb, 0x54, 0x5e 
    };
    const unsigned char t[TAGLEN] = {
        0x48, 0xf3, 0xf6, 0xc6, 0xc1, 0x00, 0x36, 0xd2, 
        0xf4, 0x47, 0x74, 0x8b, 0xc1, 0xf7, 0x22, 0x90
    };
    return run_test(k, h, hlen, m, mlen, c, t);
}

// ---------------------------------------------------------------------

static int test4()
{
    unsigned long long mlen = 56;
    const unsigned long long hlen = BLOCKLEN;
    const unsigned char k[KEYLEN] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    const unsigned char h[BLOCKLEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    };
    const unsigned char m[56] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xaf, 0xba, 0xbe
    };
    const unsigned char c[56] = {
        0x96, 0xfd, 0xaf, 0x32, 0xce, 0xe3, 0xbf, 0x28, 
        0x06, 0x1c, 0xea, 0x84, 0xbb, 0xb3, 0xbe, 0x8f, 
        0x65, 0xfa, 0x88, 0x4e, 0xe6, 0x11, 0x5a, 0xc2, 
        0x1f, 0xe0, 0xf0, 0xac, 0x9e, 0x64, 0xd7, 0x74, 
        0x94, 0xc8, 0x18, 0x28, 0xbc, 0x5a, 0x16, 0x10, 
        0xc2, 0x7d, 0xdc, 0x5a, 0x29, 0x0d, 0xb8, 0x83, 
        0x35, 0x86, 0x15, 0x18, 0xc5, 0x4e, 0x0a, 0xb9
    };
    const unsigned char t[TAGLEN] = {
        0x74, 0xe0, 0x96, 0xc1, 0x6d, 0xf5, 0x48, 0xe5, 
        0x82, 0xbc, 0x49, 0xe6, 0x35, 0xe1, 0x26, 0x6f
    };
    return run_test(k, h, hlen, m, mlen, c, t);
}

// ---------------------------------------------------------------------

static int test5()
{
    unsigned long long mlen = 52;
    const unsigned long long hlen = 24;
    const unsigned char k[KEYLEN] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    const unsigned char h[24] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xaf, 0xba, 0xbe
    };
    const unsigned char m[52] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0xfe, 0xfe, 0xba, 0xbe
    };
    const unsigned char c[52] = {
        0x73, 0x01, 0x02, 0x1f, 0x92, 0x64, 0x34, 0xa8, 
        0xdf, 0x06, 0xe1, 0x83, 0x97, 0x92, 0x54, 0xb7, 
        0xc4, 0x35, 0x25, 0xca, 0x30, 0xca, 0xfb, 0x1c, 
        0xb8, 0xb5, 0xcc, 0x25, 0xfc, 0x05, 0xcd, 0x2c, 
        0x7f, 0x6f, 0x0a, 0x4d, 0xd3, 0x6a, 0x6f, 0x9a, 
        0x1c, 0x46, 0x24, 0x61, 0xca, 0x74, 0xec, 0x1c, 
        0x7a, 0x26, 0x7d, 0x85
    };
    const unsigned char t[TAGLEN] = {
        0x86, 0x59, 0x63, 0x94, 0x3c, 0x62, 0x96, 0xf3, 
        0x61, 0xf8, 0x14, 0x7a, 0x34, 0x8e, 0xe7, 0x61
    };
    return run_test(k, h, hlen, m, mlen, c, t);
}

// ---------------------------------------------------------------------

static int test6()
{
    const char k[] = "Edgar Allan Poe.";
    const char h[] = "\"Seldom we find,\" says Solomon Don Dunce,\n\"Half an idea in the profoundest sonnet.\nThrough all the flimsy things we see at once\nAs easily as through a Naples bonnet-\nTrash of all trash!- how can a lady don it?\nYet heavier far than your Petrarchan stuff-\nOwl-downy nonsense that the faintest puff\nTwirls into trunk-paper the while you con it.\"\nAnd, veritably, Sol is right enough.\nThe general tuckermanities are arrant\nBubbles- ephemeral and so transparent-\nBut this is, now- you may depend upon it-\nStable, opaque, immortal- all by dint\nOf the dear names that he concealed within 't.";
    char m[] = "The noblest name in Allegory's page,\nThe hand that traced inexorable rage;\nA pleasing moralist whose page refined,\nDisplays the deepest knowledge of the mind;\nA tender poet of a foreign tongue,\n(Indited in the language that he sung.)\nA bard of brilliant but unlicensed page\nAt once the shame and glory of our age,\nThe prince of harmony and stirling sense,\nThe ancient dramatist of eminence,\nThe bard that paints imagination's powers,\nAnd him whose song revives departed hours,\nOnce more an ancient tragic bard recall,\nIn boldness of design surpassing all.\nThese names when rightly read, a name [make] known\nWhich gathers all their glories in its own.";

    const unsigned long long hlen = (unsigned long long)strlen(h);
    unsigned long long mlen = (unsigned long long)strlen(m);
    const unsigned char c[650] = {
        0x19, 0xc8, 0xc1, 0xba, 0x64, 0xd1, 0xeb, 0xd5, 
        0xa5, 0x84, 0x8f, 0xf6, 0x1e, 0x2d, 0xc8, 0x57, 
        0x80, 0x85, 0xa1, 0x0f, 0xdd, 0x2b, 0x03, 0x22, 
        0x69, 0x3b, 0xce, 0xbe, 0x96, 0x52, 0x63, 0xb7, 
        0xae, 0x31, 0x14, 0x9e, 0x46, 0x45, 0x7e, 0x37, 
        0x22, 0x67, 0x82, 0x6c, 0x59, 0x1e, 0xef, 0x2c, 
        0xf4, 0xc7, 0x0d, 0xf9, 0xb8, 0x51, 0x43, 0x8a, 
        0x87, 0x35, 0x1a, 0x5f, 0xaf, 0xae, 0xb4, 0xac, 
        0x83, 0xa2, 0x64, 0x4d, 0x7b, 0x42, 0x44, 0xb6, 
        0x03, 0x1a, 0x8e, 0x16, 0x88, 0xc7, 0xb0, 0x36, 
        0x13, 0xca, 0x3a, 0x28, 0x12, 0x94, 0x6d, 0x14, 
        0x91, 0x9e, 0x0b, 0x3e, 0x13, 0xe9, 0x2d, 0xae, 
        0xa9, 0x0d, 0xb9, 0x91, 0xb9, 0x8c, 0x85, 0xda, 
        0x5a, 0xb8, 0x74, 0xca, 0xdf, 0x90, 0x30, 0x2f, 
        0xcf, 0xdb, 0x9f, 0x3c, 0x7f, 0xc6, 0x50, 0x18, 
        0x12, 0x4d, 0x57, 0x34, 0x6a, 0x99, 0xba, 0x0c, 
        0xd4, 0x70, 0xed, 0x38, 0x37, 0x66, 0x6c, 0xbb, 
        0x64, 0x04, 0x9a, 0x10, 0x7d, 0xa0, 0x29, 0x74, 
        0x00, 0xa7, 0x10, 0x9d, 0xec, 0x45, 0x88, 0x65, 
        0x64, 0x4e, 0x24, 0xb4, 0x1c, 0xf5, 0xe5, 0xbb, 
        0x18, 0xa5, 0xca, 0x42, 0x6a, 0x67, 0xe2, 0x19, 
        0x41, 0x0f, 0x91, 0x29, 0x80, 0xc6, 0x77, 0xa4, 
        0x4e, 0x16, 0xb0, 0x0a, 0x68, 0xa1, 0xa4, 0xd1, 
        0x2e, 0xce, 0x3d, 0x76, 0x8d, 0x84, 0x62, 0xef, 
        0x5c, 0xed, 0xae, 0x22, 0x0f, 0xe6, 0x60, 0xc4, 
        0x17, 0xdc, 0x04, 0xb2, 0x70, 0x71, 0x6e, 0x68, 
        0x2b, 0x3a, 0xde, 0x8a, 0xb3, 0xb9, 0x5d, 0xcc, 
        0xfa, 0xc8, 0xdc, 0x18, 0xdb, 0xb1, 0xf3, 0xcb, 
        0xcf, 0x44, 0x70, 0xee, 0x6c, 0x13, 0x06, 0x9f, 
        0x3f, 0x8e, 0xc1, 0xa2, 0x7c, 0xb4, 0x98, 0xaf, 
        0x85, 0x16, 0x70, 0x2c, 0x9f, 0x58, 0x93, 0xb7, 
        0xdc, 0xb1, 0x14, 0xa1, 0x8b, 0xee, 0xb9, 0xbf, 
        0x43, 0xc7, 0x8a, 0x0d, 0xd3, 0x2d, 0xf0, 0x12, 
        0x1c, 0x91, 0xea, 0x07, 0xfb, 0x60, 0x77, 0x9f, 
        0x44, 0x13, 0xd9, 0x4a, 0xd7, 0x10, 0xaf, 0x8d, 
        0xc2, 0x89, 0x15, 0x8e, 0xf3, 0x16, 0x06, 0xa8, 
        0x84, 0xdc, 0x89, 0xfb, 0xe8, 0xa4, 0x9e, 0x93, 
        0x69, 0xa2, 0x83, 0xfe, 0x62, 0xa6, 0x0d, 0x9e, 
        0xc9, 0xb7, 0x46, 0xea, 0x05, 0x5c, 0xaa, 0xa9, 
        0x8a, 0x26, 0x71, 0x01, 0xd4, 0xca, 0xec, 0xac, 
        0x9d, 0x97, 0xcc, 0xa3, 0xa7, 0x74, 0xe7, 0x5a, 
        0xf6, 0xd3, 0xd5, 0x22, 0x36, 0xd4, 0x25, 0x07, 
        0xe4, 0xa8, 0xc3, 0x2e, 0x5a, 0x43, 0xaf, 0x1f, 
        0xbb, 0xef, 0x29, 0x7c, 0xed, 0xd2, 0x4c, 0x4a, 
        0x26, 0xce, 0x21, 0xf0, 0x87, 0x76, 0xcc, 0x11, 
        0x4c, 0x65, 0x4d, 0xc1, 0x8a, 0xdc, 0x3d, 0x45, 
        0x46, 0x3a, 0xe4, 0x88, 0x4b, 0xa6, 0x6e, 0x63, 
        0x0d, 0xc5, 0x5a, 0x9d, 0xb0, 0x91, 0xe4, 0x1f, 
        0xaf, 0xa7, 0x78, 0x60, 0x2c, 0xfe, 0x74, 0x22, 
        0x04, 0x13, 0xcf, 0x44, 0xd2, 0xc3, 0xc4, 0xd3, 
        0x2e, 0x24, 0xe8, 0x3c, 0x2b, 0xf2, 0xd7, 0x20, 
        0xb2, 0x1d, 0xe2, 0x9f, 0x2a, 0xd0, 0x66, 0x71, 
        0xb8, 0x30, 0x12, 0xb4, 0x28, 0xa7, 0x4b, 0x95, 
        0x1d, 0xb1, 0xa0, 0xff, 0x2d, 0xee, 0x21, 0xfd, 
        0x63, 0x70, 0x79, 0x0c, 0xfc, 0x62, 0x04, 0x04, 
        0x3c, 0xc1, 0xbb, 0x25, 0xc2, 0x86, 0xd4, 0x48, 
        0x99, 0xad, 0x0c, 0x4f, 0x0b, 0x7e, 0x49, 0x20, 
        0x9f, 0xc2, 0x6a, 0x32, 0x1a, 0xe5, 0x7d, 0x80, 
        0xc1, 0xc9, 0x59, 0x38, 0x44, 0x0a, 0x06, 0x26, 
        0x1e, 0xa1, 0x71, 0xfb, 0x13, 0x02, 0x19, 0x8b, 
        0x6e, 0xeb, 0x1c, 0x96, 0x6f, 0x72, 0xa1, 0x35, 
        0xb4, 0x3d, 0xa4, 0xaf, 0x35, 0x8a, 0xe3, 0xea, 
        0x0e, 0xe6, 0x5f, 0x3a, 0x86, 0xa3, 0xec, 0x71, 
        0xff, 0xd7, 0x48, 0xe9, 0x09, 0xb9, 0x5c, 0x20, 
        0x1b, 0xde, 0x6f, 0xb2, 0x7a, 0xd4, 0x5e, 0xcb, 
        0xe1, 0x9a, 0x69, 0x8f, 0x58, 0x58, 0xe3, 0x8f, 
        0x5b, 0x65, 0xf9, 0x0b, 0x70, 0x33, 0x4f, 0x38, 
        0xb3, 0xfa, 0xdd, 0xef, 0x2d, 0xdc, 0x6f, 0xcf, 
        0x68, 0x3d, 0x43, 0x13, 0x82, 0x83, 0x56, 0xde, 
        0xb8, 0x63, 0x85, 0x61, 0x24, 0xe8, 0x8e, 0x17, 
        0xc6, 0x4e, 0x6f, 0xc2, 0xa6, 0xc8, 0xfe, 0xa6, 
        0xf9, 0x92, 0x06, 0x2e, 0x14, 0x11, 0xcf, 0x39, 
        0xa3, 0xc6, 0x43, 0x3e, 0x6f, 0x97, 0x1d, 0x95, 
        0x6f, 0x52, 0x7c, 0x76, 0xb7, 0xbb, 0xb0, 0x4c, 
        0x2b, 0x94, 0xb5, 0x6a, 0xea, 0x48, 0x93, 0x42, 
        0x18, 0x3e, 0x5e, 0xc3, 0x53, 0xda, 0x2a, 0x5f, 
        0x52, 0xaf, 0xe1, 0xb0, 0xa6, 0x04, 0x97, 0x59, 
        0x2a, 0xc6, 0x52, 0xb8, 0xdd, 0x1f, 0x66, 0x7c, 
        0x68, 0x3a, 0xe7, 0x06, 0xfc, 0xb0, 0xad, 0x02, 
        0x86, 0xd0, 0xd3, 0x94, 0x06, 0x51, 0x61, 0xcb, 
        0x4d, 0x63, 0x2a, 0xfd, 0xbf, 0xd5, 0x90, 0x26, 
        0x91, 0x8a
    };
    const unsigned char t[TAGLEN] = {
        0x1a, 0x75, 0xa2, 0xc8, 0x57, 0xe7, 0xeb, 0xdd, 
        0x9e, 0xa1, 0x26, 0xdb, 0x58, 0xe3, 0xee, 0x59
    };
    return run_test((const unsigned char*)k, (const unsigned char*)h, hlen,
                    (unsigned char*)m, mlen, c, t);
}

// ---------------------------------------------------------------------

int main()
{
    int result = 0;
    result |= test1();
    result |= test2();
    result |= test3();
    result |= test4();
    result |= test5();
    result |= test6();

    if (result) {
        puts("Test result:  FAILED");
    } else {
        puts("Tests result: SUCCESS");
    }

    return 0;
}
