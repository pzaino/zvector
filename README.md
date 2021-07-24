![CodeQL](https://github.com/pzaino/vector/actions/workflows/codeql-analysis.yml/badge.svg)

# ZVector
This is a simple yet effective library to have dynamic vectors (dynamic arrays) in ANSI C.

## Introduction
I wrote this library for fun, after watching some presentation on the internet from different authors that were talking about dynamic arrays in standard C.

The library is relatively small, however it comes with some nice features:

- We can store whatever data structure we want in the vector; we can use our own data structures and/or use standard base types.
- When we add an element to the vector it gets copied in, so we can safely store elements that we have created as local (aka not using the heap).
- We can set a vector to be securely wiped (there is a flag for that), and when we do that, the library will sero out all the bytes that composed the data that is being removed or the entire old vector when a new vector is being created after an expansion.
- The library is also Thread Safe, so if our code is multi-threaded we can use this library without having to do complicated code. The mutex are also applied for each specific vector and only when they are required, so when two threads try to modify two different vectors there are no performance penalties at all.
- The library should be fully reentrant, so changed are applied when we are ready to and all the library functions do not use global state.
- If you are working on single threaded application you can easly disable the extra thread safe code, making so the library smaller and faster.
- The library is suitable also for Embeeded and IoT coding, when compiled without thread safe code.
- For low memory devices the library supports also a vector shrinking function to avoid any possible memory waste.

I'll add more functions as I have time and also the possibility to keep the vector ordered.

## How does it works?
It's very simple, it's an ANSI C99 library, no funky dependeces, so it should compile everywhere (let me know if you find any issue).

The vector follows the rules of immutable data. So, when you extend a vector it gets copied into a new and larger one. However, if you are concerned about the security implications of immutable data structures, you can set a special flag on each security sensitive vector you create and the library will ensure to erase each element of these flagged vectors when extending or squishing the vector or deleting it.

The library tries to hide the _vector datastructure, this to make it easier to use the library and improve a big clean coding where possible.

## How do I use it?
To learn the API have a look at the `vector.h` file in src. To learn how to use it have a look at the Unit Test code in tests.

As general rules:

Add the `zvector.h` to your C code with:

```
#include "zvector.h"
```

When compile make sure you link your code to the libvector.a as shown in the Makefile for the Unit Tests (in `tests`).

Before you can use a vector you need to create one using the function `vect_create([initial elements], sizeof([your data structure]), secure_wipe_true|false)`

One important note for beginners is that whenever you try to store some data in the vector, please remember:

* If you have defined such data as a pointer then you can just use the data name, so, for example: `vect_add( myvector, myDataPointer )`
* If instead you have defined your data as a regular variable for example, then you need to pass it to the add function with an `&` before its name, so something like `vect_add( myvector, &myInt )`

Don't worry, in both cases the actual data contained in your reference will be copied (aka stored) in the vector, so if you free your reference or leave the function that defined it (as long as the vector scope is above such function), the data you've stored in the vector will persist.

## How do I build it?
if you have GCC installed then use the Makefile provided, I'll add more support for other compilers when I'll have time.

Wheneever you build it the Unit Tests will be executed as part of the build (I am a huge supported of tests automation, so if you are new to this you can use this library to check how powerful can be test automation and how they can help you to get faster at coding and releasing code!).

Thanks!
