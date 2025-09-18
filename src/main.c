#include "../inc/jmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "third_party/murmur3-master/murmur3.h"

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

bool is_between(const char *key, const void *value, const void *ctx) {
    (void)ctx; // Unused context pointer
    (void)key; // Unused key pointer
    int val = *(int*)value;
    return val >= 20 && val <= 40;
}





int main(void) {
    JMAP map;

    // Initialize the map with int values
    JMAP_USER_CALLBACK_IMPLEMENTATION imp;
    imp.print_element_callback = print_element_callback;
    imp.is_equal_callback = is_equal_callback;
    jmap.init(&map, sizeof(int), JMAP_TYPE_VALUE, imp);
    JMAP_CHECK_RET_RETURN;

    printf("Initial capacity: %zu\n", map._length);

    // Insert 10 values
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        jmap.put(&map, key, JMAP_DIRECT_INPUT(int, i * 10));
        JMAP_CHECK_RET;

        printf("Inserted %s -> %d | size=%zu capacity=%zu\n", 
               key, i * 10, map._length, map._capacity);
    }

    // Retrieve and print them
    printf("\n=== Retrieving values ===\n");
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);

        int value = *(int*)jmap.get(&map, key);
        JMAP_CHECK_RET_RETURN;
        printf("%s -> %d\n", key, value);
    }

    // Test updating one key
    jmap.put(&map, "key5", JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET_RETURN;

    int updated = *(int*)jmap.get(&map, "key5");
    JMAP_CHECK_RET_RETURN;
    printf("\nUpdated key5 -> %d\n", updated);

    printf("\nFinal size: %zu, capacity: %zu\n", map._length, map._capacity);
    // Print the entire map
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    JMAP clone = jmap.clone(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nCloned map:\n");
    jmap.print(&clone);
    JMAP_CHECK_RET_RETURN;
    jmap.clear(&clone);
    JMAP_CHECK_RET_RETURN;
    jmap.print(&clone);
    JMAP_CHECK_RET; // Should print error

    bool is_empty = jmap.is_empty(&clone);
    JMAP_CHECK_RET_RETURN;
    printf("\nClone empty ? %s\n", is_empty ? "true" : "false");
    jmap.free(&clone);

    bool exists = jmap.contains_key(&map, "key5");
    JMAP_CHECK_RET_RETURN;
    printf("\nKey 'key5' exists: %s\n", exists ? "true" : "false");

    exists = jmap.contains_key(&map, "key0");
    JMAP_CHECK_RET_RETURN;
    printf("\nKey 'key0' exists: %s\n", exists ? "true" : "false");

    bool value_exists = jmap.contains_value(&map, JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET_RETURN;
    printf("\nValue 999 exists: %s\n", value_exists ? "true" : "false");

    value_exists = jmap.contains_value(&map, JMAP_DIRECT_INPUT(int, 1000));
    JMAP_CHECK_RET_RETURN;
    printf("\nValue 1000 exists: %s\n", value_exists ? "true" : "false");

    char **keys = jmap.get_keys(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nKeys in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        if (keys[i]) {
            printf("%s\n", keys[i]);
            free(keys[i]); // Free each key after use
        }
    }
    free(keys); // Free the keys array itself

    int *values = (int*)jmap.get_values(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nValues in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        printf("%d\n", values[i]);
    }
    free(values); // Free the values array

    printf("\n=== For each element, divide by two ===\n");
    jmap.for_each(&map, divide_by_two_callback, NULL);
    JMAP_CHECK_RET_RETURN;
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    printf("\n=== Put if absent ===\n");
    jmap.put_if_absent(&map, "key5", JMAP_DIRECT_INPUT(int, 500));
    JMAP_CHECK_RET;
    printf("\nPut if absent on key5 (should not change):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.put_if_absent(&map, "key11", JMAP_DIRECT_INPUT(int, 1100));
    JMAP_CHECK_RET_RETURN;
    printf("\nPut if absent on key11 (should add new key):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove(&map, "key11");
    JMAP_CHECK_RET_RETURN;
    printf("\nAfter removing key11:\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove_if_value_match(&map, "key5", JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET;
    printf("\nAfter removing key5 with value 999 (should not remove):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove_if_value_match(&map, "key5", JMAP_DIRECT_INPUT(int, 499));
    JMAP_CHECK_RET_RETURN;
    printf("\nAfter removing key5 with value 499 (should remove):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove_if_value_not_match(&map, "key8", JMAP_DIRECT_INPUT(int, 40));
    JMAP_CHECK_RET;
    printf("\nAfter removing key8 with value not matching 40 (should not remove):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove_if_value_not_match(&map, "key8", JMAP_DIRECT_INPUT(int, 42));
    JMAP_CHECK_RET_RETURN;
    printf("\nAfter removing key8 with value not matching 42 (should remove):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    jmap.remove_if(&map, is_between, NULL);
    JMAP_CHECK_RET;
    printf("\nAfter removing elements between 20 and 40 (included):\n");
    jmap.print(&map);
    JMAP_CHECK_RET_RETURN;

    JMAP jmap_data;
    jmap.init(&jmap_data, sizeof(int), JMAP_TYPE_VALUE, imp);
    JMAP_CHECK_RET_RETURN;

    jmap.clear(&jmap_data);
    JMAP_CHECK_RET_RETURN;

    uint32_t hash;
    size_t n = 1000000;
    int random_data[n];
    for (size_t i = 0; i < n; i++) {
        char key[10];
        snprintf(key, sizeof(key), "%d", (int)i);
        MurmurHash3_x86_32(key, strlen(key), 42, &hash);
        random_data[i] = hash & 0x3FF;
    }



    for (size_t i = 0; i < n; i++) {
        char key[10];
        snprintf(key, sizeof(key), "%d", random_data[(int)i]);
        void * pvalue = jmap.get(&jmap_data, key);
        if (jmap_last_error_trace.error_code == JMAP_ELEMENT_NOT_FOUND) {
            jmap.put(&jmap_data, key, JMAP_DIRECT_INPUT(int, 1));
            JMAP_CHECK_RET_RETURN;
        } else if (jmap_last_error_trace.has_error) {
            JMAP_CHECK_RET_RETURN;
        }
        else {
            int value = *(int*)pvalue;
            value++;
            jmap.put(&jmap_data, key, JMAP_DIRECT_INPUT(int, value));
            JMAP_CHECK_RET_RETURN;
        }
    }

    printf("\n=== Final JMAP Data ===\n");

    jmap.print(&jmap_data);
    JMAP_CHECK_RET_RETURN;

    // cleanup
    jmap.free(&map);
    jmap.free(&jmap_data);

    return 0;
}
