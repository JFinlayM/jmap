#ifndef JMAP_H
#define JMAP_H

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_ERR_MSG_LENGTH 100


typedef struct JMAP JMAP;

typedef enum {
    JMAP_NO_ERROR = 0,
    JMAP_UNINITIALIZED,
    JMAP_DATA_NULL,
    JMAP_PRINT_ELEMENT_CALLBACK_UNINTIALIZED,
    JMAP_EMPTY,
    JMAP_ELEMENT_NOT_FOUND,
    JMAP_INVALID_ARGUMENT,
    JMAP_UNIMPLEMENTED_FUNCTION,
} JMAP_ERROR;

typedef enum {
    JMAP_INT_PRESET = 0,
    JMAP_STRING_PRESET,
} JMAP_TYPE_PRESET;

typedef struct JMAP_RETURN {
    JMAP_ERROR error_code;
    char error_msg[MAX_ERR_MSG_LENGTH];
    bool has_error;
    const JMAP* ret_source;
} JMAP_RETURN;

/**
 * @brief User-defined function implementations for JMAP.
 * This structure contains pointers to user-defined functions for printing, comparing, and checking equality of elements.
 * These functions can be set by the user to customize the behavior of the JMAP. These functions are used by the JMAP_INTERFACE to perform operations on the elements.
 */
typedef struct JMAP_USER_CALLBACK_IMPLEMENTATION {
    // Function to print an element. This function is mandatory if you want to use the jmap.print function.
    void (*print_element_callback)(const void* value);
    // Function to convert an element to a string. This function is NOT mandatory but can be useful for functions like join.
    char *(*element_to_string)(const void* value);
    // Function to check if two elements are equal. This function is mandatory if you want to use the jmap.contains, jmap.find_first, jmap.indexes_of functions.
    bool (*is_equal)(const void* value_a, const void* value_b);
    // This function is MANDATORY if storing pointers (Example : strdup for char*).
    void *(*copy_elem_override)(const void* value);    
} JMAP_USER_CALLBACK_IMPLEMENTATION;

typedef struct JMAP_USER_OVERRIDE_IMPLEMENTATION {
    // Override function to print errors. This function is NOT mandatory.
    void (*print_error_override)(const JMAP_RETURN ret);
    // Override function to print the whole array. This function is NOT mandatory.
    void (*print_array_override)(const JMAP* array);
    // Function to compare two elements. This function is mandatory if you want to use the jmap.sort function.
    int (*compare_pairs_override)(const char *key_a, const void *value_a, const char *key_b, const void *value_b);
}JMAP_USER_OVERRIDE_IMPLEMENTATION;

typedef enum JMAP_DATA_TYPE {
    JMAP_TYPE_VALUE = 0,
    JMAP_TYPE_POINTER
}JMAP_DATA_TYPE;

typedef struct JMAP {
    char ** keys;
    void * data;
    size_t _elem_size;
    size_t _length;
    size_t _capacity;
    float _load_factor; // Load factor for resizing
    size_t _key_max_length; // Maximum length of keys, used for memory allocation
    JMAP_DATA_TYPE _data_type;
    JMAP_USER_CALLBACK_IMPLEMENTATION user_callbacks;
    JMAP_USER_OVERRIDE_IMPLEMENTATION user_overrides;
} JMAP;



typedef struct JMAP_INTERFACE {
    void (*print_array_err)(const char *file, int line);
    void (*print)(const JMAP *self);
    /**
     * @brief Initializes the JMAP structure.
     * @param self Pointer to the JMAP structure to initialize.
     * @param elem_size Size of the elements to be stored in the JMAP.
     * @param imp structure that contains the pointer to the user function implementations.
     */
    void (*init)(JMAP *self, size_t elem_size, JMAP_DATA_TYPE data_type, JMAP_USER_CALLBACK_IMPLEMENTATION imp);
    JMAP (*init_preset)(JMAP_TYPE_PRESET preset);
    /**
     * @brief Inserts a key-value pair into the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to insert.
     * @param value Pointer to the value to insert.
     */
    void (*put)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Retrieves a value by its key from the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to retrieve.
     * @return Pointer to element. Do NOT free.
     */
    void* (*get)(const JMAP *self, const char *key);
    /**
     * @brief Empties the JMAP, removing all key-value pairs.
     * @param self Pointer to the JMAP structure.
     */
    void (*clear)(JMAP *self);
    /**
     * @brief Resizes the JMAP to a new length.
     * @note The new length must be a power of 2 and greater than the current length.
     *       This is to ensure that the hash table can be resized correctly.
     * @param self Pointer to the JMAP structure.
     * @param new_length The new length for the JMAP.
     */
    void (*resize)(JMAP *self, size_t new_length);
    /**
     * @brief Clones the JMAP structure.
     * @param self Pointer to the JMAP structure to clone.
     * @return Cloned JMAP
     */
    JMAP (*clone)(const JMAP *self);
    /**
     * @brief Checks if a key exists in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to check.
     * @return boolean: true if key exists, false otherwise.
     */
    bool (*contains_key)(const JMAP *self, const char *key);
    /**
     * @brief Returns an array of keys in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @return Pointer to char*. The keys that are dynamically allocated and should be freed by the caller.
     */
    char** (*get_keys)(const JMAP *self);
    /**
     * @brief Checks if a value exists in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param value Pointer to the value to check.
     * @return boolean: true if value exists, false otherwise.
     */
    bool (*contains_value)(const JMAP *self, const void *value);
    /**
     * @brief Returns an array of values in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @return Pointer to the first value of a heap allocated array, containing all values.
     */
    void* (*get_values)(JMAP *self);
    /**
     * @brief Iterates over each key-value pair in the JMAP and applies a callback function.
     * @param self Pointer to the JMAP structure.
     * @param callback Function to call for each key-value pair.
     * @param ctx Context pointer passed to the callback function.
     */
    void (*for_each)(const JMAP *self, void (*callback)(const char *key, void *value, const void *ctx), const void *ctx);
    /**
     * @brief Checks if the JMAP is empty.
     * @param self Pointer to the JMAP structure.
     * @return boolea: true if JMAP empty, false otherwise.
     */
    bool (*is_empty)(const JMAP *self);
    /**
     * @brief Puts a key-value pair into the JMAP if the key does not already exist.
     * @param self Pointer to the JMAP structure.
     * @param key The key to insert.
     * @param value Pointer to the value to insert.
     */
    void (*put_if_absent)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Removes a key-value pair from the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to remove.
     */
    void (*remove)(JMAP *self, const char *key);
    /**
     * @brief Removes a key-value pair from the JMAP if the value matches.
     * @param self Pointer to the JMAP structure.
     * @param key The key to check and remove.
     * @param value The value to match for removal.
     */
    void (*remove_if_value_match)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Removes a key-value pair from the JMAP if the value does not match.
     * @param self Pointer to the JMAP structure.
     * @param key The key to check and remove.
     * @param value The value to not match for removal.
     */
    void (*remove_if_value_not_match)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Removes all key-value pairs that match a predicate function.
     * @param self Pointer to the JMAP structure.
     * @param predicate Function to determine if a key-value pair should be removed.
     * @param ctx Context pointer passed to the predicate function.
     */
    void (*remove_if)(JMAP *self, bool (*predicate)(const char *key, const void *value, const void *ctx), const void *ctx);
    /**
     * @brief Frees the JMAP structure and its resources.
     * @param self Pointer to the JMAP structure to free.
     */
    void (*free)(JMAP *self);
    /**
     * @brief Sorts the JMAP based on a comparison function.
     * @param self Pointer to the JMAP structure.
     * @param keys Pointer to array of char*. Will point to keys.
     * @param values Pointer to array of element. Will point to valius.
     * @param compare Function to compare two key-value pairs.
     * @param ctx Context pointer passed to the comparison function.
     */
    void (*to_sort)(JMAP *self, char ***keys, void **values);
} JMAP_INTERFACE;

extern JMAP_INTERFACE jmap;
extern JMAP_RETURN jmap_last_error_trace;


/* ----- MACROS ----- */

/**
 * @brief Extracts the value from a pointer returned by JMAP.
 *
 * @param type The type of the value stored in the pointer.
 * @param val Pointer to the value.
 * @return The value pointed by val, cast to type `type`.
 *
 * @note Does NOT free the pointer.
 */
#define JMAP_GET_VALUE(type, val) (*(type*)val)

#define JMAP_GET_POINTER(type, val) ((type*)val)

/**
 * @brief Allocates memory and copies a value into it.
 *
 * @param size Size of the value in bytes.
 * @param value Pointer to the value to copy.
 * @return Pointer to the newly allocated copy, or NULL if allocation fails.
 *
 * @note Caller is responsible for freeing the returned pointer.
 */
static inline void* jmap_direct_input_impl(size_t size, void *value) {
    void *p = malloc(size);
    if (p) memcpy(p, value, size);
    return p;
}


/**
 * @brief Creates a pointer to a temporary value of a given type.
 *
 * @param type Type of the value.
 * @param val Value to copy into allocated memory.
 * @return Pointer to the allocated copy of the value.
 *
 * @note Caller must free the returned pointer if needed.
 */
#define JMAP_DIRECT_INPUT(type, val) ((type*) jmap_direct_input_impl(sizeof(type), &(type){val}))


/**
 * @brief Checks if global error trace contains error.
 *
 * @return true if error, false otherwise.
 */
#define JMAP_CHECK_RET \
    ({ \
        bool ret_val = false; \
        if (jmap_last_error_trace.has_error) { \
            jmap.print_array_err(__FILE__, __LINE__); \
            ret_val = true; \
        } \
        ret_val; \
    })

#define JMAP_CHECK_RET_RETURN \
    if (jmap_last_error_trace.has_error) { \
        jmap.print_array_err(__FILE__, __LINE__); \
        return EXIT_FAILURE; \
    }

#endif