/*
 *    Name: UTest002
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#define __STDC_WANT_LIB_EXT1__
#define UNUSED(x) (void)x

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if (  defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

// Some globals we'll need in
// one of the tests later:
char test_name[255];
float test_speed;
uint32_t test_year;

typedef struct Car
{
    char name[255];
    float speed;
    uint32_t year;
} car;

void clear_str(char *str, uint32_t numchars)
{
    memset(str, 0, numchars);
}

void strCopy(char *dst, const char *src, size_t len)
{
#if ( OS_TYPE == 1 )
    snprintf(dst, len + 1, "%s", src);
#elif ( OS_TYPE == 2 )
    strcpy_s(dst, 18, src);
#else
    strcpy(dst, src);
    UNUSED(len);
#endif
}

void add_a_car(vector v)
{
    car car3;
    clear_str(car3.name, 255); // Just to make sure the string is empty!
    strCopy(car3.name, "Hennessey Venom GT", 18);
    car3.year = 2010;
    car3.speed = 270;

    vect_add(v, &car3);
    assert(vect_size(v) == 3);

    // Use the following globals to store
    // the car data for future comparisions
    // with the data stored in the vector:
    strCopy(test_name, car3.name, sizeof(car3.name));
    test_speed = car3.speed;
    test_year = car3.year;
}

int main()
{
    // Setup tests:
    char *testGrp = "002";
    uint8_t testID = 1;

    printf("=== UTest%s ===\n", testGrp);
    printf("Testing vectors with complex data structures\n");

    fflush(stdout);

    printf("Test %s_%d: Create a vector of 2 elements and using Car for the vector data:\n", testGrp, testID);
    vector v;
    v = vect_create(2, sizeof(car), ZV_SAFE_WIPE);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Insert 3 elements of type Car and check if they are stored correctly (for the 3rd car use an external function and data stored on the stack):\n", testGrp, testID);
    car car1;
    clear_str(car1.name, 255); // Just to make sure the string is empty!
    strCopy(car1.name, "Daimler-Benz", 12);
    car1.year = 1939;
    car1.speed = (float)394.71;

    car car2;
    clear_str(car2.name, 255); // Just to make sure the string is empty!
    strCopy(car2.name, "Koenigsegg One:1", 16);
    car2.year = 2014;
    car2.speed = 273;

    // Let's add a new car in the vector:
    vect_add(v, &car1);
    // Let's check if the vector size has grown correctly:
    assert(vect_size(v) == 1);
    // Let's retrieve the car we have just inserted:
    car carA = *((car *)vect_get_at(v, 0));
    // Let's check if the name is the same as the original car we have inserted:
    assert(!strcmp(carA.name, car1.name));
    printf("1st Car added name: %s, year: %d, speed: %f\n", carA.name, carA.year, carA.speed);

    // Let's add another car:
    vect_add(v, &car2);
    // Let's check if the vector size has grown correctly:
    assert(vect_size(v) == 2);

    // Get last car added
    car carB = *((car *)vect_get(v));
    printf("2nd Car added name: %s, year: %d, speed: %f\n", carB.name, carB.year, carB.speed);

    // Add the 3rd car from a fuction where the func
    // will create a car on the stack and then we'll
    // check the vector for the old values in this
    // funct.
    printf("Now adding the 3rd car from a separate function, with data allocated on the stack...\n");
    add_a_car(v);

    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Extract last car from the vector and verify if it's correct:\n", testGrp, testID);
    // carC is defined as a point and so we do not need the extra *() around (car *)vect_get_at()
    car *carC = (car *)vect_get(v);
    printf("Car name: %s, year: %d, speed: %f\n", carC->name, carC->year, carC->speed);
    assert(!strcmp(carC->name, test_name));
    assert(carC->year == test_year);
    assert(carC->speed == test_speed);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Swap 1st vector element with last, show the results and check if it's correct:\n", testGrp, testID);
    vect_swap(v, 0, vect_size(v) - 1);

    car *carEnd;
    carEnd = (car *)vect_get(v);
    printf("Car name: %s, year: %d, speed: %f\n", carEnd->name, carEnd->year, carEnd->speed);
    assert(!strcmp(car1.name, carEnd->name));
    assert(car1.year == carEnd->year);
    assert(car1.speed == carEnd->speed);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Clear vector:\n", testGrp, testID);
    vect_clear(v);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Check if vector size is now 0 (zero):\n", testGrp, testID);
    assert(vect_size(v) == 0);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: destroy the vector:\n", testGrp, testID);
    vect_destroy(v);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("================\n\n");

    return 0;
}
