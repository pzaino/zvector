#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vector.h"

typedef struct Car
{
    char name[255];
    float speed;
    uint32_t year;
} car;

int main()
{
    printf("=== UTest002 ===\n");

    printf("1st Test: Create a vector of 2 elements and using Car for the vector data:\n");
    vector v;
    v = vector_create(2, sizeof(car), true);
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

    car car3;
    strcpy(car3.name, "Hennessey Venom GT");
    car3.year = 2010;
    car3.speed = 270;

    vector_add(v, &car1);
    assert(vector_size(v) == 1);
    assert(vector_get(v, 0) == &car1);

    vector_add(v, &car2);
    assert(vector_size(v) == 2);
    assert(vector_get(v, 1) == &car2);

    vector_add(v, &car3);
    assert(vector_size(v) == 3);
    assert(vector_get(v, 2) == &car3);

    char test_name[255];
    strcpy(test_name, car3.name);
    float test_speed = car3.speed;
    int test_year = car3.year;

    printf("done.\n");

    printf("3rd Test: Extract a car from the vector:\n");
    car *carX;
    carX = (car *)vector_get(v, 2);
    printf("done.\n");

    printf("4th Test: Display the content of the extracted Car:\n");
    printf("Car name: %s, year: %d, speed: %f\n", carX->name, carX->year, carX->speed);
    assert(!strcmp(carX->name, test_name));
    assert(carX->year == test_year);
    assert(carX->speed == test_speed);
    printf("done.\n");

    printf("5th Test: Clear vector:\n");
    vector_clear(v);
    printf("done.\n");

    printf("6th Test: Check if vector size is now 0 (zero):\n");
    assert(vector_size(v) == 0);
    printf("done.\n");

    printf("7th Test: destroy the vector:\n");
    vector_destroy(v);
    printf("done.\n");

    return 0;
}
