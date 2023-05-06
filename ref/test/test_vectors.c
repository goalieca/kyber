/* Deterministic randombytes by Daniel J. Bernstein */
/* taken from SUPERCOP (https://bench.cr.yp.to)     */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../kem.h"
#include "../randombytes.h"
#include "../fips202.h"

#define NTESTS 10000

static uint64_t shake128_state[25] = {0x1F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, (1ULL << 63)};
static uint64_t shake128_offset = SHAKE128_RATE;

void randombytes(uint8_t *x,size_t xlen)
{
  while(xlen>0) {
    while(shake128_offset<SHAKE128_RATE && xlen>0){
      *x++ = shake128_state[shake128_offset/8] >> 8*(shake128_offset%8);
      shake128_offset++;
      xlen--;
    }
    if(shake128_offset==SHAKE128_RATE) {
      KeccakF1600_StatePermute(shake128_state);
      shake128_offset = 0;
    }
  }
}

int main(void)
{
  unsigned int i,j;
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
  uint8_t key_a[CRYPTO_BYTES];
  uint8_t key_b[CRYPTO_BYTES];

  for(i=0;i<NTESTS;i++) {
    // Key-pair generation
    crypto_kem_keypair(pk, sk);
    printf("Public Key: ");
    for(j=0;j<CRYPTO_PUBLICKEYBYTES;j++)
      printf("%02x",pk[j]);
    printf("\n");
    printf("Secret Key: ");
    for(j=0;j<CRYPTO_SECRETKEYBYTES;j++)
      printf("%02x",sk[j]);
    printf("\n");

    // Encapsulation
    crypto_kem_enc(ct, key_b, pk);
    printf("Ciphertext: ");
    for(j=0;j<CRYPTO_CIPHERTEXTBYTES;j++)
      printf("%02x",ct[j]);
    printf("\n");
    printf("Shared Secret B: ");
    for(j=0;j<CRYPTO_BYTES;j++)
      printf("%02x",key_b[j]);
    printf("\n");

    // Decapsulation
    crypto_kem_dec(key_a, ct, sk);
    printf("Shared Secret A: ");
    for(j=0;j<CRYPTO_BYTES;j++)
      printf("%02x",key_a[j]);
    printf("\n");

    for(j=0;j<CRYPTO_BYTES;j++) {
      if(key_a[j] != key_b[j]) {
        fprintf(stderr, "ERROR\n");
        return -1;
      }
    }

    // Decapsulation of invalid (random) ciphertexts
    randombytes(ct, KYBER_CIPHERTEXTBYTES); 
    crypto_kem_dec(key_a, ct, sk);
    printf("Pseudorandom shared Secret A: ");
    for(j=0;j<CRYPTO_BYTES;j++)
      printf("%02x",key_a[j]);
    printf("\n");
  }

  return 0;
}