#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%lu ", JMAP_GET_VALUE(const unsigned long, x));
}

static char *element_to_string_array_callback(const void *x){
    unsigned long value = JMAP_GET_VALUE(const unsigned long, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(21*sizeof(char));
    if (str) {
        snprintf(str, 21, "%lu", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const unsigned long, x) == JMAP_GET_VALUE(const unsigned long, y);
}


JMAP create_jmap_ulong(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(unsigned long), JMAP_TYPE_VALUE, imp);
    return array;
}