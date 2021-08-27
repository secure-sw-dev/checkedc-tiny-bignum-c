# Converting tiny-bignum-c

## Purpose
This repository serves as a workflow demonstration of [3C](https://github.com/correctcomputation/checkedc-clang), a tool that automatically converts C code to [Checked C](https://github.com/Microsoft/checkedc-clang/wiki) code.

### Checked C

Checked C is an extension to C which aims to ensure [spatial memory safety](http://www.pl-enthusiast.net/2014/07/21/memory-safety/). It achieves this by supporting three new *checked* pointer types:
- `_Ptr<`*T*`>` is either NULL or points to a single object of type *T*
- `_Array_ptr<`*T*`>` is either NULL or points to an array of zero or more objects of type *T*
- `_Nt_array_ptr<`*T*`>` is either NULL or points to an array of zero or more objects of type *T* up to a NULL terminator

The latter two types are associated with *bounds* declarations, that define how long the pointed-to array can be. For example, `_Array_ptr<int> x : count(10);` is a declaration of variable `x` which is a pointer to an array of size 10.

Programmers can annotate regions of code as *checked*; e.g., a code block `{ ... }` can be marked `_Checked { ... }`. Checked regions restrict their contents so as to ensure spatial safety; e.g.,  checked regions may *only* use checked pointers. Regions of code deemed to be *unchecked* may freely intermix checked and legacy pointers. This flexibility permits a partial, incremental conversion of programs. 

### 3C

3C analyzes a codebase to determine how its pointers are used, and whether they are used safely. Safely used pointers have their declarations rewritten so as to have checked types. For example, a pointer declared `int *p = &x` might get rewritten to `_Ptr<int> p = &x`. 

3C is unlikely to completely and correctly convert a whole program in one go, so programmers are encouraged to make additional changes to the converted code. Once they have, they may find it useful to run through 3C again.

This demo therefore takes place over a number of commits, each of which represents the project after a step in the process. 

## Install

You will need to [install 3C](https://github.com/correctcomputation/checkedc-clang/blob/main/clang/docs/checkedc/3C/INSTALL.md) and Checked C's `clang` compiler before beginning. Since 3C's repository also includes the `clang` compiler, you can build both when installing 3C. Follow the instructions linked above and also build TARGET `clang` (along with target `3c`). 

Make sure to clone the `checkedc` project into the `checkedc-wrapper` directory as in the instructions. There are important header files that 3C needs. Note the location of the `build/bin` directory, which might change if you use an IDE. Add the build directory to your `PATH` so that both `3c` and `clang` are available.

The demo uses the version of 3C and Checked C's `clang` current as of August 26, 2021 ([this commit](https://github.com/correctcomputation/checkedc-clang/commit/d7b9b2de54257682c03db5c4023443ab0f5f08cb) for 3C and clang, and [this commit](https://github.com/microsoft/checkedc/commit/5b51b0fc788fd94ffd1c9de71b1643def4724e6f) for the Checked C headers, needed when building them both).

## Getting the Repo Ready for the Demo

We start from a fork of the [tiny-bignum-c project](https://github.com/kokke/tiny-bignum-c). This is a small library with multiple test programs. The rest of this README describes the changes we made starting at <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/1d7a1f9b8e77316187a6b3eae8e68d60a6f9a4d4" data-commit-start="true">this commit</a> (the root of the fork). If you want to go through the tutorial yourself, we recommend cloning this repo and then going to <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/e1e7d95d1373b119ca0351a4526c76de8159388f" data-commit-subject="setup">this commit</a>. It's the same as the one above, but we have added a script `convert_all.sh` to the root directory and improved the makefile and `.gitignore` file. Save this README, though, as it was added to the repo after the work it describes.

## Initial Conversion

To being the conversion, we run 3C as follows, using the `convert_all.sh` script:
```
3c -alltypes -warn-root-cause \
-output-dir=out \
./bn.c \
tests/hand_picked.c \
tests/rsa.c \
tests/factorial.c \
tests/load_cmp.c \
tests/test_div_algo.c \
tests/golden.c \
tests/randomized.c \
-- -Wall -Wextra -I.
```
This is a single command that executes `3c` on `bn.c`, the main library file, and on `tests/*.c`, the various test files. In the process of considering all of these files together, it will also consider `bn.h`, since the files `#include` it. There are several options we are passing to `3c`.

- `-alltypes` directs `3c` to attempt infer `_Array_ptr` and `_Nt_array_ptr` types, and their bounds. If you do not pass in `-alltypes` then only `_Ptr` types are inferred.
- `-warn-root-cause` asks `3c` to output information about certain "root causes" that prevent pointers from being made checked. This option currently doesn't output anything for `tiny-bignum-c`, but it is useful in general.
- `-output-dir=out` tells `3c` to not rewrite the original `.c` or `.h` file, but instead to output any changed file in a mirrored directory structure rooted at directory `out`. E.g., `bn.c` is output to `out/bn.c` and `tests/load_cmp.c` is output to `out/tests/load_cmp.c`.
- The `--` is a standard syntax used with Clang LibTooling-based tools (which `3c` is). It says that the following options should be passed to the `clang` portion of `3c` before running the `3c`-specific portion. The three options here say to run the typechecker with extra warnings and to consider the current directory for `#include`s.

    As an alternative to the `--` syntax, you can have `3c` read the `clang` options from a Clang _compilation database_ (in which case the options can be different for each file), but we don't bother to use a compilation database for this small project. If you are not using a compilation database, you should pass `--` to tell `3c` not to search for one, even if you are not passing any `clang` options after the `--`.

After running this command, we will see modified versions of the library files in the `out` directory. We can run `cp -r out/* .` to move the checked versions over-top of the originals. Now, if we like, we can see the effect of the conversion by doing `git diff`. If we wanted to recover overwritten files we could use `git reset`. 

We <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/6a9b86640853b7a94c3b3a917917a586b201635c" data-commit-subject="First run of 3c">commit these changes</a> with comment "First run of 3c". 

## Fixing `bignum_to_string` buffer lengths

A lot of changes were made by this conversion command. Having copied the changed files over-top their originals, we can see these changes with the command `git diff`. Most of the diffs involve `struct bn*` being changed to `_Ptr<struct bn>`. Arrays have the `_Checked` or `_Nt_checked` keyword added to ensure runtime bounds checks, and all checked pointers are initialized.

Now we can try to compile the code using Checked C's `clang` by typing `make -k CC=clang` (recall that we are assuming that Checked C's `clang` is in your `PATH`). Unfortunately, the code does not compile; we see a bunch of errors like this in different files:

```
./tests/golden.c:260:29: error: argument does not meet declared bounds for 2nd parameter
      bignum_to_string(&sa, buf, sizeof(buf));
                            ^~~
./tests/golden.c:260:29: note: destination bounds are wider than the source bounds
./tests/golden.c:260:29: note: destination upper bound is above source upper bound
./tests/golden.c:260:29: note: (expanded) expected argument bounds are 'bounds((char *)buf, (char *)buf + (int)sizeof (buf))'
./tests/golden.c:260:29: note: (expanded) inferred bounds are 'bounds(buf, buf + 8191)'
      bignum_to_string(&sa, buf, sizeof(buf));
                            ^~~
```

The issue arises from the call to `bignum_to_string`. If we look at `bn.h` and `bn.c`, we will notice that nearly every pointer is a fully checked pointer, but the type of `str` in this function prototype is an exception:

```c
void bignum_to_string(_Ptr<struct bn> n, char *str : itype(_Array_ptr<char>) count(maxsize), int maxsize);
```

It has an *interop type* `char *str : itype(_Array_ptr<char>) ...`. To a first approximation, this means that other code can treat `str` as either an unchecked `char *` or a checked `_Array_ptr<char>` without an explicit cast. Interop types (often abbreviated "itypes") provide valuable flexibility in the middle of porting: we gain the ability to treat `str` as an `_Array_ptr<char>` without having to immediately add casts to all code that treats it as a `char *`. Here, `str` is used unsafely in the body of `bignum_to_string`, so 3C automatically assigned it an itype so the body of `bignum_to_string` will continue to compile but 3C can go ahead and start updating calls to `bignum_to_string` to pass checked pointers. In this way, 3C localizes unsafe behavior in order to convert as much of the code as possible in each pass.

We'll see later why 3C believes `str` is used unsafely inside `bignum_to_string`. For now, let's figure out why the calls to `bignum_to_string` are producing errors. 3C has guessed that `str` has the bound `count(maxsize)` because at the call sites, the `maxsize` argument is in fact equal to the size of the buffer passed for `str`. For example, `tests/golden.c` contains the following code:

```c
char buf _Nt_checked[8192];
// ...
bignum_to_string(&sa, buf, sizeof(buf));
printf("    a = %s \n", buf);
```

However, 3C's guess did not take into account that at the call site, `buf` has been inferred to be a _null-terminated_ array (`_Nt_checked`), which follows special rules in Checked C. (`buf` has to be null-terminated because it is passed to `printf`, which requires a null-terminated string.) In Checked C, an `_Nt_array_ptr : count(n)` points to an array of at least `n` elements _not counting_ the null terminator. This makes it safe to convert an `_Nt_array_ptr : count(n)` to an `_Array_ptr : count(n)`, because the `_Array_ptr` cannot be used to overwrite the null terminator. (If the `_Nt_array_ptr` count included the null terminator, then conversion to an `_Array_ptr` would have to subtract 1 from the count, which would be ugly.) However, this means that when an `_Nt_checked` array is declared, Checked C automatically reserves the last element for the null terminator, and the corresponding `_Nt_array_ptr` count is one less than the original array size. In our example, `buf` becomes an `_Nt_array_ptr<char> : count(8191)`. This is not convertible to the `_Array_ptr<char> : count(8192)` that we get when we instantiate the declared type of the `str` parameter with the specified `maxsize = sizeof(buf) = 8192`, hence the compile error.

To find the correct way to fix the error, first we need to confirm what the API of `bignum_to_string` actually is: does the `maxsize` parameter include the null terminator or not? Either design is valid as long as it is used consistently. Clearly, the call sites assume that `maxsize` includes the null terminator. If this were the API, we would just change the declaration of `str` to say `count(maxsize-1)`, and the errors at the call sites would go away. But examination of the implementation of `bignum_to_string` suggests that it assumes that `maxsize` (a.k.a. `nbytes`) _excludes_ the null terminator: `bignum_to_string` asserts that `nbytes` is even on the grounds that converting a whole number of bytes to hexadecimal should produce an even number of characters, which only makes sense if `nbytes` is the number of characters excluding the null terminator. So **Checked C porting has called our attention to an inconsistency in the original C code that presents a potential spatial memory safety violation.** If the caller passes an 8192-byte buffer (8191 bytes plus a null terminator) but `bignum_to_string` thinks the buffer has space for 8192 bytes plus a null terminator, `bignum_to_string` might write the null terminator out of bounds or might just write 8192 non-null bytes, causing the subsequent `printf` to read out of bounds.

To resolve the API inconsistency, we can either change the call sites so that `maxsize` excludes the null terminator or change the implementation so that `maxsize` includes it. Again, either approach would lead to a working program; in general, the decision may depend on the difficulty of updating the call sites as compared to the implementation. In this case, a quick look at both the implementation and callers of `bignum_from_string` indicates that its `nbytes` parameter excludes the null terminator, so we decide to make `bignum_to_string` consistent with `bignum_from_string` by changing the call sites so that `maxsize` excludes the null terminator. This is a matter of replacing `sizeof(buf)` with `sizeof(buf)-1` (or whatever the variable is named) at all the call sites. But since `bignum_to_string` requires `maxsize` to be even, we also have to add 1 to the size of `buf` so that `sizeof(buf)-1` will be even.

We <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/ddc132d587c7943ec7d112d17573d8f19fd9a04c" data-commit-subject="fix bignum_to_string buffer lengths">commit these changes</a> with the message "manual commit: fix bignum_to_string buffer lengths".

## Fixing the unsafe use of `bignum_from_string`'s `str`

With these fixes in place, we can try to compile the program again. We still see a problem:

```
bn.c:117:12: error: it is not possible to prove argument meets declared bounds for 1st parameter
    sscanf(&str[i], SSCANF_FORMAT_STR, &tmp);
           ^~~~~~~
bn.c:117:12: note: source bounds are an empty range
bn.c:117:12: note: the expected argument bounds use the variable 'i' and there is no relational information involving 'i' and any of the expressions used by the inferred bounds
bn.c:117:12: note: (expanded) expected argument bounds are 'bounds((const char *)&str[i], (const char *)&str[i] + 0)'
bn.c:117:12: note: (expanded) inferred bounds are 'bounds(str, str + 0)'
    sscanf(&str[i], SSCANF_FORMAT_STR, &tmp);
           ^~~~~~~
```

The Checked C compiler comes with its own Checked C declarations of the most common C library functions. If we look at the type of `sscanf` in the header [`stdio_checked.h`](https://github.com/microsoft/checkedc/blob/master/include/stdio_checked.h#L106), we see that the input buffer of `sscanf` has an itype:

```c
int sscanf(const char * restrict s : itype(restrict _Nt_array_ptr<const char>),
           const char * restrict format : itype(restrict _Nt_array_ptr<const char>), ...);
```

This is because `sscanf` stops reading at a null byte, so we can safely pass any `_Nt_array_ptr<const char>` and `sscanf` will not read out of bounds. Because of the itype, 3C could make the `str` parameter fully checked. But while we could safely pass `str` itself to `sscanf`, the same is not true of `&str[i]`: unless we know something more about `str` or `i`, `&str[i]` might be beyond the null terminator of `str` and `sscanf` might read out of bounds. The intent is clearly that `bignum_from_string` should be called with `nbytes` equal to the length of `str`, but this is not enforced since the declaration of `str` does not specify a `count`. So while the `bignum_from_string` implementation is in fact safe with respect to its _intended_ API (unlike the original `bignum_to_string`), we don't have the protection against incorrect calls that we would expect in Checked C.

To fix this, first we have to change the declaration of `str` to `_Nt_array_ptr<char> str : count(nbytes)` to express the assumption about its length. If we stop here, we still get a compile error (the same as above). Why? While the compiler now knows that `str` is `nbytes` long, it is unable to guess the loop invariant `0 <= i <= nbytes - (2 * WORD_SIZE)` that is needed to prove that `&str[i]` is in bounds. We help the compiler out using a `_Dynamic_bounds_cast`. Such a cast is an assertion about a pointer's bounds that the compiler can assume to be true from here on, based on a check it will insert at run-time. We replace the `sscanf` call with the following:

```c
    _Nt_array_ptr<char> read_pos =
      _Dynamic_bounds_cast<_Nt_array_ptr<char>>(str + i, count(0));
    _Unchecked { sscanf(read_pos, SSCANF_FORMAT_STR, &tmp); }
```

<details>
  <summary>Why is the bound `count(0)`?</summary>
  
Do not be confused by the `count(0)`: here, and for any `_Nt_array_ptr`, it specifies the guaranteed _minimum_ length of the array, not the exact length. `_Nt_array_ptr` without a count specified defaults to `count(0)`. Checked C code is allowed to read elements beyond this minimum as long as it checks that each element is not a null terminator before reading the next, as `sscanf` does.
</details>

Now `bn.c` compiles, but we (eventually, it takes a while to compile) get three similar errors in `tests/randomized.c`:

```
./tests/randomized.c:39:35: error: call expression not allowed in argument for parameter used in function parameter bounds expression
  bignum_from_string(&a, argv[2], strlen(argv[2]));
                                  ^~~~~~~~~~~~~~~
./tests/randomized.c:40:35: error: call expression not allowed in argument for parameter used in function parameter bounds expression
  bignum_from_string(&b, argv[3], strlen(argv[3]));
                                  ^~~~~~~~~~~~~~~
./tests/randomized.c:41:35: error: call expression not allowed in argument for parameter used in function parameter bounds expression
  bignum_from_string(&c, argv[4], strlen(argv[4]));
                                  ^~~~~~~~~~~~~~~
```

The problem here is that the compiler can't reason about the `strlen` call's connection its argument's length. Since this code pattern occurs 3 times and it's going to take several extra lines of code to make the compiler happy, let's start by encapsulating the operation in a helper function:

```c
  bignum_from_string_using_strlen(&a, argv[2]);
  bignum_from_string_using_strlen(&b, argv[3]);
  bignum_from_string_using_strlen(&c, argv[4]);
```

For the implementation of `bignum_from_string_using_strlen`, we use the following:

```c
void bignum_from_string_using_strlen(_Ptr<struct bn> n, _Nt_array_ptr<char> str) {
  int len = strlen(str);
  _Nt_array_ptr<char> str2 : count(len) =
    _Assume_bounds_cast<_Nt_array_ptr<char>>(str, count(len));
  bignum_from_string(n, str2, len);
}
```

Here the `_Assume_bounds_cast` is telling the compiler to simply _assume_ that `str2` (an alias for `str`) is at least as long as `count(len)`. This is not a crazy assumption: This is what `strlen` is supposed to do! The Checked C compiler should soon be able to verify this. 

Now the code compiles without warnings. You can run the tests by typing `make test`. We <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/88fcaca0142a5ec622d49610b6f55974b38049c9" data-commit-subject="migrate bignum_from_string">commit these changes</a> with the message "manual commit: migrate bignum_from_string".

<details>
  <summary>What if I don't want to assume <tt>strlen</tt>'s connection to its parameter's bounds?</summary>

Since the compiler does not verify the assumption, this construct is not allowed in a checked region, so we have to mark the function `_Unchecked`. This version of the code compiles and passes the tests. However, with a little more work, we can eliminate the unchecked region by scanning the string ourselves in a way that the compiler can verify that the null terminator is where we claim it is:

```c
void bignum_from_string_using_strlen(_Ptr<struct bn> n, _Nt_array_ptr<char> str) {
  int i = 0;
  _Nt_array_ptr<char> str2 : count(i) = str;
  while (str2[i]) {
    int j = i + 1 _Where str2 : count(j);
    i = j;
  }
  bignum_from_string(n, str2, i);
}
```

Each time we check that `str2[i]` is not the terminator, we're allowed to increase the guaranteed minimum length of `str2` to `i + 1`. The loop terminates with `i == strlen(str)`, so we call the equivalent of the original `bignum_from_string(n, str, strlen(str))` but with the Checked C compiler able to verify the spatial memory safety. 

While it's nice to know how to verify the length of an `_Nt_array_ptr` at runtime in Checked C, in the real world, we might prefer to use the first implementation of `bignum_from_string_using_strlen` with the unchecked region and accept responsibility for reviewing it manually (it's very simple) in exchange for using the C library's much better-optimized `strlen` implementation.
</details>

## Fixing `bignum_to_string` using `snprintf`

We said earlier that 3C gave the `str` parameter of `bignum_to_string` an interop type (itype) because `str` was used unsafely inside the function. What exactly is the unsafe use? The easiest way to find out is to manually change the type to be fully checked rather than an itype:

```c
void bignum_to_string(_Ptr<struct bn> n, _Array_ptr<char> str : count(nbytes), int nbytes)
```

and likewise in `bn.h`, and then recompile: this will turn any unsafe use into a compile error:

```c
bn.c:140:13: error: passing '_Array_ptr<char>' to parameter of incompatible type 'char *'
    sprintf(&str[i], SPRINTF_FORMAT_STR, n->array[j]); 
            ^~~~~~~
/home/mwh/checkedc-clang/build/lib/clang/12.0.0/include/stdio_checked.h:102:29: note: passing argument to parameter 's' here
int sprintf(char * restrict s,
                            ^
```

The problem here is a type mismatch in the call to `sprintf`. Here's its checked type in [`stdio_checked.h`](https://github.com/microsoft/checkedc/blob/master/include/stdio_checked.h#L101):

```c
int sprintf(char * restrict s,
            const char * restrict format : itype(restrict _Nt_array_ptr<const char>), ...);
```

Notice that `format` has an itype, so it can be passed either a `const char *` (by C code that has not yet been converted) or an `_Nt_array_ptr<const char>` (by fully checked code) without a cast. But `s` (the output buffer) is just a `char *`, so it does not accept checked pointers at all, hence the error when we try to pass `&str[i]` (which is a checked pointer because `str` is). `s` was declared this way because it's not easy to statically bound the amount of data that `sprintf` writes to its output buffer (as C programmers should be well aware!).

If we want to verify our code with Checked C, we'll have to use `snprintf` instead. Here's [its checked declaration](https://github.com/microsoft/checkedc/blob/master/include/stdio_checked.h#L119):

```c
int snprintf(char * restrict s : itype(restrict _Nt_array_ptr<char>) count(n-1),
             size_t n _Where n > 0,
             const char * restrict format : itype(restrict _Nt_array_ptr<const char>), ...);
```

Here `s` accepts an `_Nt_array_ptr<char> : count(n-1)` (because the `n` parameter to `snprintf` includes the null terminator but the `count` of an `_Nt_array_ptr` does not).

Based on this bounds declaration, we should call `snprintf(&str[i], nbytes - i + 1, ...)`: Any case that would previously have caused a buffer overflow will instead silently truncate the output. However, silent truncation may still constitute an application bug, so while we're thinking about buffer lengths, it's worth thinking about truncation too. A quick look at the definition of `SPRINTF_FORMAT_STR` shows that we expect it to write `2 * WORD_SIZE` characters. The loop condition `nbytes > (i + 1)` (equivalent to `nbytes >= i + 2`) ensures that we have space for another 2 characters, but we need `2 * WORD_SIZE` (and `WORD_SIZE` defaults to 4). So **Checked C porting has called our attention to another buffer overflow in the original C code!** It wasn't triggered by any of the test cases since they all use large buffers, but we could demonstrate it by using a small buffer size that is a multiple of 2 but not `2 * WORD_SIZE`.

It appears the intent was to exit the loop if the next iteration would overflow the buffer, so to achieve that, we can simply replace `nbytes > (i + 1)` with `nbytes >= i + 2 * WORD_SIZE`.
(We will just assume output truncation is OK at this point, to keep things moving; a more debuggable design might be to report an error if the output buffer is not big enough.)

After updating the code as follows:

```c
  while ((j >= 0) && (nbytes >= i + 2 * WORD_SIZE))
  {
    snprintf(&str[i], nbytes - i + 1, SPRINTF_FORMAT_STR, n->array[j]);
```

we still get a similar compile error:

```c
bn.c:140:14: error: passing '_Array_ptr<char>' to parameter of incompatible type 'char *'
    snprintf(&str[i], nbytes - i + 1, SPRINTF_FORMAT_STR, n->array[j]);
             ^~~~~~~
```

The problem now is that `str` is an `_Array_ptr<char>` but `snprintf` requires _either_ a `char *` or an `_Nt_array_ptr<char>`. (The error message leaves something to be desired.) When we originally ran 3C, there was nothing in the code to indicate that `str` needed to be an `_Nt_array_ptr<char>`, so 3C gave it the more general type `_Array_ptr<char>`. We can just change the type to `_Nt_array_ptr<char>` in both `bn.c` and `bn.h`, since the call sites are already passing`_Nt_array_ptr<char>`. Then we're left with a compiler warning:

```c
bn.c:140:14: warning: cannot prove argument meets declared bounds for 1st parameter [-Wcheck-bounds-decls-unchecked-scope]
    snprintf(&str[i], nbytes - i + 1, SPRINTF_FORMAT_STR, n->array[j]);
             ^~~~~~~
bn.c:140:14: note: (expanded) expected argument bounds are 'bounds((char *)&str[i], (char *)&str[i] + (size_t)nbytes - i + 1 - 1)'
bn.c:140:14: note: (expanded) inferred bounds are 'bounds(str, str + nbytes)'
    snprintf(&str[i], nbytes - i + 1, SPRINTF_FORMAT_STR, n->array[j]);
             ^~~~~~~
```

Unfortunately, the compiler's support for algebraic simplification of bounds expressions is currently very limited, and it is unable to verify that `(char *)str + i + (size_t)nbytes - i + 1 - 1 = str + nbytes`. We can live with this warning, or we can use a `_Dynamic_bounds_cast` to give us greater assurance.

<details>
<summary>How we can use `_Dynamic_bounds_cast` to eliminate this warning</summary>

To make the warning go away, we can use a `_Dynamic_bounds_cast`. We also have to introduce several temporary variables: We replace the original `snprintf` call with the following:

```c
    size_t remaining_space = nbytes - i;
    _Nt_array_ptr<char> write_pos : count(remaining_space) =
      _Dynamic_bounds_cast<_Nt_array_ptr<char>>(str + i, count(remaining_space));
    snprintf(write_pos, remaining_space + 1, SPRINTF_FORMAT_STR, n->array[j]);
```

Now the code once again compiles without warnings and passes the tests. 

Another valid approach would have been to use `2 * WORD_SIZE + 1` rather than `nbytes - i + 1` as the size argument to `snprintf` to make it clearer when looking at the `snprintf` call in isolation that truncation cannot occur. Ideally, the Checked C compiler would still be able to verify that `snprintf(str + i, 2 * WORD_SIZE + 1, ...)` is safe because of the loop condition that `nbytes >= i + 2 * WORD_SIZE`. However, the compiler currently doesn't support this kind of reasoning, so we'd have to use a `_Dynamic_bounds_cast` like the one above anyway. If we want to detect truncation, yet another approach would be to use a wrapper function for `snprintf` that raises an assertion error on truncation.
</details>

At this point the code compiles again, and the tests work properly. We <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/157871636d94df7ddac166c4585f4d3c1be95227" data-commit-subject="migrate bignum_to_string to snprintf">commit these changes</a> with the message "manual commit: migrate bignum_to_string to snprintf".

## Putting all the code in a checked region

We've seen that by default, Checked C lets us mix checked and unchecked operations, but its safety guarantee applies only to checked operations. Ultimately, we'd like to place as much of the code as possible in checked regions to make it easier to see what (if any) unchecked operations remain whose safety we have to reason about manually. We can do this by adding `#pragma CHECKED_SCOPE on` at the beginning of each `.c` file, which places all subsequent code in a checked region.

Adding `#pragma CHECKED_SCOPE on` to the beginning of a file forces all code within it, including code from expanded `#include`s, to be considered in a checked regiond. This isn't a problem for us because we're applying it to the entire program at once and all the external headers we use (such as `stdio.h`) are compatible with `#pragma CHECKED_SCOPE on`. In a header file, it's good practice to localize the designation of a checked region to that file, thus:

```
#pragma CHECKED_SCOPE push
#pragma CHECKED_SCOPE on
 ...code...
#pragma CHECKED_SCOPE pop
```

We apply the above pattern to `bn.h`, and just add `#pragma CHECKED_SCOPE on` to the top of `bn.c`. Recompiling, we get this error:

```
bn.c:119:5: error: cannot use a variable arguments function in a checked scope or function
    sscanf(read_pos, SSCANF_FORMAT_STR, &tmp);
    ^
bn.c:140:5: error: cannot use a variable arguments function in a checked scope or function
    snprintf(&str[i], nbytes - i + 1, SPRINTF_FORMAT_STR, n->array[j]);
    ^
```

In general, there's no way for the declaration of a variable-arguments function to even specify what assumptions the function makes about the types of the variable arguments passed to it, so it isn't safe to allow calls to such a function in checked regions. In practice, this is a major nuisance for `printf` and similar functions. However, for `printf`-like functions in particular, many C compilers have special support for parsing format strings and validating the corresponding argument types. There are [plans](https://github.com/microsoft/checkedc-clang/issues/1160) to extend this validation for Checked C so that calls to `printf`-like functions that pass validation can be safely allowed in checked regions. In the meantime, the best we can do is wrap every call to a `printf`-like function in an unchecked region, like this:

```c
  _Unchecked { sscanf(read_pos, SSCANF_FORMAT_STR, &tmp); }
```
(and likewise for `snprintf`.) Doing so, these functions now compile. Adding `#pragma CHECKED_SCOPE on` to the `.c` files in `tests` will lead to similar errors, which we could fix in the same way. E.g., each call to `printf` we could replace with `_Unchecked { printf ... }`. However, this adds a lot of clutter to the code. To ameliorate this problem, we define `printf` as a preprocessor macro that automatically generates an unchecked block around the real `printf` call so that existing calls don't need to be changed:

```c
#define printf(...) _Unchecked { printf(__VA_ARGS__); }
```

Of course, what we gain in reduced clutter, we lose in ability to easily see where the potentially unsafe operations are.

Now the code _almost_ compiles: the compiler is now complaining about the `_Assume_bounds_cast` we added in `bignum_from_string_using_strlen`. We added this problem similarly: We add a `_Unchecked` annotation on the function body. With this, the code compiles and passes the tests again, _and_ we're guaranteed that the entire program is spatially memory safe except for the `_Unchecked` blocks (modulo bugs and limitations of the Checked C compiler). We <a href="https://github.com/correctcomputation/checkedc-tiny-bignum-c/commit/b44cad8bf3ef7a4a6b752b675c0c0f27cee471cd" data-commit-subject="put everything in a checked scope except printf, etc.">commit these changes</a> with the message "manual commit: put everything in a checked scope except printf, etc.".

## Final thoughts

Thus concludes our Checked C port of `tiny-bignum-c`; future versions of the compiler should make some steps a little better, but overall that wasn't too bad. We even found two spatial memory safety bugs in the original code in the process.

Good luck porting your own programs! We hope you find that 3C and Checked C provide a good degree of assurance of spatial memory safety in relation to the porting work you put in, compared to more ambitious approaches such as porting to a language like Rust. We've pursued this tutorial to the bitter end in order to illustrate as many issues as possible, but in a real port, you can choose to stop when you believe that additional reworking of unchecked code is not justified by the stronger assurance you get for that code.



