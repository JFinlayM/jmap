#include "../inc/jmap.h"
#include <stdio.h>

int main(void){
    JMAP map;
    map = jmap.init_preset(JMAP_STRING_PRESET);
    JMAP_CHECK_RET;

    // Insert 10 values
    char *key = (char*)malloc(6*sizeof(char));
    char *value = (char*)malloc(8*sizeof(char));
    for (int i = 1; i <= 10; i++) {
        snprintf(key, 6, "key%d", i);
        snprintf(value, 8, "value%d", i);
        jmap.put(&map, key, &value);
        JMAP_CHECK_RET;

        printf("Inserted %s -> %s | size=%zu capacity=%zu\n", 
               key, value, map._length, map._capacity);
    }
    free(key);
    free(value);

    // Retrieve and print them
    printf("\n=== Retrieving values ===\n");
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);

        char **value = (char**)jmap.get(&map, key);
        JMAP_CHECK_RET_RETURN;
        printf("%s -> %s\n", key, *value);
    }

    // Test updating one key
    jmap.put(&map, "key5", JMAP_DIRECT_INPUT(char*,"hello keys5 !"));
    JMAP_CHECK_RET_RETURN;

    char ** updated = (char**)jmap.get(&map, "key5");
    JMAP_CHECK_RET_RETURN;
    printf("\nUpdated key5 -> %s\n", *updated);

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

    bool value_exists = jmap.contains_value(&map, JMAP_DIRECT_INPUT(char*,"value5"));
    JMAP_CHECK_RET_RETURN;
    printf("\nvalue5 exists: %s\n", value_exists ? "true" : "false");

    value_exists = jmap.contains_value(&map, JMAP_DIRECT_INPUT(char*,"value6"));
    JMAP_CHECK_RET_RETURN;
    printf("\nvalue6 exists: %s\n", value_exists ? "true" : "false");

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

    char **values = (char**)jmap.get_values(&map);
    JMAP_CHECK_RET_RETURN;
    
    printf("\nValues in the map:\n");
    for (size_t i = 0; i < map._length; i++) {
        printf("%s\n", values[i]);
        free(values[i]);
    }
    free(values);
    

    jmap.free(&map);

    return 0;
}