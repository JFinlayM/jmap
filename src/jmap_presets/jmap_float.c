#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%.2f ", JMAP_GET_VALUE(const float, x));
}

static char *element_to_string_array_callback(const void *x){
    float value = JMAP_GET_VALUE(const float, x);
    // Allocate enough space for the string representation
    char *str = (char*)malloc(16*sizeof(char));
    if (str) {
        snprintf(str, 16, "%.2f", value);
    }
    return str;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(const float, x) == JMAP_GET_VALUE(const float, y);
}


JMAP create_map_float(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(float), JMAP_TYPE_VALUE, imp);
    return array;
}