/*
  Testing bignum_from_string and bignum_from_int 

  Also testing bignum_cmp

*/


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bn.h"



int main(int argc, _Array_ptr<_Nt_array_ptr<char>> argv : count(argc))
{
  char sabuf _Nt_checked[8193];
  char sbbuf _Nt_checked[8193];
  char scbuf _Nt_checked[8193];
  char sdbuf _Nt_checked[8193];
  char iabuf _Nt_checked[8193];
  char ibbuf _Nt_checked[8193];
  char icbuf _Nt_checked[8193];
  char idbuf _Nt_checked[8193];

  struct bn sa = {};
struct bn sb = {};
struct bn sc = {};
struct bn sd = {};
struct bn se = {};
  struct bn ia = {};
struct bn ib = {};
struct bn ic = {};
struct bn id = {};

  bignum_init(&sa);
  bignum_init(&sb);
  bignum_init(&sc);
  bignum_init(&sd);
  bignum_init(&ia);
  bignum_init(&ib);
  bignum_init(&ic);
  bignum_init(&id);

  printf("\nLoading numbers from strings and from int.\n");
  
  bignum_from_string(&sa, "000000FF", 8);
  bignum_from_string(&sb, "0000FF00", 8);
  bignum_from_string(&sc, "00FF0000", 8);
  bignum_from_string(&sd, "FF000000", 8);
 
  bignum_from_int(&ia, 0x000000FF);
  bignum_from_int(&ib, 0x0000FF00);
  bignum_from_int(&ic, 0x00FF0000);
  bignum_from_int(&id, 0xFF000000);

  printf("Verifying comparison function.\n");

  assert(bignum_cmp(&ia, &ib) == SMALLER);
  assert(bignum_cmp(&ia, &ic) == SMALLER);
  assert(bignum_cmp(&ia, &id) == SMALLER);

  assert(bignum_cmp(&ib, &ia) == LARGER);
  assert(bignum_cmp(&ic, &ia) == LARGER);
  assert(bignum_cmp(&id, &ia) == LARGER);

  assert(bignum_cmp(&sa, &sb) == SMALLER);
  assert(bignum_cmp(&sa, &sc) == SMALLER);
  assert(bignum_cmp(&sa, &sd) == SMALLER);

  assert(bignum_cmp(&sb, &sa) == LARGER);
  assert(bignum_cmp(&sc, &sa) == LARGER);
  assert(bignum_cmp(&sd, &sa) == LARGER);

  assert(bignum_cmp(&ia, &sa) == EQUAL);
  assert(bignum_cmp(&ib, &sb) == EQUAL);
  assert(bignum_cmp(&ic, &sc) == EQUAL);
  assert(bignum_cmp(&id, &sd) == EQUAL);

  printf("Verifying to_string function.\n");

  bignum_to_string(&sa, sabuf, sizeof(sabuf)-1);
  bignum_to_string(&sb, sbbuf, sizeof(sbbuf)-1);
  bignum_to_string(&sc, scbuf, sizeof(scbuf)-1);
  bignum_to_string(&sd, sdbuf, sizeof(sdbuf)-1);

  bignum_to_string(&ia, iabuf, sizeof(iabuf)-1);
  bignum_to_string(&ib, ibbuf, sizeof(ibbuf)-1);
  bignum_to_string(&ic, icbuf, sizeof(icbuf)-1);
  bignum_to_string(&id, idbuf, sizeof(idbuf)-1);

  assert(strcmp(sabuf, iabuf) == 0);
  assert(strcmp(sbbuf, ibbuf) == 0);
  assert(strcmp(scbuf, icbuf) == 0);
  assert(strcmp(sdbuf, idbuf) == 0);

  printf("Verifying increment + decrement functions.\n");

  bignum_init(&sd);
  int i;
  for (i = 0; i < 255; ++i)
  {
    bignum_inc(&sd);
    assert(!bignum_is_zero(&sd));
  }

  assert(bignum_cmp(&sd, &ia) == EQUAL);

  for (i = 0; i < 255; ++i)
  {
    assert(!bignum_is_zero(&sd));
    bignum_dec(&sd);
  }
  assert(bignum_is_zero(&sd));

  bignum_init(&sa);
  bignum_init(&sb);
  bignum_init(&sc);
  bignum_init(&sd);

  char hex_1000 _Nt_checked[]    = "000003E8";
  char hex_1000000 _Nt_checked[] = "000F4240";

  /* Load 0x0308 into A and B from string */
  bignum_from_string(&sa, hex_1000, 8);
  bignum_from_string(&sb, hex_1000, 8);

  /* Load 0x0308 into C from integer */
  bignum_from_int(&sc, 0x3e8);

  /* Check for agreement - does bignum_from_string match bignum_from_int? */
  assert(bignum_cmp(&sa, &sc) == EQUAL);
  assert(bignum_cmp(&sb, &sc) == EQUAL);

  /* Load comparison value: */
  bignum_from_string(&sd, hex_1000000, 8);
  bignum_from_int(&se, 0xf4240);

  /* Perform calculation:  C = A * B => C = 0x308 * 0x308 */
  bignum_mul(&sa, &sb, &sc);

  /* Check if results agree: */
  assert(bignum_cmp(&sc, &sd) == EQUAL);
  assert(bignum_cmp(&sc, &se) == EQUAL);

  /* Perform calculation other way around: C = B * A */
  bignum_mul(&sb, &sa, &sc);

  /* Check if results agree: */
  assert(bignum_cmp(&sc, &sd) == EQUAL);
  assert(bignum_cmp(&sc, &se) == EQUAL);


  printf("\nTests successful.\n\n");

  return 0;
}

