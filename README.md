# Vector
This is a simple yet effective library to have dynamic vectors (dynamic arrays) in C.

## Introduction
I wrote this library for fun, after watching some presentation on the internet from different authors that were talking about dynamic arrays in standard C.

One of the nice feature of this library is that you can store whatever you want in the vector, so yes you can use your own data structures and/or use standard base types.

Another nice featrue of the library is that when you add an element to the vector it gets copied in, so you can safely store datastructure elements that you have created as local (aka not using the heap).

Another nice feature is that you can set a vectore to be wiped (there is a flag for that), and when you do the library will wipe the data that are being removed or the entire old vector when a new vector is being created after an expansion.

I'll add more functions as I have time and also the possibility to keep the vector ordered.

## How does it works?
It's very simple, it's an ANSI C99 library, no funky dependeces, so it should compile everywhere (let me know if you find any issue).

The vector follows the rules of immutable data, so when you extend it it gets copied, however if you are concerned about the security you can set a special flag on each security sensitive vectors and the library will ensure to erase each elements of these flagged ones when extending or squishing the vector or deleting it.

The library tries to hide the _vector datastructure so that the usage is pretty clean and as safe as possible in C.

## How do I use it?
To learn the API have a look at the vector.h file in src. To learn how to use it have a look at the Unit Test cod ein tests.

As general rules:

Add the vector.h to your C code with:

```
#include "vector.h"
```

When compile make sure you link your code to the libvector.a as shown in the Makefile for the Unit Tests (in `tests`).

Before you can use a vector you need to create one using the function `vector_create([initial elements], sizeof([your data structure]))`

One important note for beginners is that whenever you try to store som edata in the vector, please remember:

* If you have defined such data as a pointer then you can just use the data name, so, for example: `vector_add( myvector, myDataPointer )`
* If instead you have defined your data as a regular variable for example, then you need to pass it to the add function with an `&` before its name, so something like `vector_add( myvector, &myInt )`

Don't worry, in both cases the actual data contained in your reference will be copied (aka stored) in the vector, so if you free your reference or leave the function that defined it (as long as the vector scope is above such function), the data you've stored in the vector will persist.

## How do I build it?
if you have GCC installed then use the Makefile provided, I'll add more support for other compilers when I'll have time.

Wheneever you build it the Unit Tests will be executed as part of the build (I am a huge supported of tests automation, so if you are new to this you can use this library to check how powerful can be test automation and how they can help you to get faster at coding and releasing code!).


Thanks!
