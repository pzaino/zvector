<img align="right" width="320" height="280" src="/images/ZVectorLogo2.png">

Development branch status:

[![CodeQL](https://github.com/pzaino/zvector/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/pzaino/zvector/actions)

[![Spectral Scan](https://github.com/pzaino/zvector/actions/workflows/spectral_scan.yml/badge.svg)](https://github.com/pzaino/zvector/actions)

[![CI/CD](https://github.com/pzaino/zvector/actions/workflows/CI/badge.svg)](https://github.com/pzaino/zvector/actions) (Linux, macOS)


# ZVector
This is a fast, configurable, portable, thread safe and reentrant Vector Library (dynamic arrays) in ANSI C.

You can use ZVector to create:
- Dynamic Arrays
- Dynamic Stacks (LIFO)
- Dynamic Queues (FIFO) (included priority queues)
- Dynamic Ordered Queues

The library also offers automatic Secure Data Wiping, so you can use it to store sensitive data. It is also constantly tested for security and bug hunting.

## Introduction
I wrote this library for fun, after watching some presentations on the internet (from different authors) about dynamic arrays in C.

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

   The library should be fully reentrant, so changes are applied when we are ready for, and all the library functions do not use global state.

- **Configurable featureset**

   For example: if you are working on a single threaded application, you can easily disable the extra thread safe code, making so the library smaller and faster. To configure the library, check the zvector_config.h and the Makefile.

- **Suitable for Embedded and IoT applications**

   The library is suitable also for Embedded and IoT coding, when compiled without thread safe code.

- **Suitable for low memory devices**

   For low memory devices the library supports also a vector shrinking function to avoid any possible memory waste.

- **Stack and Queue behaviour support**

   We can also use the vector as a dynamic stack (FIFO) structure. Or we can use it to create Queues (LIFO) structures (including priority queues)

- **Elements swapping support**

   The library comes with a handy reentrant and thread safe swap function that can swap elements in the vector (vect_swap), a vect_swap_range to swap a range of values in a vector and many more useful data manipulation functions (including vector rotation and more).

- **Single call to apply a function to the entire vector**

   The library supports a single call to apply a C function to each and every item in a vector, very handy in many situations (vect_apply). It also supports "conditional function application" to an entire vector (vect_apply_if) and a handy vect_apply_range which applies a user function to a range of values in a vector.

- **Bulk Data copy, move, insert and merge support**

   ZVector comes with 4 handy calls to copy one vector into another, or move it into another, merge it with another and bulk-insert items from a vector to another. These functions are also optimised for speed.

- **Custom QuickSort and Improved Adaptive Binary Search**

   ZVector comes with a custom QuickSort algorithm that uses 3 ways partitioning for very fast ordering of a vector. It also comes with an improved Adaptive Binary Search algorithm for very fast record search. Both of them supports custom user compare functions, so ordering and searches can be done for every possible type of records.

- **CI/CD support**

   The library comes with its own Unit and Integration tests that are built and executed systematically with each library build and that can be extended automatically just by adding new C files in the `tests` directory (you the make process will detect them, build them automatically and execute them at every build)

- **GitHub code test automation**

   This library is continuously tested on GitHub (check above the CodeQL badge) at every commit and pull request.

More features will be added over time as well as I constantly seek to improve its performance.

## How does it works?
It's very simple, it's an ANSI C99 library, no funky dependencies, so it should compile everywhere (let me know if you find any issue).

ZVector uses a `_vector` struct to represent a dynamic array of arbitrary items. The library tries to hide the `_vector` data structure, this to make it easier to use the library and improve clean coding where possible.

The user decides which type of items (between regular base types or custom types or data structures, etc), the initial capacity of a vector and its properties.

Properties can be expressed as a set of flags, for example: ZV_BYREF | ZV_SEC_WIPE will set a vector with both these two properties on. Turning on a property simply means asking ZVector to automatically deal with that specific property. So enabling ZV_SEC_WIPE means that ZVector itself will handle secure data wipe of the data stored in a vector when such data is no longer needed.

When a vector gets extended it may also gets its data copied into the new larger vector, however, to improve performances, ZVector only maintains and copies an array of pointers to such data (so the actual user data is untouched) and the functions that perform such copy are optimised for memory bandwidth to improve performance.

## How do I use it?
To learn the API have a look at the `vector.h` file in src. To learn how to use it have a look at the Unit Test code in tests.

As general rules:

Add the `zvector.h` to your C code with:

```
#include "zvector.h"
```

I wrote a full User Guide [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/) and trying to keep it up-to-date.

When compile make sure you link your code to the libvector.a as shown in the Makefile for the Unit Tests (in `tests`).

Before you can use a vector you need to create one using the function `vect_create([initial elements], sizeof([your data structure]), secure_wipe_true|false)`

One important note for beginners is that whenever you try to store some data in the vector, please remember:

* If you have defined such data as a pointer then you can just use the data name, so, for example: `vect_add( myvector, myDataPointer )`
* If instead you have defined your data as a regular variable for example, then you need to pass it to the add function with an `&` before its name, so something like `vect_add( myvector, &myInt )`

Don't worry, in both cases the actual data contained in your reference will be copied (aka stored) in the vector (unless you've specified as your vector's property ZV_BYREF), so if you free your reference or leave the function that defined it (as long as the vector scope is above such function), the data you've stored in the vector will persist.

## How do I build it?
if you have GCC installed then use the Makefile provided, to build:

```
make
```

And to build and run the tests:

```
make tests
```

To install the static library and headers use:

```
make install
```

I'll add support for other compilers when I'll have time.

For more details, pre-requisites and whatnot please check the User Guide [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/)

## Can I use it in my own commercial applications?
Yes, absolutely. The library is distributed with the MIT license, so please have a look at the [LICENSE](./LICENSE) file for details.

## A final note
Have fun and happy coding, and if you'd like, drop me a line telling me how you've used this library in your own projects,

Thanks!

## Contributors

![GitHub Contributors Image](https://contrib.rocks/image?repo=pzaino/zvector)
