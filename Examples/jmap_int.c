#include "../inc/jmap.h"
#include <stdio.h>

int main(void){
    JMAP map;
    map = jmap_init_preset(JMAP_INT_PRESET);
    JMAP_CHECK_RET;

    // Insert 10 values
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        jmap_put(&map, key, i * 10);
        JMAP_CHECK_RET;

        printf("Inserted %s -> %d | size=%zu capacity=%zu\n", 
               key, i * 10, map._length, map._capacity);
    }

    // Retrieve and print them
    printf("\n=== Retrieving values ===\n");
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);

        int value = *(int*)jmap_get(&map, key);
        JMAP_CHECK_RET_RETURN;
        printf("%s -> %d\n", key, value);
    }

    // Test updating one key
    jmap_put(&map, "key5", 999);
    JMAP_CHECK_RET_RETURN;

    int updated = *(int*)jmap_get(&map, "key5");
    JMAP_CHECK_RET_RETURN;
    printf("\nUpdated key5 -> %d\n", updated);

    printf("\nFinal size: %zu, capacity: %zu\n", map._length, map._capacity);
    // Print the entire map
    jmap_print(&map);
    JMAP_CHECK_RET_RETURN;

    JMAP clone = jmap_clone(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nCloned map:\n");
    jmap_print(&clone);
    JMAP_CHECK_RET_RETURN;
    jmap_clear(&clone);
    JMAP_CHECK_RET_RETURN;
    jmap_print(&clone);
    JMAP_CHECK_RET; // Should print error

    bool is_empty = jmap_is_empty(&clone);
    JMAP_CHECK_RET_RETURN;
    printf("\nClone empty ? %s\n", is_empty ? "true" : "false");
    jmap_free(&clone);

    bool exists = jmap_contains_key(&map, "key5");
    JMAP_CHECK_RET_RETURN;
    printf("\nKey 'key5' exists: %s\n", exists ? "true" : "false");

    exists = jmap_contains_key(&map, "key0");
    JMAP_CHECK_RET_RETURN;
    printf("\nKey 'key0' exists: %s\n", exists ? "true" : "false");

    bool value_exists = jmap_contains_value(&map, 999);
    JMAP_CHECK_RET_RETURN;
    printf("\nValue 999 exists: %s\n", value_exists ? "true" : "false");

    value_exists = jmap_contains_value(&map, 1000);
    JMAP_CHECK_RET_RETURN;
    printf("\nValue 1000 exists: %s\n", value_exists ? "true" : "false");

    char **keys = jmap_get_keys(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nKeys in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        if (keys[i]) {
            printf("%s\n", keys[i]);
            free(keys[i]); // Free each key after use
        }
    }
    free(keys); // Free the keys array itself

    int *values = (int*)jmap_get_values(&map);
    JMAP_CHECK_RET_RETURN;
    printf("\nValues in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        printf("%d\n", values[i]);
    }
    free(values); // Free the values array

    jmap_free(&map);

    return 0;
}