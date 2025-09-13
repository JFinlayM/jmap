#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%d ", JMAP_GET_VALUE(const int, x));
}

static char *element_to_string_array_callback(const void *x){
    int value = JMAP_GET_VALUE(const int, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(12*sizeof(char)); // Enough for 32-bit int
    if (str) {
        snprintf(str, 12, "%d", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const int, x) == JMAP_GET_VALUE(const int, y);
}


JMAP create_map_int(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(int), JMAP_TYPE_VALUE, imp);
    return array;
}