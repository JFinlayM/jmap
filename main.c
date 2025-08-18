#include "jmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void) {
    JMAP map;
    JMAP_RETURN ret;

    // Initialize the map with int values
    ret = jmap.init(&map, sizeof(int));
    CHECK_RET_FREE(ret);

    printf("Initial capacity: %zu\n", map._length);

    // Insert 10 values
    for (int i = 1; i <= 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key%d", i);
        ret = jmap.put(&map, key, TO_POINTER(int, i * 10));
        CHECK_RET_FREE(ret);

        printf("Inserted %s -> %d | size=%zu capacity=%zu\n", 
               key, i * 10, map._elem_nb, map._length);
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

    printf("\nFinal size: %zu, capacity: %zu\n", map._elem_nb, map._length);

    // cleanup
    jmap.free(&map);

    return 0;
}
