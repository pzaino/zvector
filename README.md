# ZVector

<img align="right" width="320" height="280" src="/images/ZVectorLogo2.png">

**Status:**

Still under active development.

`- Security Tests:` [![CodeQL](https://github.com/pzaino/zvector/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/pzaino/zvector/actions) [![FlawFinder](https://github.com/pzaino/zvector/actions/workflows/flawfinder.yml/badge.svg)](https://github.com/pzaino/zvector/actions)

`- _________CI/CD:` [![CI/CD](https://github.com/pzaino/zvector/actions/workflows/ci.yml/badge.svg)](https://github.com/pzaino/zvector/actions) (Linux, macOS)

This is a fast, configurable, portable, thread safe and reentrant Vector Library (dynamic arrays) in ANSI C 99.

You can use ZVector to create:

- Dynamic Arrays
- Dynamic Stacks (LIFO)
- Dynamic Queues (FIFO) (included priority queues)
- Dynamic Ordered Queues

The library also offers automatic Secure Data Wiping, so you can use it to store sensitive data. It is also constantly tested for security and bug hunting.

## Introduction

I wrote this library for fun, after watching some presentations on the internet (from different authors) about dynamic arrays in C.

### What problem does it solve?

According to a reading on [Wikipedia](https://en.wikipedia.org/wiki/Dynamic_array#Performance) on the matter of Dynamic Arrays, it seems that there are still no Dynamic Arrays libraries that solve most common limitations of generic dynamic arrays.

So, I thought it would be fun to develop a dynamic array library that actually presents high performance also when adding items at the beginning of an array as well as inside, while still maintaining the original high performance when accessing each item using their index.

So far, ZVector has resulted in solving the traditional issues presented by Dynamic arrays, and indeed one can use it to create high performance dynamic arrays that stay fast also when adding elements at the beginning of them or inside (check 04PTest001, 04PTest002 and 04PTest003 to see how one can do that).

There are also other advantages using ZVector instead of regular C Arrays (beside of being dynamic):

- For instance, passing a ZVector to a function will NOT lose it's size, so you can safely do this.

- If you need to do binary searches, the binary search provided with ZVector optimise for each vector (even when you pass them to another function, such optimisation is passed on too!).

- Fetching items from a ZVector also triggers more checks and so, in general terms, it's safer than fetching data from a regular C Array.

- ZVector offers a lot more features (described below) than a regular C Array.

### Which features does it offer?

The library is relatively small, however it comes with some nice features:

- **All Data Structures support**

   We can store whatever data structure we want in the vector; we can use our own data structures and/or use standard base types.

- **Data copy support**

   When we add an element to the vector it gets copied, so we can safely store elements that we have created as local (aka not using the heap). If you instead, need passing values by reference, then you can configure a vector to do so.

- **Secure Data Wipe support**

   We can set a vector to be securely wiped (there is a flag for that), and when we do that, the library will automatically zero out all the bytes that composed the element that is being removed or the entire old vector when a new vector is being created after an expansion.

- **Vector Properties**

   We can configure a set of properties for each vector we create using ZVector. The library will then manipulate and update the vector according to its properties. Read the User Guide for a complete list of all available properties.

- **Thread Safe**

   The library is also Thread Safe, so if our code is multi-threaded we can use this library without having to do complicated code. The mutex is also applied for each specific vector and only when it's required, so when two threads try to modify two different vectors there are no performance penalties at all.

- **Reentrant**

   The library should be fully reentrant, so changes are made available only when the full set of actions have been completed. Also, the library functions do not use global state.

- **Configurable feature-set**

   For example: if you are working on a single threaded application, you can easily disable the extra thread safe code, making so the library smaller and faster. To configure the library, check the `zvector_config.h` and the `Makefile`.

- **Suitable for Embedded and IoT applications**

   The library is suitable also for Embedded and IoT coding, when compiled without thread safe code.

- **Suitable for low memory devices**

   For low memory devices the library supports also a vector shrinking function to avoid any possible memory waste.

- **Stack and Queue behavior support**

   We can also use the vector as a dynamic stack (FIFO) structure. Or we can use it to create Queues (LIFO) structures (including priority queues).

- **Elements swapping support**

   The library comes with a handy reentrant and thread safe swap function that can swap elements in the vector (`vect_swap`), a `vect_swap_range` to swap a range of values in a vector and many more useful data manipulation functions (including vector rotation and more).

- **Single call to apply a function to the entire vector**

   The library supports a single call to apply a C function to each and every item in a vector, very handy in many situations (`vect_apply`). It also supports "conditional function application" to an entire vector (`vect_apply_if`) and a handy `vect_apply_range` which applies a user function to a range of values in a vector.

- **Bulk Data copy, move, insert and merge support**

   ZVector comes with 4 handy calls to copy one vector into another, or move it into another, merge it with another and bulk-insert items from a vector to another. These functions are also optimised for speed.

- **Custom QuickSort and Improved Adaptive Binary Search**

   ZVector comes with a custom QuickSort algorithm that uses 3 ways partitioning for very fast ordering of a vector. It also comes with an improved Adaptive Binary Search algorithm for very fast record search. Both of them supports custom user compare functions, so ordering and searches can be done for every possible type of records.

- **CI/CD support**

   The library comes with its own Unit and Integration tests that are built and executed systematically with each library build and that can be extended automatically just by adding new C files in the `tests` directory (the make process will detect them, build them automatically and execute them at every build).

- **GitHub code test automation**

   This library is continuously tested on GitHub (check above the CodeQL badge) at every commit and pull request.

More features will be added over time as well as I constantly seek to improve its performance.

## How does it work?

It's very simple, it's an ANSI C99 library, no funky dependencies, so, it should compile everywhere (let me know if you find any issues).

ZVector uses a `p_vector` struct (everything that begins with a `p_` in zvector is "private") to represent a dynamic array of arbitrary items. The library tries to hide the `p_vector` data structure (the public type is called `vector`), this to make it easier to use the library and improve clean coding where possible. Given that, a typical size for a cache line is usually 64 Bytes (16 words), p_vector is optimized for such type of caches and so, an entire vector struct should take 1 single line in a cache, plus the number of pointers to user data.

The user decides which type of items (between regular base types or custom types or data structures, etc), the initial capacity of a vector and its properties.

Properties can be expressed as a set of flags, for example: `ZV_BYREF | ZV_SEC_WIPE` will set a vector with both these two properties on. Turning on a property simply means asking ZVector to automatically deal with that specific property. So enabling `ZV_SEC_WIPE` means that ZVector itself will handle secure data wipe of the data stored in a vector when such data is no longer needed.

When a vector gets extended, it may also gets its data copied into the new larger vector, however, to improve performances, ZVector only maintains and copies an "array of pointers" to such data (so the actual user data is untouched) and the functions that perform such copy are optimized for memory bandwidth to improve performance.

## How do I use it?

To learn the API have a look at the `zvector.h` file in the `src` directory. To learn how to use it have a look at the test code in `tests` directory. I wrote a full User Guide [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/) and trying to keep it up-to-date.

As general rules:

Add the `zvector.h` to your C code with:

```C
#include "zvector.h"
```

When compiling, make sure you link your code to the `libvector.a` as shown in the `Makefile` for the tests (in `tests`).

Before you can use a vector, you need to create one using the function:

```C
vect_create([initial elements], sizeof([your data structure]), [property flags list])
```

Where:

- `[initial elements]` is an integer number that represent the initial capacity of your vector, for example 16

- `[your data structure]` is the name of your data structure that you want to store in the vector and correct C modifiers to ensure sizeof returns the correct size of your data structure or base type.

- `[property flags list]` is a set of flags that will set the properties of your vector in a typical C flag set fashion, for example: `ZV_BYREF | ZV_SEC_WIPE`

Example:

```C
vector v = vect_create(8, sizeof(int), ZV_SEC_WIPE);
```

Will create a vector with an initial size of 8 elements, that can store integer values and that has as property secure wipe set, so whenever you'll remove a number from your vector that original location will be securely wiped.

One important note for beginners is that whenever you try to store some data in the vector, please remember:

- If you have defined such data as a pointer then you can just use the data name, so, for example: `vect_add( myvector, myDataPointer )`
- If instead you have defined your data as a regular variable for example, then you need to pass it to the add function with an `&` before its name, so something like `vect_add( myvector, &myInt )`

Don't worry, in both cases the actual data contained in your reference will be copied (aka stored) in the vector (unless you've specified as your vector's property ZV_BYREF), so if you free your reference or leave the function that defined it (as long as the vector scope is above such function), the data you've stored in the vector will persist.

For the complete and up-to-date User Guide please click [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/).

## How do I build it?

### GCC or CLang

This has been tested on different Linux distributions, Apple macOS, FreeBSD, NetBSD, OpenBSD. It should also work on Free RTOS and ARM Embed OS.

if you have GCC or CLang installed then use the Makefile provided, to build:

```bash
make
```

And to build and run the tests:

```bash
make tests
```

To install the static library and headers use:

```bash
make install
```

Note for CLang users on Linux: please check the Makefile before trying to build with clang and replace the value of variable `CC:=gcc` with `CC:=clang`.

### Visual Studio

If you use Microsoft Windows, then you can open the project in Visual Studio 2019 or 2022 and build it using Visual Studio Build function.

### Other C Compilers

At the moment I started to add support for different compilers which include NORCROFT C, HPC, IBM C and quite few others which should also allow to use ZVector on Operating Systems like OS/2 (ArkaOS), Haiku OS, RISC OS, old Windows, Amiga OS, Atari TOS / EmuTOS, OS-9, IBM AIX and AS-400 and more.

I'll add support for other compilers when I'll have time.

For more details, pre-requisites and whatnot please check the User Guide [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/)

## Performance

ZVector is already really fast, however, if one wants to gain even more performance out of it, you can:

- Create large arrays from the beginning
- If your app is multi-threaded:
  - If it does a lot of sequential calls to `vect_add()` or `vect_remove()` then try to use the user locks before starting your loop of calls to vect_add or vect_remove. To use the user locks have a look at the User Guide for the function `vect_lock()` and `vect_unlock()`.
  - If you can, then use local vectors to your thread to process thread data, and when processing is completed, use `vect_move()` or `vect_merge()` to merge your local vector items to your global vector. This will reduce concurrency and increase parallelism. If you use this approach you can also improve performances even more by setting the local vector property `VECT_NOLOCKING`, so each vect_add etc. operation will not lock on the local vector. See 04PTest005 for more details on how to use this technique.
- Try to use ZVector in conjunction with jemalloc or other fast memory allocation algorithms like tcmalloc etc.
  - To run a quick test with jemalloc for example, if you have it installed in `/usr/lib64/`, then run:

```bash
LD_PRELOAD=/usr/lib64/libjemalloc.so.2 time ./tests/bin/04PTest001
```

Please note: when using libraries like jemalloc and similar, performance improvements will depend a lot on the system architecture you're using to test your code. So, do not expect the same performance improvements on an old Atom CPU compared to a more modern AMD Zen3 ;)

To have an idea of the performance, you can use the following tests that come with ZVector:

- 04PTest005 This test spins 16 threads, 8 producers and 8 consumers and they all work in parallel. This happens because each producer and consumer works on a local vector that is derived from the global vector in chunks. So the only concurrency is when a chunk of items (all at once) is either moved from a local vector to the global vector or is fetched from the global vector into a local vector. This technique helps a lot to improve Amdahl's law constraints on the matter of parallelism. If you have a look at the test code, ZVector handles all the complexity of using multi-threading, so one can simply use local structures and let ZVector deal with locking mechanisms and concurrency complexity. When you run this test using jemalloc or tcmalloc you reduce the critical sections time even more improving both performance and parallelism.
             You can easily increase the number of threads in this test to your like, need. Look at the source for more details.

- 04PTestX All the tests that starts with 04PTest are generic performance tests and they try to measure specific costs of each activity in the library.

If you do not need the Thread Safe code in your own projects, then you can disable it from the Makefile (have a look at the file for details). Disabling the Thread Safe code will make the library even faster.

If you do not need all the features offered by the library, you can disable subsets of the features. This will allow the library binary to be even smaller than it is now (on an ARM it's roughly 40KB) and that can help with caching the entire library.

## How much data can I store in this?

ZVector, by default, uses unsigned int 32 bit (`uint32_t`)for the vector index (`vect_index`), so the amount of possible storage available per each vector is huge. If needed you can also reconfigure this using unsigned integers 64bit have a look at the file `zvect_config.h` for this.

Of course for IoT, embedded devices and retrocomputing Operating Systems, you can change the default behavior to use smaller indexes too, for instance using unsigned int 16bit for old 16bit Architectures. Again changing this is really simple, just redefine the typedef for `zvect_index` in the `zvect_config.h`

## How small can I make this library?

So, if you want to use it for embedded software developments and IoT, I have shrunk release 1.0.0 RC7 down to 17KB on ARM AArch64.

To achieve similar results, you just need to configure the `Makefile` as follows:

- Replace the values in `P_CFLAGS` with just `-Os` if you're using GCC or clang
- Disable ALL the extensions in the extension section of the Makefile
- Compile as explained above and check the results

## Can I use it in my own commercial applications?

Yes, absolutely. The library is distributed with the MIT license, so please have a look at the [LICENSE](./LICENSE) file for details.

## A final note

Have fun and happy coding, and if you'd like, drop me a line telling me how you've used this library in your own projects,

Thanks!

## Contributors

![GitHub Contributors Image](https://contrib.rocks/image?repo=pzaino/zvector)
