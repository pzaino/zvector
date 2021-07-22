#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vector.h"

// Some globals we'll need in
// one of the tests later:
char test_name[255];
float test_speed;
int test_year;

typedef struct Car
{
    char name[255];
    float speed;
    uint32_t year;
} car;

void add_a_car(vector v)
{
    car car3;
    strcpy(car3.name, "Hennessey Venom GT");
    car3.year = 2010;
    car3.speed = 270;

    vect_add(v, &car3);
    assert(vect_size(v) == 3);

    // Use the following globals to store
    // the car data for future comparisions
    // with the data stored in the vector:
    strcpy(test_name, car3.name);
    test_speed = car3.speed;
    test_year = car3.year;
}

int main()
{
    printf("=== UTest002 ===\n");

    printf("1st Test: Create a vector of 2 elements and using Car for the vector data:\n");
    vector v;
    v = vect_create(2, sizeof(car), true);
    printf("done.\n");

    printf("2nd Test: Insert 3 elements of type Car, then free the original car element and check if they are stored correctly:\n");
    car car1;
    strcpy(car1.name, "Daimler-Benz");
    car1.year = 1939;
    car1.speed = 394.71;

    car car2;
    strcpy(car2.name, "Koenigsegg One:1");
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

    // Let's add another car:
    vect_add(v, &car2);
    // Let's check if the vector size has grown correctly:
    assert(vect_size(v) == 2);

    // Get last car added
    car carY = *((car *)vect_get(v));
    printf("2nd Car added name: %s, year: %d, speed: %f\n", carY.name, carY.year, carY.speed);

    // Add the 3rd car from a fuction where the func
    // will create a car on the stack and then we'll
    // check the vector for the old values in this
    // funct.
    add_a_car(v);

    printf("done.\n");

    printf("3rd Test: Extract a car from the vector:\n");
    car *carX;
    carX = (car *)vect_get_at(v, 2);
    printf("done.\n");

    printf("4th Test: Display the content of the extracted Car:\n");
    printf("Car name: %s, year: %d, speed: %f\n", carX->name, carX->year, carX->speed);
    assert(!strcmp(carX->name, test_name));
    assert(carX->year == test_year);
    assert(carX->speed == test_speed);
    printf("done.\n");

    printf("5th Test: Clear vector:\n");
    vect_clear(v);
    printf("done.\n");

    printf("6th Test: Check if vector size is now 0 (zero):\n");
    assert(vect_size(v) == 0);
    printf("done.\n");

    printf("7th Test: destroy the vector:\n");
    vect_destroy(v);
    printf("done.\n");

    return 0;
}
