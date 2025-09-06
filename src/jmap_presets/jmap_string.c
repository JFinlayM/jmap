#include "../../inc/jmap.h"
#include <stdio.h>

static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}


static void print_element_array_callback(const void *x){
    char **str = (char**)x;
    printf("%s ", *str);
}

static char *element_to_string_array_callback(const void *x){
    char **str = (char**)x;
    return my_strdup(*str);
}

static bool is_equal_array_callback(const void *x, const void *y){
    char **str_x = (char**)x;
    char **str_y = (char**)y;
    int cmp = strcmp(*str_x, *str_y);
    if (cmp != 0) return false;
    return true;
}


static void *copy_elem_override(const void *x){
    char **str = (char**)x;
    char **res = (char**)malloc(sizeof(char**));
    *res = my_strdup(*str);
    return res;
}


JMAP create_jmap_string(void){
    JMAP map;
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_array_callback;
    imp.element_to_string = element_to_string_array_callback;
    imp.is_equal = is_equal_array_callback;
    imp.copy_elem_override = copy_elem_override;
    jmap.init(&map, sizeof(char*), JMAP_TYPE_POINTER, imp);
    return map;
}