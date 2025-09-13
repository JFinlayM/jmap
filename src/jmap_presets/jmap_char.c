#include "../../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print_element_array_callback(const void *x){
    printf("%c ", JMAP_GET_VALUE(char, x));
}

static char *element_to_string_array_callback(const void *x){
    char val = JMAP_GET_VALUE(char, x);
    char *buf = (char*)malloc(2*sizeof(char)); 
    snprintf(buf, 2, "%c", val);
    return buf;
}

static bool is_equal_array_callback(const void *x, const void *y){
    return JMAP_GET_VALUE(char, x) == JMAP_GET_VALUE(char, y);
}


JMAP create_map_char(void){
    JMAP array;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    jmap.init(&array, sizeof(char), JMAP_TYPE_VALUE, imp);
    return array;
}