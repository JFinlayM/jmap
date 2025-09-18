#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%u ", JMAP_GET_VALUE(const unsigned int, x));
}

static char *element_to_string_array_callback(const void *x){
    unsigned int value = JMAP_GET_VALUE(const unsigned int, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(11*sizeof(char));
    if (str) {
        snprintf(str, 11, "%u", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const unsigned int, x) == JMAP_GET_VALUE(const unsigned int, y);
}


JMAP create_map_uint(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string_callback = element_to_string_array_callback;
    imp.is_equal_callback = is_equal_array_callback;
    jmap.init(&array, sizeof(unsigned int), JMAP_TYPE_VALUE, imp);
    return array;
}