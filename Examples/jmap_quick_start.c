#include "../inc/jmap.h"
#include <stdio.h>

int main() {
    
    // Initialize
    JMAP map = jmap.init_preset(JMAP_INT_PRESET);
    JMAP_CHECK_RET_RETURN;
    
    // Add elements
    jmap.put(&map, "age", JMAP_DIRECT_INPUT(int, 25));
    JMAP_CHECK_RET_RETURN;
    
    jmap.put(&map, "score", JMAP_DIRECT_INPUT(int, 100));
    JMAP_CHECK_RET_RETURN;
    
    // Get value
    int ret = *(int*)jmap.get(&map, "age");
    JMAP_CHECK_RET_RETURN;
    printf("Age: %d\n", ret);
    
    jmap.print(&map);
    jmap.free(&map);
    return 0;
}