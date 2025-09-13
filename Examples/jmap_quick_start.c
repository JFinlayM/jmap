#include "../inc/jmap.h"
#include <stdio.h>

int main() {
    
    // Initialize
    JMAP map = jmap_init_preset(JMAP_INT_PRESET);
    JMAP_CHECK_RET_RETURN;
    
    // Add elements
    jmap_put(&map, "age", 25);
    JMAP_CHECK_RET_RETURN;
    
    jmap_put(&map, "score", 100);
    JMAP_CHECK_RET_RETURN;
    
    // Get value
    int ret = *(int*)jmap_get(&map, "age");
    JMAP_CHECK_RET_RETURN;
    printf("Age: %d\n", ret);
    
    jmap_print(&map);
    jmap_free(&map);
    return 0;
}