#ifndef JMAP_H
#define JMAP_H

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


typedef enum {
    JMAP_UNINITIALIZED = 0,
    DATA_NULL,
    PRINT_ELEMENT_CALLBACK_UNINTIALIZED,
    EMPTY_JMAP,
    ELEMENT_NOT_FOUND,
    INVALID_ARGUMENT,
    UNIMPLEMENTED_FUNCTION,
} JMAP_ERROR;

typedef struct JMAP_RETURN_ERROR {
    JMAP_ERROR error_code;
    char* error_msg;
} JMAP_RETURN_ERROR;



typedef struct USER_FUNCTION_IMPLEMENTATION {
    void (*print_element_callback)(const void*);
    void (*print_error_callback)(const JMAP_RETURN_ERROR);
    bool (*is_equal_callback)(const void*, const void*);
} USER_FUNCTION_IMPLEMENTATION;



typedef struct JMAP {
    char ** keys;
    void * data;
    size_t _elem_size;
    size_t _length;
    size_t _capacity;
    float _load_factor; // Load factor for resizing
    size_t _key_max_length; // Maximum length of keys, used for memory allocation
    USER_FUNCTION_IMPLEMENTATION user_implementation;
} JMAP;

typedef struct JMAP_RETURN {
    union {
        void* value;
        JMAP_RETURN_ERROR error;
    };
    bool has_value;
    bool has_error;
    const JMAP* ret_source; // Pointer to the JMAP structure that generated this return
} JMAP_RETURN;

typedef struct JMAP_INTERFACE {
    void (*print_array_err)(const JMAP_RETURN ret, const char *file, int line);
    JMAP_RETURN (*print)(const JMAP *self);
    /**
     * @brief Initializes the JMAP structure.
     * @param self Pointer to the JMAP structure to initialize.
     * @param elem_size Size of the elements to be stored in the JMAP.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*init)(JMAP *self, size_t elem_size);
    /**
     * @brief Inserts a key-value pair into the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to insert.
     * @param value Pointer to the value to insert.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*put)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Retrieves a value by its key from the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to retrieve.
     * @return JMAP_RETURN structure containing the value or error.
     */
    JMAP_RETURN (*get)(const JMAP *self, const char *key);
    /**
     * @brief Empties the JMAP, removing all key-value pairs.
     * @param self Pointer to the JMAP structure.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*clear)(JMAP *self);
    /**
     * @brief Resizes the JMAP to a new length.
     * @note The new length must be a power of 2 and greater than the current length.
     *       This is to ensure that the hash table can be resized correctly.
     * @param self Pointer to the JMAP structure.
     * @param new_length The new length for the JMAP.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*resize)(JMAP *self, size_t new_length);
    /**
     * @brief Clones the JMAP structure.
     * @param self Pointer to the JMAP structure to clone.
     * @return JMAP_RETURN structure containing the cloned JMAP or error.
     */
    JMAP_RETURN (*clone)(const JMAP *self);
    /**
     * @brief Checks if a key exists in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to check.
     * @return JMAP_RETURN structure indicating whether the key exists or an error. The value is a pointer to a boolean.
     *         The caller is responsible for freeing the returned pointer.
     */
    JMAP_RETURN (*contains_key)(const JMAP *self, const char *key);
    /**
     * @brief Returns an array of keys in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @return JMAP_RETURN structure containing an array of keys or an error.
     *         The keys are dynamically allocated and should be freed by the caller.
     */
    JMAP_RETURN (*get_keys)(const JMAP *self);
    /**
     * @brief Checks if a value exists in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param value Pointer to the value to check.
     * @return JMAP_RETURN structure indicating whether the value exists or an error. The value is a pointer to a boolean.
     *         The caller is responsible for freeing the returned pointer.
     */
    JMAP_RETURN (*contains_value)(const JMAP *self, const void *value);
    /**
     * @brief Returns an array of values in the JMAP.
     * @param self Pointer to the JMAP structure.
     * @return JMAP_RETURN structure containing an array of values or an error.
     *         The values are dynamically allocated and should be freed by the caller.
     */
    JMAP_RETURN (*get_values)(const JMAP *self);
    /**
     * @brief Iterates over each key-value pair in the JMAP and applies a callback function.
     * @param self Pointer to the JMAP structure.
     * @param callback Function to call for each key-value pair.
     * @param ctx Context pointer passed to the callback function.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*for_each)(const JMAP *self, void (*callback)(const char *key, void *value, const void *ctx), const void *ctx);
    /**
     * @brief Checks if the JMAP is empty.
     * @param self Pointer to the JMAP structure.
     * @return JMAP_RETURN structure indicating whether the JMAP is empty or an error.
     *         The value is a pointer to a boolean indicating emptiness.
     */
    JMAP_RETURN (*is_empty)(const JMAP *self);
    /**
     * @brief Puts a key-value pair into the JMAP if the key does not already exist.
     * @param self Pointer to the JMAP structure.
     * @param key The key to insert.
     * @param value Pointer to the value to insert.
     * @return JMAP_RETURN structure indicating success or error.
     */
    JMAP_RETURN (*put_if_absent)(JMAP *self, const char *key, const void *value);
    /**
     * @brief Removes a key-value pair from the JMAP.
     * @param self Pointer to the JMAP structure.
     * @param key The key to remove.
     * @return JMAP_RETURN structure indicating success or error. The value is NULL if the key was removed successfully.
     */
    JMAP_RETURN (*remove)(JMAP *self, const char *key);
    /**
     * @brief Removes a key-value pair from the JMAP if the value matches.
     * @param self Pointer to the JMAP structure.
     * @param key The key to check and remove.
     * @param value The value to match for removal.
     * @return JMAP_RETURN structure indicating success or error. The value is NULL if the key was removed successfully.
     */
    JMAP_RETURN (*remove_if_value_match)(JMAP *self, const char *key, const void *value);

    /**
     * @brief Frees the JMAP structure and its resources.
     * @param self Pointer to the JMAP structure to free.
     */
    void (*free)(JMAP *self);
} JMAP_INTERFACE;

extern JMAP_INTERFACE jmap;





#define GET_VALUE(type, val) (*(type*)val)

static inline void* direct_input_impl(size_t size, void *value) {
    void *p = malloc(size);
    if (p) memcpy(p, value, size);
    return p;
}


#define DIRECT_INPUT(type, val) ((type*) direct_input_impl(sizeof(type), &(type){val}))

/**
 * @brief Extracts the value from a JMAP_RETURN, and frees the data pointed by .value if not NULL.
 * @param type The type of the value to extract.
 * @param JMAP_RETURN The JMAP_RETURN structure to extract the value from.
 * @return The extracted value of type `type`.
 */
#define RET_GET_VALUE_FREE(type, JMAP_RETURN) \
    ({ type _tmp = *(type*)(JMAP_RETURN).value; free((JMAP_RETURN).value); _tmp; })

/**
 * @brief Extracts the pointer from a JMAP_RETURN.
 * @param type The type of the pointer to extract.
 * @param JMAP_RETURN The JMAP_RETURN structure to extract the pointer from.
 * @return The extracted pointer of type `type*`.
 */
#define RET_GET_POINTER_OWNED(type, JMAP_RETURN) \
    ({ type* _tmp = (type*)(JMAP_RETURN).value; (JMAP_RETURN).value = NULL; _tmp; })

/**
 * @brief Extracts the value from a JMAP_RETURN without freeing it. It is the caller's responsibility to free the .value if needed.
 * @param type The type of the value to extract.
 * @param JMAP_RETURN The JMAP_RETURN structure to extract the value from.
 * @return The extracted value of type `type`.
 */
#define RET_GET_VALUE(type, JMAP_RETURN) (*(type*)(JMAP_RETURN).value)

/**
 * @brief Extracts the pointer from a JMAP_RETURN without freeing it. It is the caller's responsibility to free the pointer if needed.
 * @param type The type of the pointer to extract.
 * @param JMAP_RETURN The JMAP_RETURN structure to extract the pointer from.
 * @return The extracted pointer of type `type*`.
 */
#define RET_GET_POINTER(type, JMAP_RETURN) ((type*)(JMAP_RETURN).value)

/**
 * @bried Extract the value from a JMAP_RETURN, and returns a default value if the JMAP_RETURN has no value.
 * @param type The type of the value to extract.
 * @param JMAP_RETURN The JMAP_RETURN structure to extract the value from.
 * @param default_value The default value to return if the JMAP_RETURN has no value.
 * @return The extracted value of type `type`, or the default value if the JMAP_RETURN has no value.
 */
#define RET_GET_VALUE_SAFE(type, JMAP_RETURN, default_value) \
    ((JMAP_RETURN).has_value ? *(type*)((JMAP_RETURN).value) : (default_value))

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so. Then returns.
 * @param ret The JMAP_RETURN structure to check.
 */
#define CHECK_RET(ret) \
    if ((ret).has_error) { jmap.print_array_err(ret, __FILE__, __LINE__); return 1; }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists. Then returns.
 * @param ret The JMAP_RETURN structure to check.
 */
#define CHECK_RET_FREE(ret) \
    if ((ret).has_value) free((ret).value); \
    if ((ret).has_error) { jmap.print_array_err(ret, __FILE__, __LINE__);  return 1; }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists.
 * @param ret The JMAP_RETURN structure to check.
 * @return true if the JMAP_RETURN has an error, false otherwise.
 */
#define CHECK_RET_CONTINUE(ret) \
    if ((ret).has_error) { jmap.print_array_err(ret, __FILE__, __LINE__); }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists.
 * @param ret The JMAP_RETURN structure to check.
 * @return true if the JMAP_RETURN has an error, false otherwise.
 */
#define CHECK_RET_CONTINUE_FREE(ret) \
    if ((ret).has_value) free((ret).value); \
    if ((ret).has_error) { jmap.print_array_err(ret, __FILE__, __LINE__); }

/**
 * @brief Frees the value in a JMAP_RETURN if it has a value.
 * @param ret The JMAP_RETURN structure to free.
 */
#define FREE_RET(ret) \
    if ((ret).has_value && (ret).value != NULL) { free((ret).value); (ret).value = NULL; }

#define MAX(a, b) a > b ? a : b

#endif