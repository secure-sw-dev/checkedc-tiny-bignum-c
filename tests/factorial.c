/*

    Testing Big-Number library by calculating factorial(100) a.k.a. 100!
    ====================================================================

    For the uninitiated:

        factorial(N) := N * (N-1) * (N-2) * ... * 1


    Example:

        factorial(5) = 5 * 4 * 3 * 2 * 1 = 120



    Validated by Python implementation of big-numbers:
    --------------------------------------------------

        In [1]: import math

        In [2]: "%x" % math.factorial(100)
        Out[]: '1b30964ec395dc24069528d54bbda40d16e966ef9a70eb21b5b2943a321cdf10391745570cca9420c6ecb3b72ed2ee8b02ea2735c61a000000000000000000000000'


    ... which should also be the result of this program's calculation


*/


#include <stdio.h>
#include "bn.h"

#pragma CHECKED_SCOPE on

void factorial(_Ptr<struct bn> n, _Ptr<struct bn> res)
{
  struct bn tmp = {};

  /* Copy n -> tmp */
  bignum_assign(&tmp, n);

  /* Decrement n by one */
  bignum_dec(n);
  
  /* Begin summing products: */
  while (!bignum_is_zero(n))
  {
    /* res = tmp * n */
    bignum_mul(&tmp, n, res);

    /* n -= 1 */
    bignum_dec(n);
    
    /* tmp = res */
    bignum_assign(&tmp, res);
  }

  /* res = tmp */
  bignum_assign(res, &tmp);
}


int main(int argc, _Array_ptr<_Nt_array_ptr<char>> argv : count(argc))
{
  struct bn num = {};
  struct bn result = {};
  char buf _Nt_checked[8193];

  bignum_from_int(&num, 100);
  factorial(&num, &result);
  bignum_to_string(&result, buf, sizeof(buf)-1);
  printf("factorial(100) using bignum = %s\n", buf);

  return 0;
}


