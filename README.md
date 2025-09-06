# JMAP - Generic Hash Map for C

**Note**: I'm a beginner programmer, so don't take this code too seriously. This is a learning project!

A hash map library for C with key-value storage, iteration, and various utility functions. The library is designed to be generic, allowing storage of any data type by specifying the element size during initialization, or by using preset.
It provides a variety of utility functions and supports user-defined callbacks for custom behavior. 
The functions are inspired by higher-level languages like JavaScript or Java.

## Install

Via Git download:
```bash
mkdir build && cd build
cmake ..
sudo make lib
sudo ldconfig
```

Via dpkg:

```bash
sudo dpkg -i libjmap-dev_1.0.0-beta-1_amd64.deb
sudo apt-get install -f
sudo ldconfig
```
This should install libjmap.so and libjmap.a in /urs/local/lib/ and jmap.h in usr/local/include/

To use in your projet just include with *#include <jmap.h>* and link with *-ljmap* when compiling.
You can find in folder `Examples` some simple c files using jmap. To see result (for jmap_string.c for example): 
```bash
cd Examples
make
./jmap_string
```

## Quick Start

```c
#include <jmap.h>
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
```
### Output

```bash
Age: 25
JMAP [size: 2, capacity: 16, load factor:0.75] =>
{7, age -> 25 }
{8, score -> 100 }
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
jmap.to_sort(&map, res_keys, res_values);            // Returns via `res_keys` and `res_values` the keys and values sorted using the compare_pairs function
```

## Required Callbacks

Set these before using related functions:
```c
JMAP_USER_CALLBACK_IMPLEMENTATION imp;
imp.print_element_callback = print_element_array_callback;
imp.element_to_string = element_to_string_array_callback;
imp.is_equal = is_equal_array_callback;
```

## Override callbacks

There is some functions that can be overriden. Maybe more will be added later:
```c
JMAP_USER_OVERRIDE_IMPLEMENTATION imp;
imp.print_error_override = error_func;        // For error printing
imp.print_array_override = print_array_func;  // For print() override
imp.copy_elem_override = copy_elem_func;      // For copy override. MANDATORY when storing pointers (Example : strdup for char*)
imp.compare_pairs_override = compare_pairs_func; // By default, the keys of pairs are compared
```

## Configuration

The JMAP structure includes several configuration options:
```c
map._load_factor = 0.75f;        // Resize when 75% full (default)
map._key_max_length = 50;       // Maximum key length (default)
```

## Good practices
- you **should** implement every function of `JARRAY_USER_CALLBACKS_IMPLEMENTATION`.
- always check return value with macros below to be noticed if the last jmap function call produced an error.
- if you know rougly how many element there should be in your jmap, you should use `reserve` function to allocate memory beforehand (to reduce realloc calls).
- if you need to store pointers, you **must** implement the `copy_elem_override` function and set it in the user implementation structure of your array. Please look at file `jmap_string.c` in folder `Examples` where I implemented an array of string (char*) as an example. 

## Macros

Use these macros for automatic error checking and value extraction:
```c
JARRAY_CHECK_RET;                   // Print error, free error, return true if error
JARRAY_CHECK_RET_RETURN;            // Print error, free error, return EXIT_FAILURE if error
JARRAY_GET_VALUE(type, val);        // Extract value from pointer (doesn't free)
JARRAY_DIRECT_INPUT(type, val);     // Create pointer for input value
JARRAY_FREE_RET;                    // Free error
```