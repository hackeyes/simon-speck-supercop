#include <openssl/rand.h>
#define crypto_sign_ronald768_openssl_MODULUSBITS 768 
#define crypto_sign_ronald768_openssl_MODULUSBYTES (crypto_sign_ronald768_openssl_MODULUSBITS / 8)

#define crypto_sign_ronald768_openssl_SECRETKEYBYTES (crypto_sign_ronald768_openssl_MODULUSBYTES * 8)
#define crypto_sign_ronald768_openssl_PUBLICKEYBYTES (crypto_sign_ronald768_openssl_MODULUSBYTES)
#define crypto_sign_ronald768_openssl_BYTES (crypto_sign_ronald768_openssl_MODULUSBYTES)