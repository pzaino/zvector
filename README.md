<img align="right" width="320" height="280" src="/images/ZVectorLogo2.png">

Development branch status: 
![CodeQL](https://github.com/pzaino/vector/actions/workflows/codeql-analysis.yml/badge.svg)  
[![CI](https://github.com/pzaino/vector/workflows/CI/badge.svg)](https://github.com/pzaino/vector/actions)


# ZVector
This is a fast, configurable, portable, thread safe and reentrant Vector Library (dynamic arrays) in ANSI C.

You can use ZVector to create:
- Dynamic Arrays
- Dynamic Stacks (LIFO)
- Dynamic Queues (FIFO)

The library also offers automatic safe data wiping, so you can use it to store sensitive data. It is also constantly tested for security and bug hunting.

## Introduction
I wrote this library for fun, after watching some presentations on the internet (from different authors) that were talking about dynamic arrays in standard C.

The library is relatively small, however it comes with some nice features:

- All Data Structure support - We can store whatever data structure we want in the vector; we can use our own data structures and/or use standard base types.
- Data copy support - When we add an element to the vector it gets copied in, so we can safely store elements that we have created as local (aka not using the heap).
- Secure Data Wipe support - We can set a vector to be securely wiped (there is a flag for that), and when we do that, the library will automatically zero out all the bytes that composed the element that is being removed or the entire old vector when a new vector is being created after an expansion.
- Thread Safe - The library is also Thread Safe, so if our code is multi-threaded we can use this library without having to do complicated code. The mutex are also applied for each specific vector and only when they are required, so when two threads try to modify two different vectors there are no performance penalties at all.
- Reentrant - The library should be fully reentrant, so changes are applied when we are ready to and all the library functions do not use global state.
- Configurable code default behaviour - for example: if you are working on single threaded application you can easly disable the extra thread safe code, making so the library smaller and faster. To configure the library check the zvector_config.h and the Makefile.
- Suitable for Embedded and IoT applications - The library is suitable also for Embeeded and IoT coding, when compiled without thread safe code.
- Suitable for low memory devices - For low memory devices the library supports also a vector shrinking function to avoid any possible memory waste.
- Stack behaviour support - We can also use the vector as a dynamic stack (FIFO) structure.
- Elements swapping support - The library comes with an handy reentrant and thread safe swap function that can swap elements in the vector (vect_swap)
- Single call to apply a function to the entire vector - The library supports a single call to apply a C function to each and every element of the vector, very handy in many situations (vect_apply).
- Data copy/move and merge support - The library comes with 3 handy calls to copy one vector into another, or move one vector into another or merge one vector with another and in all the 3 cases a user can chose from which element to which element to perform the requested function.
- CI/CD support - The library comes with its own Unit and Integration tests that are build and executed systematically with each library build and that can be extended automatically just by adding new C files in the `tests` directory (you the make proces will detect them, build them automatically and execute them at every build)
- GitHub code test automation - This library is tested on github (check above the CodeQL badge) at every commit and pull request.

I'll add more functions as I have time and also the possibility to keep the vector ordered.

## How does it works?
It's very simple, it's an ANSI C99 library, no funky dependecies, so it should compile everywhere (let me know if you find any issue).

The vector follows the rules of immutable data. So, when you extend a vector it gets copied into a new and larger one. However, if you are concerned about the security implications of immutable data structures, you can set a special flag on each security sensitive vector you create and the library will ensure to erase each element of these flagged vectors when extending or squishing the vector or deleting it.

The library tries to hide the `_vector` datastructure, this to make it easier to use the library and improve clean coding where possible.

## How do I use it?
To learn the API have a look at the `vector.h` file in src. To learn how to use it have a look at the Unit Test code in tests.

As general rules:

Add the `zvector.h` to your C code with:

```
#include "zvector.h"
```

I wrote a full User Guide [here](https://paolozaino.wordpress.com/2021/07/27/software-development-zvector-an-ansi-c-open-source-vector-library/). I also try to keep it up-to-date.

When compile make sure you link your code to the libvector.a as shown in the Makefile for the Unit Tests (in `tests`).

Before you can use a vector you need to create one using the function `vect_create([initial elements], sizeof([your data structure]), secure_wipe_true|false)`

One important note for beginners is that whenever you try to store some data in the vector, please remember:

* If you have defined such data as a pointer then you can just use the data name, so, for example: `vect_add( myvector, myDataPointer )`
* If instead you have defined your data as a regular variable for example, then you need to pass it to the add function with an `&` before its name, so something like `vect_add( myvector, &myInt )`

Don't worry, in both cases the actual data contained in your reference will be copied (aka stored) in the vector, so if you free your reference or leave the function that defined it (as long as the vector scope is above such function), the data you've stored in the vector will persist.

## How do I build it?
if you have GCC installed then use the Makefile provided, I'll add more support for other compilers when I'll have time.

Wheneever you build it the Unit Tests and Integration Tests will be build and executed as part of the library build process (I am a huge supporter of Tests Automation, Test Driven Development and BDD, so if you are new to this you can use this library to check how powerful can be test automation and how it can help you to get faster at coding and releasing code!).

## Can I use it in my own commercial applications?
Yes, absolutely. The library is distributed with the MIT license, so please have a look at the [LICENSE](./LICENSE) file for details.

## A final note
Have fun and happy coding, and if you'd like drop me a line telling me how you've used this library in your own projects,

Thanks!

## Contributors

![GitHub Contributors Image](https://contrib.rocks/image?repo=pzaino/zvector)
