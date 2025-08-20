#include "jmap.h"
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

int compare_keys(const void *key_a, const void *value_a, const void *key_b, const void *value_b, const void *ctx) {
    (void)ctx;      // unused
    (void)value_a;  // unused
    (void)value_b;  // unused

    int int_a = atoi((const char*)key_a);
    int int_b = atoi((const char*)key_b);

    if (int_a < int_b) return -1;
    if (int_a > int_b) return 1;
    return 0;
}


int main(void) {
    JMAP map;
    JMAP_RETURN ret;

    // Initialize the map with int values
    ret = jmap.init(&map, sizeof(int));
    JMAP_CHECK_RET_FREE(ret);
    map.user_implementation.print_element_callback = print_element_callback;
    map.user_implementation.is_equal_callback = is_equal_callback;

    printf("Initial capacity: %zu\n", map._length);

    // Insert 10 values
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        ret = jmap.put(&map, key, JMAP_DIRECT_INPUT(int, i * 10));
        JMAP_CHECK_RET_FREE(ret);

        printf("Inserted %s -> %d | size=%zu capacity=%zu\n", 
               key, i * 10, map._length, map._capacity);
    }

    // Retrieve and print them
    printf("\n=== Retrieving values ===\n");
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);

        ret = jmap.get(&map, key);
        JMAP_CHECK_RET(ret);

        int value = JMAP_RET_GET_VALUE(int, ret);
        printf("%s -> %d\n", key, value);
    }

    // Test updating one key
    ret = jmap.put(&map, "key5", JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.get(&map, "key5");
    JMAP_CHECK_RET(ret);
    int updated = JMAP_RET_GET_VALUE(int, ret);
    printf("\nUpdated key5 -> %d\n", updated);

    printf("\nFinal size: %zu, capacity: %zu\n", map._length, map._capacity);
    // Print the entire map
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.clone(&map);
    JMAP_CHECK_RET(ret);
    JMAP *clone = JMAP_RET_GET_POINTER(JMAP, ret);
    printf("\nCloned map:\n");
    ret = jmap.print(clone);
    JMAP_CHECK_RET_FREE(ret);
    ret = jmap.clear(clone);
    JMAP_CHECK_RET_FREE(ret);
    ret = jmap.print(clone);
    JMAP_CHECK_RET_CONTINUE(ret); // Should print error

    ret = jmap.is_empty(clone);
    JMAP_CHECK_RET(ret);
    bool is_empty = JMAP_RET_GET_VALUE_FREE(bool, ret);
    printf("\nClone empty ? %s\n", is_empty ? "true" : "false");
    jmap.free(clone);

    ret = jmap.contains_key(&map, "key5");
    JMAP_CHECK_RET(ret);
    bool exists = JMAP_RET_GET_VALUE_FREE(bool, ret);
    printf("\nKey 'key5' exists: %s\n", exists ? "true" : "false");

    ret = jmap.contains_key(&map, "key0");
    JMAP_CHECK_RET(ret);
    exists = JMAP_RET_GET_VALUE_FREE(bool, ret);
    printf("\nKey 'key0' exists: %s\n", exists ? "true" : "false");

    ret = jmap.contains_value(&map, JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET(ret);
    bool value_exists = JMAP_RET_GET_VALUE_FREE(bool, ret);
    printf("\nValue 999 exists: %s\n", value_exists ? "true" : "false");

    ret = jmap.contains_value(&map, JMAP_DIRECT_INPUT(int, 1000));
    JMAP_CHECK_RET(ret);
    value_exists = JMAP_RET_GET_VALUE_FREE(bool, ret);
    printf("\nValue 1000 exists: %s\n", value_exists ? "true" : "false");

    ret = jmap.get_keys(&map);
    JMAP_CHECK_RET(ret);
    char **keys = JMAP_RET_GET_POINTER(char*, ret);
    printf("\nKeys in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        if (keys[i]) {
            printf("%s\n", keys[i]);
            free(keys[i]); // Free each key after use
        }
    }
    free(keys); // Free the keys array itself

    ret = jmap.get_values(&map);
    JMAP_CHECK_RET(ret);
    int *values = JMAP_RET_GET_POINTER(int, ret);
    printf("\nValues in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        printf("%d\n", values[i]);
    }
    free(values); // Free the values array

    printf("\n=== For each element, divide by two ===\n");
    ret = jmap.for_each(&map, divide_by_two_callback, NULL);
    JMAP_CHECK_RET_FREE(ret);
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    printf("\n=== Put if absent ===\n");
    ret = jmap.put_if_absent(&map, "key5", JMAP_DIRECT_INPUT(int, 500));
    JMAP_CHECK_RET_CONTINUE(ret);
    printf("\nPut if absent on key5 (should not change):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.put_if_absent(&map, "key11", JMAP_DIRECT_INPUT(int, 1100));
    JMAP_CHECK_RET(ret);
    printf("\nPut if absent on key11 (should add new key):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove(&map, "key11");
    JMAP_CHECK_RET(ret);
    printf("\nAfter removing key11:\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_match(&map, "key5", JMAP_DIRECT_INPUT(int, 999));
    JMAP_CHECK_RET_CONTINUE(ret);
    printf("\nAfter removing key5 with value 999 (should not remove):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_match(&map, "key5", JMAP_DIRECT_INPUT(int, 499));
    JMAP_CHECK_RET(ret);
    printf("\nAfter removing key5 with value 499 (should remove):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_not_match(&map, "key8", JMAP_DIRECT_INPUT(int, 40));
    JMAP_CHECK_RET_CONTINUE(ret);
    printf("\nAfter removing key8 with value not matching 40 (should not remove):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove_if_value_not_match(&map, "key8", JMAP_DIRECT_INPUT(int, 42));
    JMAP_CHECK_RET(ret);
    printf("\nAfter removing key8 with value not matching 42 (should remove):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    ret = jmap.remove_if(&map, is_between, NULL);
    JMAP_CHECK_RET_CONTINUE(ret);
    printf("\nAfter removing elements between 20 and 40 (included):\n");
    ret = jmap.print(&map);
    JMAP_CHECK_RET_FREE(ret);

    JMAP jmap_data;
    ret = jmap.init(&jmap_data, sizeof(int));
    JMAP_CHECK_RET_FREE(ret);
    jmap_data.user_implementation.print_element_callback = print_element_callback;
    jmap_data.user_implementation.is_equal_callback = is_equal_callback;

    ret = jmap.clear(&jmap_data);
    JMAP_CHECK_RET_FREE(ret);

    uint32_t hash;
    size_t n = 1000000;
    int random_data[n];
    for (int i = 0; i < n; i++) {
        char key[10];
        snprintf(key, sizeof(key), "%d", i);
        MurmurHash3_x86_32(key, strlen(key), 42, &hash);
        random_data[i] = hash & 0x3FF;
    }



    for (int i = 0; i < n; i++) {
        char key[6];
        snprintf(key, sizeof(key), "%d", random_data[i]);
        ret = jmap.get(&jmap_data, key);
        if (ret.error.error_code == JMAP_ELEMENT_NOT_FOUND) {
            ret = jmap.put(&jmap_data, key, JMAP_DIRECT_INPUT(int, 1));
            JMAP_CHECK_RET(ret);
        } else if (ret.has_error) {
            JMAP_CHECK_RET_FREE(ret);
        }
        else {
            int value = JMAP_RET_GET_VALUE(int, ret);
            value++;
            ret = jmap.put(&jmap_data, key, JMAP_DIRECT_INPUT(int, value));
            JMAP_CHECK_RET(ret);
        }
    }

    printf("\n=== Final JMAP Data ===\n");

    ret = jmap.get(&jmap_data, "500");
    JMAP_CHECK_RET(ret);
    int value = JMAP_RET_GET_VALUE(int, ret);
    printf("\nValue for key '500': %d\n", value);
    ret = jmap.sort(&jmap_data, compare_keys, NULL);
    JMAP_CHECK_RET_FREE(ret);
    ret = jmap.print(&jmap_data);
    JMAP_CHECK_RET_FREE(ret);

    jmap.free(&jmap_data);

    /*int *keys_array = NULL;
    int *values_array = NULL;
    size_t count;
    
    if (jmap.histogram(random_data, n, &keys_array, &values_array, &count) != EXIT_SUCCESS) {
        fprintf(stderr, "Error creating histogram\n");
        return EXIT_FAILURE;
    }
    
    printf("\n=== Histogram ===\n");
    for (size_t i = 0; i < count; i++) {
        printf("Key: %d, Value: %d\n", keys_array[i], values_array[i]);
    }
    
    free(keys_array);
    free(values_array);*/
    

    // cleanup
    jmap.free(&map);

    return 0;
}
