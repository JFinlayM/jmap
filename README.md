# JMAP - Generic Hash Map for C

**Note**: I'm a beginner programmer, so don't take this code too seriously. This is a learning project!

A hash map library for C with key-value storage, iteration, and various utility functions. The library is designed to be generic, allowing storage of any data type by specifying the element size during initialization. 
It provides a variety of utility functions and supports user-defined callbacks for custom behavior. The functions return a struct containing either the result or an error code/message, which can be checked using provided macros. 
The functions are inspired by higher-level languages like JavaScript or Java but hopefully quicker...

## Building

```bash
sudo dpkg -i libjmap-dev_1.0.0-1_amd64.deb
sudo apt-get install -f
gcc main.c -o main -ljmap
```

## Quick Start

```c
#include <jmap.h>
#include <stdio.h>

void print_int(const void *x) {
    printf("%d ", JMAP_GET_VALUE(const int, x));
}

bool is_equal_int(const void *a, const void *b) {
    return JMAP_GET_VALUE(const int, a) == JMAP_GET_VALUE(const int, b);
}

int main() {
    JMAP map;
    JMAP_RETURN ret;
    
    // Initialize
    ret = jmap.init(&map, sizeof(int));
    if (JMAP_CHECK_RET_FREE(ret)) return EXIT_FAILURE; // Check for errors and return
    // Or just JMAP_CHECK_RET_FREE(ret) after function calls if you don't need to return
    // Or JMAP_CHECK_RET(ret) if you need the return value later
    // Or nothing but that creates memory leaks
    
    // Set callbacks
    map.user_implementation.print_element_callback = print_int;
    map.user_implementation.is_equal_callback = is_equal_int;
    
    // Add elements
    ret = jmap.put(&map, "age", JMAP_DIRECT_INPUT(int, 25));
    if (JMAP_CHECK_RET_FREE(ret)) return EXIT_FAILURE;
    
    ret = jmap.put(&map, "score", JMAP_DIRECT_INPUT(int, 100));
    if (JMAP_CHECK_RET_FREE(ret)) return EXIT_FAILURE;
    
    // Get value
    ret = jmap.get(&map, "age");
    if (!ret.has_error) {
        printf("Age: %d\n", JMAP_RET_GET_VALUE(int, ret));
        JMAP_FREE_RET_VALUE(ret);
    }
    
    jmap.print(&map);
    jmap.free(&map);
    return 0;
}
```
### Output

```bash
JMAP [size: 2] =>
age: 25 
score: 100 
Age: 25
```

## Core Functions

### Basic Operations
```c
jmap.init(&map, sizeof(int));                       // Initialize empty map
jmap.put(&map, "key", JMAP_DIRECT_INPUT(int, 42));  // Insert/update key-value pair
jmap.get(&map, "key");                               // Get value by key
jmap.contains_key(&map, "key");                      // Check if key exists
jmap.contains_value(&map, &value);                   // Check if value exists
jmap.remove(&map, "key");                            // Remove key-value pair
jmap.clear(&map);                                    // Remove all pairs
jmap.is_empty(&map);                                 // Check if map is empty
jmap.print(&map);                                    // Print map (needs print_element_callback)
jmap.clone(&map);                                    // Deep copy
jmap.resize(&map, new_size);                         // Resize hash table (must be power of 2)
jmap.free(&map);                                     // Free memory
```

### Advanced Operations
```c
jmap.get_keys(&map);                                 // Get array of all keys
jmap.get_values(&map);                               // Get array of all values
jmap.put_if_absent(&map, "key", &value);             // Insert only if key doesn't exist
jmap.remove_if_value_match(&map, "key", &value);     // Remove if value matches
jmap.remove_if_value_not_match(&map, "key", &value); // Remove if value doesn't match
jmap.remove_if(&map, predicate, ctx);                // Remove pairs matching predicate
jmap.for_each(&map, callback, ctx);                  // Apply function to each pair
```

## Examples

### Basic Usage
```c
JMAP map;
jmap.init(&map, sizeof(int));

// Insert values
jmap.put(&map, "apple", JMAP_DIRECT_INPUT(int, 5));
jmap.put(&map, "banana", JMAP_DIRECT_INPUT(int, 3));
jmap.put(&map, "orange", JMAP_DIRECT_INPUT(int, 8));

// Check if key exists
JMAP_RETURN ret = jmap.contains_key(&map, "apple");
if (!ret.has_error && JMAP_RET_GET_VALUE(bool, ret)) {
    printf("Apple found!\n");
}
JMAP_FREE_RET(ret);
```

### Iterating Over Pairs
```c
void print_pair(const char *key, void *value, const void *ctx) {
    (void)ctx; // Unused
    printf("%s: %d\n", key, JMAP_GET_VALUE(int, value));
}

jmap.for_each(&map, print_pair, NULL);
```

### Conditional Removal
```c
bool remove_low_values(const char *key, const void *value, const void *ctx) {
    (void)key; // Unused
    int threshold = *(int*)ctx;
    return JMAP_GET_VALUE(const int, value) < threshold;
}

int threshold = 5;
jmap.remove_if(&map, remove_low_values, &threshold);
```

### Working with Keys and Values
```c
// Get all keys
JMAP_RETURN ret = jmap.get_keys(&map);
if (!ret.has_error) {
    char **keys = JMAP_RET_GET_POINTER(char*, ret);
    // Use keys array...
    // Free each key and the array when done
    for (size_t i = 0; i < map._length; i++) {
        free(keys[i]);
    }
    free(keys);
}

// Get all values
ret = jmap.get_values(&map);
if (!ret.has_error) {
    void *values = JMAP_RET_GET_POINTER(void, ret);
    // Use values array (size is map._length * map._elem_size)
    free(values);
}
```

### Custom Types
```c
typedef struct { int x, y; } Point;

void print_point(const void *p) {
    Point pt = JMAP_GET_VALUE(const Point, p);
    printf("(%d,%d) ", pt.x, pt.y);
}

bool points_equal(const void *a, const void *b) {
    Point pa = JMAP_GET_VALUE(const Point, a);
    Point pb = JMAP_GET_VALUE(const Point, b);
    return pa.x == pb.x && pa.y == pb.y;
}

JMAP points_map;
jmap.init(&points_map, sizeof(Point));
points_map.user_implementation.print_element_callback = print_point;
points_map.user_implementation.is_equal_callback = points_equal;

Point p = {3, 4};
jmap.put(&points_map, "origin", &p);
```

### Safe Operations
```c
// Put only if key doesn't exist
ret = jmap.put_if_absent(&map, "new_key", JMAP_DIRECT_INPUT(int, 42));
if (!ret.has_error) {
    printf("Key was added\n");
} else {
    printf("Key already exists\n");
}

// Remove only if value matches
int expected_value = 10;
ret = jmap.remove_if_value_match(&map, "key", &expected_value);
if (!ret.has_error) {
    printf("Key removed because value matched\n");
}
```

## Required Callbacks

Set these before using related functions:
```c
map.user_implementation.print_element_callback = print_func;     // For print()
map.user_implementation.print_error_callback = error_func;      // For custom error printing
map.user_implementation.is_equal_callback = equal_func;         // For contains_value(), remove_if_value_*()
map.user_implementation.compare_pairs_callback = compare_func;  // For sorting operations (future use)
```

## Configuration

The JMAP structure includes several configuration options:
```c
map._load_factor = 0.75f;        // Resize when 75% full (default)
map._key_max_length = 256;       // Maximum key length (default)
```

## Error Handling

All functions return a `JMAP_RETURN` structure that contains either a value or an error:
```c
typedef struct JMAP_RETURN {
    union {
        void* value;
        JMAP_RETURN_ERROR error;
    };
    bool has_value;
    bool has_error;
    const JMAP* ret_source;
} JMAP_RETURN;
```

## MACROS

Use these macros for automatic error checking and value extraction:
```c
JMAP_CHECK_RET(ret);             // Print error, free error, return true if error -> if you need ret value later
JMAP_CHECK_RET_FREE(ret);        // Print error, free return value and error, return true if error -> if you don't need ret value later
JMAP_GET_VALUE(type, val);       // Extract value from pointer (doesn't free)
JMAP_DIRECT_INPUT(type, val);    // Create pointer for input value
JMAP_RET_GET_VALUE(type, ret);   // Get value from return (doesn't free)
JMAP_RET_GET_VALUE_FREE(type, ret); // Get value and free return
JMAP_RET_GET_POINTER(type, ret); // Get pointer from return
JMAP_FREE_RET(ret);              // Free both value and error
JMAP_FREE_RET_VALUE(ret);        // Free only value
JMAP_FREE_RET_ERROR(ret);        // Free only error
```