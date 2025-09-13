#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%.2lf ", JMAP_GET_VALUE(const double, x));
}

static char *element_to_string_array_callback(const void *x){
    double value = JMAP_GET_VALUE(const double, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(32*sizeof(char));
    if (str) {
        snprintf(str, 32, "%.2lf", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const double, x) == JMAP_GET_VALUE(const double, y);
}


JMAP create_map_double(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(double), JMAP_TYPE_VALUE, imp);
    return array;
}