#include "jmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void print_element_callback(const void *value) {
    if (value) {
        printf("%d", *(int*)value);
    }
}

void divide_by_two_callback(const char *key, void *value, const void *ctx) {
    (void)ctx; // Unused context pointer
    (void)key; // Unused key pointer
    int *int_value = (int*)value;
    *int_value /= 2;
}

bool is_equal_callback(const void *a, const void *b) {
    return *(int*)a == *(int*)b;
}

int main(void) {
    JMAP map;
    JMAP_RETURN ret;

    // Initialize the map with int values
    ret = jmap.init(&map, sizeof(int));
    CHECK_RET_FREE(ret);
    map.user_implementation.print_element_callback = print_element_callback;
    map.user_implementation.is_equal_callback = is_equal_callback;

    printf("Initial capacity: %zu\n", map._length);

    // Insert 10 values
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        ret = jmap.put(&map, key, TO_POINTER(int, i * 10));
        CHECK_RET_FREE(ret);

        printf("Inserted %s -> %d | size=%zu capacity=%zu\n", 
               key, i * 10, map._length, map._capacity);
    }

    // Retrieve and print them
    printf("\n=== Retrieving values ===\n");
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);

        ret = jmap.get(&map, key);
        CHECK_RET(ret);

        int value = RET_GET_VALUE(int, ret);
        printf("%s -> %d\n", key, value);
    }

    // Test updating one key
    ret = jmap.put(&map, "key5", TO_POINTER(int, 999));
    CHECK_RET_FREE(ret);

    ret = jmap.get(&map, "key5");
    CHECK_RET(ret);
    int updated = RET_GET_VALUE(int, ret);
    printf("\nUpdated key5 -> %d\n", updated);

    printf("\nFinal size: %zu, capacity: %zu\n", map._length, map._capacity);
    // Print the entire map
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    ret = jmap.clone(&map);
    CHECK_RET(ret);
    JMAP *clone = RET_GET_POINTER(JMAP, ret);
    printf("\nCloned map:\n");
    ret = jmap.print(clone);
    CHECK_RET_FREE(ret);
    ret = jmap.clear(clone);
    CHECK_RET_FREE(ret);
    ret = jmap.print(clone);
    CHECK_RET_CONTINUE_FREE(ret); // Should print error

    ret = jmap.is_empty(clone);
    CHECK_RET(ret);
    bool is_empty = RET_GET_VALUE(bool, ret);
    printf("\nClone empty ? %s\n", is_empty ? "true" : "false");
    jmap.free(clone);

    ret = jmap.contains_key(&map, "key5");
    CHECK_RET(ret);
    bool exists = RET_GET_VALUE_SAFE(bool, ret, false);
    printf("\nKey 'key5' exists: %s\n", exists ? "true" : "false");

    ret = jmap.contains_key(&map, "key0");
    CHECK_RET(ret);
    exists = RET_GET_VALUE_SAFE(bool, ret, false);
    printf("\nKey 'key0' exists: %s\n", exists ? "true" : "false");

    ret = jmap.contains_value(&map, TO_POINTER(int, 999));
    CHECK_RET(ret);
    bool value_exists = RET_GET_VALUE_SAFE(bool, ret, false);
    printf("\nValue 999 exists: %s\n", value_exists ? "true" : "false");

    ret = jmap.contains_value(&map, TO_POINTER(int, 1000));
    CHECK_RET(ret);
    value_exists = RET_GET_VALUE_SAFE(bool, ret, false);
    printf("\nValue 1000 exists: %s\n", value_exists ? "true" : "false");

    ret = jmap.get_keys(&map);
    CHECK_RET(ret);
    char **keys = RET_GET_POINTER(char*, ret);
    printf("\nKeys in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        if (keys[i]) {
            printf("%s\n", keys[i]);
            free(keys[i]); // Free each key after use
        }
    }
    free(keys); // Free the keys array itself

    ret = jmap.get_values(&map);
    CHECK_RET(ret);
    int *values = RET_GET_POINTER(int, ret);
    printf("\nValues in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        printf("%d\n", values[i]);
    }
    free(values); // Free the values array

    printf("\n=== For each element, divide by two ===\n");
    ret = jmap.for_each(&map, divide_by_two_callback, NULL);
    CHECK_RET_FREE(ret);
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    printf("\n=== Put if absent ===\n");
    ret = jmap.put_if_absent(&map, "key5", TO_POINTER(int, 500));
    CHECK_RET_CONTINUE(ret);
    printf("\nPut if absent on key5 (should not change):\n");
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    ret = jmap.put_if_absent(&map, "key11", TO_POINTER(int, 1100));
    CHECK_RET(ret);
    printf("\nPut if absent on key11 (should add new key):\n");
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    ret = jmap.remove(&map, "key11");
    CHECK_RET(ret);
    printf("\nAfter removing key11:\n");
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_match(&map, "key5", TO_POINTER(int, 999));
    CHECK_RET_CONTINUE(ret);
    printf("\nAfter removing key5 with value 999 (should not remove):\n");
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_match(&map, "key5", TO_POINTER(int, 499));
    CHECK_RET(ret);
    printf("\nAfter removing key5 with value 499 (should remove):\n");
    ret = jmap.print(&map);
    CHECK_RET_FREE(ret);


    // cleanup
    jmap.free(&map);

    return 0;
}
