# Vector
This is a simple yet effective library to have dynamic vectors (dynamic arrays) in C.

## Introduction
I wrote this library for fun, after watching some presentation on the internet from different authors that were talking about dynamic arrays in standard C.

## How does it works?
It's very simple, it's an ANSI C99 library, no funky dependeces, so it should compile everywhere (let me know if you find any issue).

The vector follows the rules of immutable data, so when you extend it it gets copied, however if you are concerned about the security of this please keep reading.

The library tries to hide the _vector datastructure so that the usage is pretty clean and as safe as possible in C.

To learn the API have a look at the vector.h file in src. To learn hwo to use it have a look at the Unit Test cod ein tests.

One of the nice feature of this library is that you can store whatever you want in the vector, so yes you can use your own data structures and/or use standard base types.

Another nice featrue of the library is that when you add an element to the vector it gets copied in, so you can safely store datastructure elements that you have created as local (aka not using the heap).

Another nice feature is that you can set a vectore to be wiped (there is a flag for that), and when you do the library will wipe the data that are being removed or the entire old vector when a new vector is being created after an expansion.

I'll add more functions as I have time and also the possibility to keep the vector ordered.

## How do I build it?
if you have GCC installed then use the Makefile provided, I'll add more support for other compilers when I'll have time.

Thanks!