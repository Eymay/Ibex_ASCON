#include "api.h"
#include "endian.h"
#include "permutations.h"

#define RATE (128 / 8)
#define PA_ROUNDS 12
#define PB_ROUNDS 8
#define IV                                                        \
  ((u64)(8 * (CRYPTO_KEYBYTES)) << 56 | (u64)(8 * (RATE)) << 48 | \
   (u64)(PA_ROUNDS) << 40 | (u64)(PB_ROUNDS) << 32)

int crypto_aead_encrypt(unsigned char* c, unsigned long long* clen,
                        const unsigned char* m, unsigned long long mlen,
                        const unsigned char* ad, unsigned long long adlen,
                        const unsigned char* nsec, const unsigned char* npub,
                        const unsigned char* k) {
  u32_2 K0, K1, N0, N1;
  u32_2 x0, x1, x2, x3, x4;
  u32 s_o[5] = {x0.o, x1.o, x2.o, x3.o, x4.o};
  u32 s_e[5] = {x0.e, x1.e, x2.e, x3.e, x4.e};
  u32_2 t0, t1, t2, t3, t4;
  u64 tmp0, tmp1;
  u32 i;
  (void)nsec;
  //unsigned long cycle_end, cycle_start ;

  // set ciphertext size
  *clen = mlen + CRYPTO_ABYTES;

  // load key and nonce
  to_bit_interleaving(K0, U64BIG(*(u64*)k));
  to_bit_interleaving(K1, U64BIG(*(u64*)(k + 8)));
  to_bit_interleaving(N0, U64BIG(*(u64*)npub));
  to_bit_interleaving(N1, U64BIG(*(u64*)(npub + 8)));

  // initialization
  to_bit_interleaving(x0, IV);
  s_o[0] = x0.o;
  s_e[0] = x0.e; 
  s_o[1] = K0.o;
  s_e[1] = K0.e;
  s_e[2] = K1.e;
  s_o[2] = K1.o;
  s_e[3] = N0.e;
  s_o[3] = N0.o;
  s_e[4] = N1.e;
  s_o[4] = N1.o;
  P12();
  s_e[3] ^= K0.e;
  s_o[3] ^= K0.o;
  s_e[4] ^= K1.e;
  s_o[4] ^= K1.o;

  // process associated data
  if (adlen) {
    while (adlen >= RATE) {
      to_bit_interleaving(t0, U64BIG(*(u64*)ad));
      s_e[0] ^= t0.e;
      s_o[0] ^= t0.o;
      to_bit_interleaving(t1, U64BIG(*(u64*)(ad + 8)));
      s_e[1] ^= t1.e;
      s_o[1] ^= t1.o;
      P8();
      adlen -= RATE;
      ad += RATE;
    }
    tmp0 = 0;
    tmp1 = 0;
    for (i = 0; i < adlen; ++i, ++ad)
      if (i < 8)
        tmp0 ^= INS_BYTE64(*ad, i);
      else
        tmp1 ^= INS_BYTE64(*ad, i % 8);
    if (adlen < 8)
      tmp0 ^= INS_BYTE64(0x80, adlen);
    else
      tmp1 ^= INS_BYTE64(0x80, adlen % 8);
    to_bit_interleaving(t0, tmp0);
    s_e[0] ^= t0.e;
    s_o[0] ^= t0.o;
    to_bit_interleaving(t1, tmp1);
    s_e[1] ^= t1.e;
    s_o[1] ^= t1.o;
    P8();
  }
  s_e[4] ^= 1;

  // process plaintext
  while (mlen >= RATE) {
    to_bit_interleaving(t0, U64BIG(*(u64*)m));
    s_e[0] ^= t0.e;
    s_o[0] ^= t0.o;
    to_bit_interleaving(t1, U64BIG(*(u64*)(m + 8)));
    s_e[1] ^= t1.e;
    s_o[1] ^= t1.o;

    x0.e = s_e[0]; x0.o = s_o[0]; 
    from_bit_interleaving(tmp0, x0);
    *(u64*)c = U64BIG(tmp0);
    x1.e = s_e[1]; x1.o = s_o[1];
    from_bit_interleaving(tmp1, x1);
    *(u64*)(c + 8) = U64BIG(tmp1);
    P8();
    mlen -= RATE;
    m += RATE;
    c += RATE;
  }
  tmp0 = 0;
  tmp1 = 0;
  for (i = 0; i < mlen; ++i, ++m)
    if (i < 8)
      tmp0 ^= INS_BYTE64(*m, i);
    else
      tmp1 ^= INS_BYTE64(*m, i % 8);
  if (mlen < 8)
    tmp0 ^= INS_BYTE64(0x80, mlen);
  else
    tmp1 ^= INS_BYTE64(0x80, mlen % 8);
  to_bit_interleaving(t0, tmp0);
  s_e[0] ^= t0.e;
  s_o[0] ^= t0.o;
  to_bit_interleaving(t1, tmp1);
  s_e[1] ^= t1.e;
  s_o[1] ^= t1.o;

  x0.e = s_e[0]; x0.o = s_o[0];
  x1.e = s_e[1]; x1.o = s_o[1]; 
  from_bit_interleaving(tmp0, x0);
  from_bit_interleaving(tmp1, x1);
  for (i = 0; i < mlen; ++i, ++c)
    if (i < 8)
      *c = EXT_BYTE64(tmp0, i);
    else
      *c = EXT_BYTE64(tmp1, i % 8);

  // finalization
  s_e[2] ^= K0.e;
  s_o[2] ^= K0.o;
  s_e[3] ^= K1.e;
  s_o[3] ^= K1.o;
  P12();
  s_e[3] ^= K0.e;
  s_o[3] ^= K0.o;
  s_e[4] ^= K1.e;
  s_o[4] ^= K1.o;

  // set tag
  x3.e = s_e[3]; x3.o = s_o[3]; 
  from_bit_interleaving(tmp0, x3);
  *(u64*)c = U64BIG(tmp0);
  x4.e = s_e[4]; x4.o = s_o[4];
  from_bit_interleaving(tmp1, x4);
  *(u64*)(c + 8) = U64BIG(tmp1);

  return 0;
}




/* #include "api.h"
#include "endian.h"
#include "permutations.h"
#include <stdio.h>

#define RATE (128 / 8)
#define PA_ROUNDS 12
#define PB_ROUNDS 8
#define IV                                                        \
  ((u64)(8 * (CRYPTO_KEYBYTES)) << 56 | (u64)(8 * (RATE)) << 48 | \
   (u64)(PA_ROUNDS) << 40 | (u64)(PB_ROUNDS) << 32)

int crypto_aead_encrypt(unsigned char* c, int* clen,
                        const unsigned char* m, int mlen,
                        const unsigned char* ad, int adlen,
                        const unsigned char* nsec, const unsigned char* npub,
                        const unsigned char* k) {
  u32_2 K0, K1, N0, N1;
  u32_2 x0, x1, x2, x3, x4;
  u32 s_o[5] = {x0.o, x1.o, x2.o, x3.o, x4.o};
  u32 s_e[5] = {x0.e, x1.e, x2.e, x3.e, x4.e};
  u32_2 t0, t1, t2, t3, t4;
  u64 tmp0, tmp1;
  u32 i;
  (void)nsec;
  //unsigned long cycle_end, cycle_start ;

  // set ciphertext size
  *clen = mlen + CRYPTO_ABYTES;

  //Temp lines:

  // load key and nonce
  to_bit_interleaving(K0, U64BIG(*(u64*)k));
  to_bit_interleaving(K1, U64BIG(*(u64*)(k + 8)));
  to_bit_interleaving(N0, U64BIG(*(u64*)npub));
  to_bit_interleaving(N1, U64BIG(*(u64*)(npub + 8)));

  // initialization
  to_bit_interleaving(x0, IV);
  s_o[0] = x0.o;
  s_e[0] = x0.e;
  s_o[1] = K0.o;
  s_e[1] = K0.e;
  s_e[2] = K1.e;
  s_o[2] = K1.o;
  s_e[3] = N0.e;
  s_o[3] = N0.o;
  s_e[4] = N1.e;
  s_o[4] = N1.o;
  P12();
  s_e[3] ^= K0.e;
  s_o[3] ^= K0.o;
  s_e[4] ^= K1.e;
  s_o[4] ^= K1.o;

  // process associated data
  if (adlen) {
    while (adlen >= RATE) {
      to_bit_interleaving(t0, U64BIG(*(u64*)ad));
      s_e[0] ^= t0.e;
      s_o[0] ^= t0.o;
      to_bit_interleaving(t1, U64BIG(*(u64*)(ad + 8)));
      s_e[1] ^= t1.e;
      s_o[1] ^= t1.o;
      P8();
      adlen -= RATE;
      ad += RATE;
    }
    tmp0 = 0;
    tmp1 = 0;
    for (i = 0; i < adlen; ++i, ++ad)
      if (i < 8)
        tmp0 ^= INS_BYTE64(*ad, i);
      else
        tmp1 ^= INS_BYTE64(*ad, i % 8);
    if (adlen < 8)
      tmp0 ^= INS_BYTE64(0x80, adlen);
    else
      tmp1 ^= INS_BYTE64(0x80, adlen % 8);
    to_bit_interleaving(t0, tmp0);
    s_e[0] ^= t0.e;
    s_o[0] ^= t0.o;
    to_bit_interleaving(t1, tmp1);
    s_e[1] ^= t1.e;
    s_o[1] ^= t1.o;
    P8();
  }
  s_e[4] ^= 1;

  // process plaintext
  while (mlen >= RATE) {
    to_bit_interleaving(t0, U64BIG(*(u64*)m));
    s_e[0] ^= t0.e;
    s_o[0] ^= t0.o;
    to_bit_interleaving(t1, U64BIG(*(u64*)(m + 8)));
    s_e[1] ^= t1.e;
    s_o[1] ^= t1.o;

    x0.e = s_e[0]; x0.o = s_o[0]; 
    from_bit_interleaving(tmp0, x0);
    *(u64*)c = U64BIG(tmp0);
    x1.e = s_e[1]; x1.o = s_o[1];
    from_bit_interleaving(tmp1, x1);
    *(u64*)(c + 8) = U64BIG(tmp1);
    P8();
    mlen -= RATE;
    m += RATE;
    c += RATE;
  }
  tmp0 = 0;
  tmp1 = 0;
  for (i = 0; i < mlen; ++i, ++m) {
    if (i < 8)
      tmp0 ^= INS_BYTE64(*m, i);
    else
      tmp1 ^= INS_BYTE64(*m, i % 8);
  }
    
  if (mlen < 8)
    tmp0 ^= INS_BYTE64(0x80, mlen);
  else
    tmp1 ^= INS_BYTE64(0x80, mlen % 8);
  //Burada kaldım.
 to_bit_interleaving(t0, tmp0);
  s_e[0] ^= t0.e;
  s_o[0] ^= t0.o;
  to_bit_interleaving(t1, tmp1);
  s_e[1] ^= t1.e;
  s_o[1] ^= t1.o;

  x0.e = s_e[0]; x0.o = s_o[0];
  x1.e = s_e[1]; x1.o = s_o[1]; 
  from_bit_interleaving(tmp0, x0);
  from_bit_interleaving(tmp1, x1);
  for (i = 0; i < mlen; ++i, ++c) {
    if (i < 8)
      *c = EXT_BYTE64(tmp0, i);
    else
      *c = EXT_BYTE64(tmp1, i % 8);
  }
  // finalization
  s_e[2] ^= K0.e;
  s_o[2] ^= K0.o;
  s_e[3] ^= K1.e;
  s_o[3] ^= K1.o;
  P12();
  s_e[3] ^= K0.e;
  s_o[3] ^= K0.o;
  s_e[4] ^= K1.e;
  s_o[4] ^= K1.o;

  // set tag
  x3.e = s_e[3]; x3.o = s_o[3]; 
  from_bit_interleaving(tmp0, x3);
  *(u64*)c = U64BIG(tmp0);
  x4.e = s_e[4]; x4.o = s_o[4];
  from_bit_interleaving(tmp1, x4);
    *(u64*)(c + 8) = U64BIG(tmp1);
  return 0;
}
 */