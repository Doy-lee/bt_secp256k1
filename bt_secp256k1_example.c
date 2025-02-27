// A simple program that generates a secp256k1 cryptographic keypair
// and dumps it to standard out.

#define BT_SECP256K1_IMPLEMENTATION
// NOTE: The 2 following defines are optional! We provide a default value if
// these are not defined before the implementation
#define ENABLE_MODULE_ECDH
#define ENABLE_MODULE_RECOVERY
#define ENABLE_MODULE_EXTRAKEYS
#define ENABLE_MODULE_SCHNORRSIG
#define ENABLE_MODULE_ELLSWIFT
#define ENABLE_MODULE_MUSIG
#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE   15
#include "bt_secp256k1.h"

#include <Windows.h>
#include <stdio.h>

#if defined(_WIN32)
  #if defined(__cplusplus)
  extern "C" {
  #endif

  #define BCRYPT_RNG_ALGORITHM L"RNG"
  /*NTSTATUS*/ long __stdcall BCryptOpenAlgorithmProvider(void **phAlgorithm, wchar_t const *pszAlgId, wchar_t const *pszImplementation, unsigned long dwFlags);
  /*NTSTATUS*/ long __stdcall BCryptGenRandom(void *hAlgorithm, unsigned char *pbBuffer, unsigned long cbBuffer, unsigned long dwFlags);
  #pragma comment(lib, "bcrypt")

  #if defined(__cplusplus)
  }
  #endif
#else
  #include <sys/random.h>
#endif

static int Random32Bytes(void *buffer)
{
#if defined(_WIN32)
  static void *bcrypt_handle = NULL;
  if (!bcrypt_handle) {
    long init_status = BCryptOpenAlgorithmProvider(&bcrypt_handle, BCRYPT_RNG_ALGORITHM, NULL /*implementation*/, 0 /*flags*/);
    if (!bcrypt_handle || init_status != 0) {
      fprintf(stderr, "Failed to initialise random number generator [error=%ld]\n", init_status);
      return 0;
    }
  }

  long gen_status = BCryptGenRandom(bcrypt_handle, (unsigned char *)buffer, 32 /*size*/, 0 /*flags*/);
  if (gen_status != 0) {
    fprintf(stderr, "Failed to generate random bytes [error=%ld]\n", gen_status);
    return 0;
  }

#else
  int read_bytes = 0;
  do {
    read_bytes = getrandom(buffer, 32 /*size*/, 0);
  } while (read_bytes != size || errno == EAGAIN); // NOTE: EINTR can not be triggered if size <= 32 bytes
#endif

  return 1;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  // NOTE: Initialise the library
  secp256k1_context *lib_context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

  // NOTE: Generate a public key
  unsigned char skey[32];
  if (!Random32Bytes(skey)) {
    fprintf(stderr, "Failed to generate secret key\n");
    return -1;
  }

  secp256k1_pubkey pkey;
  if (secp256k1_ec_pubkey_create(lib_context, &pkey, skey) == 1) {
    // NOTE: Serialize the key
    unsigned char pkey_serialized[33];
    size_t        pkey_serialized_size = sizeof(pkey_serialized);
    secp256k1_ec_pubkey_serialize(lib_context, pkey_serialized, &pkey_serialized_size, &pkey, SECP256K1_EC_COMPRESSED);

    // NOTE: Dump the secret key to stdout
    fprintf(stdout, "Secret Key [key=0x");
    for (size_t index = 0; index < sizeof(skey); index++) {
      fprintf(stdout, "%x", (skey[index] >> 4) & 0xF);
      fprintf(stdout, "%x", (skey[index] >> 0) & 0xF);
    }
    fprintf(stdout, "]\n");

    // NOTE: Dump the serialized public key to stdout
    fprintf(stdout, "Public Key [key=0x");
    for (size_t index = 0; index < pkey_serialized_size; index++) {
      fprintf(stdout, "%x", (pkey_serialized[index] >> 4) & 0xF);
      fprintf(stdout, "%x", (pkey_serialized[index] >> 0) & 0xF);
    }
    fprintf(stdout, "]\n");
  } else {
    fprintf(stderr, "Failed to generate public key");
  }

  return 0;
}
