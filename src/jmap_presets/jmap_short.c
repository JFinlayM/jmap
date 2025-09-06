#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%hd ", JMAP_GET_VALUE(const short, x));
}

static char *element_to_string_array_callback(const void *x){
    short value = JMAP_GET_VALUE(const short, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(7*sizeof(char));
    if (str) {
        snprintf(str, 7, "%hd", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const short, x) == JMAP_GET_VALUE(const short, y);
}


JMAP create_jmap_short(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(short), JMAP_TYPE_VALUE, imp);
    return array;
}