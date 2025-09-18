#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%hu ", JMAP_GET_VALUE(const unsigned short, x));
}

static char *element_to_string_array_callback(const void *x){
    unsigned short value = JMAP_GET_VALUE(const unsigned short, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(6*sizeof(char));
    if (str) {
        snprintf(str, 6, "%hu", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const unsigned short, x) == JMAP_GET_VALUE(const unsigned short, y);
}


JMAP create_map_ushort(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string_callback = element_to_string_array_callback;
    imp.is_equal_callback = is_equal_array_callback;
    jmap.init(&array, sizeof(unsigned short), JMAP_TYPE_VALUE, imp);
    return array;
}