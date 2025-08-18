#ifndef JMAP_H
#define JMAP_H

#include <string.h>
#include <stdbool.h>


typedef enum {
    INDEX_OUT_OF_BOUND = 0,
    ARRAY_UNINITIALIZED,
    DATA_NULL,
    PRINT_ELEMENT_CALLBACK_UNINTIALIZED,
    EMPTY_ARRAY,
    ELEMENT_NOT_FOUND,
    INVALID_ARGUMENT,
    UNIMPLEMENTED_FUNCTION,
} JMAP_ERROR;

typedef struct JMAP_RETURN_ERROR {
    JMAP_ERROR error_code;
    char* error_msg;
} JMAP_RETURN_ERROR;

typedef struct JMAP_RETURN {
    union {
        void* value;
        JMAP_RETURN_ERROR error;
    };
    bool has_value;
    bool has_error;
} JMAP_RETURN;

typedef struct USER_FUNCTION_IMPLEMENTATION {
    void (*print_element_callback)(const void*);
} USER_FUNCTION_IMPLEMENTATION;

typedef struct JMAP {
    char ** keys;
    void * data;
    size_t _elem_size;
    size_t _length;
    size_t _elem_nb;
    size_t _key_max_length; // Maximum length of keys, used for memory allocation
    USER_FUNCTION_IMPLEMENTATION user_implementation;
} JMAP;

typedef struct JMAP_INTERFACE {
    void (*print_array_err)(const JMAP_RETURN ret);
    JMAP_RETURN (*init)(JMAP *self, size_t elem_size);
    JMAP_RETURN (*put)(JMAP *self, const char *key, const void *value);
    JMAP_RETURN (*get)(const JMAP *self, const char *key);
    void (*free)(JMAP *self);
} JMAP_INTERFACE;

extern JMAP_INTERFACE jmap;





#define GET_VALUE(type, val) (*(type*)val)

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
 * @brief Converts a value to a pointer of the specified type.
 * @param type The type to convert to.
 * @param value The value to convert.
 * @return A pointer of type `type*` pointing to the value.
 */
#define TO_POINTER(type, value) (&(type){value})

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
    if ((ret).has_error) { jmap.print_array_err(ret); return 1; }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists. Then returns.
 * @param ret The JMAP_RETURN structure to check.
 */
#define CHECK_RET_FREE(ret) \
    if ((ret).has_value) free((ret).value); if ((ret).has_error) { jmap.print_array_err(ret);  return 1; }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists.
 * @param ret The JMAP_RETURN structure to check.
 * @return true if the JMAP_RETURN has an error, false otherwise.
 */
#define CHECK_RET_CONTINUE(ret) \
    if ((ret).has_error) { jmap.print_array_err(ret); }

/**
 * @brief Checks if a JMAP_RETURN has an error and prints it if so, freeing the .value if it exists.
 * @param ret The JMAP_RETURN structure to check.
 * @return true if the JMAP_RETURN has an error, false otherwise.
 */
#define CHECK_RET_CONTINUE_FREE(ret) \
    if ((ret).has_value) free((ret).value); if ((ret).has_error) { jmap.print_array_err(ret); }

/**
 * @brief Frees the value in a JMAP_RETURN if it has a value.
 * @param ret The JMAP_RETURN structure to free.
 */
#define FREE_RET(ret) \
    if ((ret).has_value && (ret).value != NULL) { free((ret).value); (ret).value = NULL; }

#define MAX(a, b) a > b ? a : b

#endif