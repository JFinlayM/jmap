#include "jmap.h"
#include <stdio.h>
#include <stdarg.h>
#include "third_party/murmur3-master/murmur3.h"

#define NEXT_INDEX(index) ((index + 1) & (self->_capacity - 1))

static const char *enum_to_string[] = {
    [JMAP_NO_ERROR]                         = "JMAP no error",
    [JMAP_UNINITIALIZED]                   = "JMAP uninitialized",
    [JMAP_DATA_NULL]                             = "Data is null",
    [JMAP_PRINT_ELEMENT_CALLBACK_UNINTIALIZED]   = "Print callback not set",
    [JMAP_EMPTY]                           = "Empty jmap",
    [JMAP_INVALID_ARGUMENT]                      = "Invalid argument",
    [JMAP_ELEMENT_NOT_FOUND]                     = "Element not found",
    [JMAP_UNIMPLEMENTED_FUNCTION]                = "Function not implemented",
};


static inline size_t max_size_t(size_t a, size_t b) {return (a > b ? a : b);}

static inline void* memcpy_elem(JMAP *self, void *__restrict__ __dest, const void *__restrict__ __elem, size_t __count){
    void *ret;
    if (!self->user_overrides.copy_elem_override) ret = memcpy(__dest, __elem, self->_elem_size * __count);
    else {
        for (size_t i = 0; i < __count; i++){
            ret = memcpy(__dest + i * self->_elem_size, self->user_overrides.copy_elem_override(__elem + i * self->_elem_size), self->_elem_size);
        }
    }
    return ret;
}


static void create_return_error(const JMAP* ret_source, JMAP_ERROR error_code, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    last_error_trace.has_error = true;
    last_error_trace.error_code = error_code;
    vsnprintf(last_error_trace.error_msg, MAX_ERR_MSG_LENGTH, fmt, args);
    last_error_trace.ret_source = ret_source;
    va_end(args);
}

static void print_array_err(const char *file, int line) {
    if (last_error_trace.ret_source->user_overrides.print_error_override) {
        last_error_trace.ret_source->user_overrides.print_error_override(last_error_trace);
        return;
    }
    if (!last_error_trace.has_error) return;
    if (last_error_trace.error_code < 0 || last_error_trace.error_code >= sizeof(enum_to_string) / sizeof(enum_to_string[0]) || enum_to_string[last_error_trace.error_code] == NULL) {
        fprintf(stderr, "%s:%d [\033[31mUnknown error: %d\033[0m] : ", file, line, last_error_trace.error_code);
    } else {
        fprintf(stderr, "%s:%d [\033[31mError: %s\033[0m] : ", file, line, enum_to_string[last_error_trace.error_code]);
    }
    fprintf(stderr, "%s\n", last_error_trace.error_msg);
}

static void reset_error_trace(){
    last_error_trace.has_error = false;
    last_error_trace.ret_source = NULL;
    last_error_trace.error_code = JMAP_NO_ERROR;
    snprintf(last_error_trace.error_msg, MAX_ERR_MSG_LENGTH, "no error");
}


static void jmap_free(JMAP *self) {
    if (self->data != NULL) {
        free(self->data);
        self->data = NULL;
    }
    if (self->keys != NULL) {
        for (size_t i = 0; i < self->_capacity; i++) {
            free(self->keys[i]);
        }
        free(self->keys);
        self->keys = NULL;
    }
    self->_length = 0;
    self->_capacity = 0;
    self->_elem_size = 0;
    self->_key_max_length = 0;
    reset_error_trace();
}

static void jmap_print(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    if (self->user_callbacks.print_element_callback == NULL)
        return create_return_error(self, JMAP_PRINT_ELEMENT_CALLBACK_UNINTIALIZED, "Print element callback not set");

    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no print\n");

    printf("JMAP [size: %zu, capacity: %zu, load factor:%.2f] =>\n", self->_length, self->_capacity, self->_load_factor);
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            printf("{%zu, %s -> ", i, self->keys[i]);
            self->user_callbacks.print_element_callback((char*)self->data + i * self->_elem_size);
            printf("}\n");
        }
    }
    reset_error_trace();
}

static void jmap_init(JMAP *array, size_t _elem_size) {
    array->_capacity = 16;
    array->_load_factor = 0.75f;
    array->_elem_size = _elem_size;
    array->_length = 0;
    array->_key_max_length = 50;
    array->data = malloc(array->_capacity * array->_elem_size);
    if (array->data == NULL) {
        return create_return_error(array, JMAP_UNINITIALIZED, "Memory allocation for data failed");
    }
    memset(array->data, 0, array->_capacity * array->_elem_size);
    array->keys = malloc(array->_capacity * sizeof(char*));
    if (array->keys == NULL) {
        free(array->data);
        return create_return_error(array, JMAP_UNINITIALIZED, "Memory allocation for keys failed");
    }
    for (size_t i = 0; i < array->_capacity; i++) {
        array->keys[i] = NULL;
    }
    

    array->user_callbacks.print_element_callback = NULL;
    array->user_overrides.print_error_override = NULL;
    array->user_callbacks.is_equal = NULL;
    
    reset_error_trace();
}

static size_t jmap_key_to_index(const JMAP *self, const char *key) {
    if (key == NULL) create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL");
    if (strlen(key) == 0) create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be empty");
    size_t output;
    uint32_t hash;
    MurmurHash3_x86_32(key, strlen(key), 42, &hash);
    output = hash & (self->_capacity - 1);
    reset_error_trace();
    return output;
}

static void jmap_resize(JMAP *self, size_t new_length) {
    if ((new_length & (new_length - 1)) != 0) {
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "new_length must be power of two");
    }

    if (new_length <= self->_capacity) {
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "new_length must be greater than current capacity");
    }

    char **old_keys   = self->keys;
    void  *old_data   = self->data;
    size_t old_length = self->_capacity;

    char **new_keys = calloc(new_length, sizeof(char*));
    if (!new_keys) return create_return_error(self, JMAP_UNINITIALIZED, "alloc keys failed");
    void *new_data = calloc(new_length, self->_elem_size);
    if (!new_data) {
        free(new_keys);
        return create_return_error(self, JMAP_UNINITIALIZED, "alloc data failed");
    }

    self->keys    = new_keys;
    self->data    = new_data;
    self->_capacity = new_length;

    self->_length   = 0;

    for (size_t i = 0; i < old_length; i++) {
        char *k = old_keys[i];
        if (!k) continue;

        size_t idx = jmap_key_to_index(self, k);
        while (self->keys[idx] != NULL) {
            idx = NEXT_INDEX(idx);
        }

        self->keys[idx] = k;
        memcpy((char*)self->data + idx * self->_elem_size,
               (char*)old_data    + i   * self->_elem_size,
               self->_elem_size);
        self->_length++;
    }

    free(old_keys);
    free(old_data);
    reset_error_trace();
}


static void jmap_put(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    if (self->_length + 1 > (self->_capacity * self->_load_factor)) {
        jmap_resize(self, self->_capacity * 2);
        if (last_error_trace.has_error) return;
    }

    size_t idx = jmap_key_to_index(self, key);
    while (self->keys[idx] != NULL && strcmp(self->keys[idx], key) != 0) {
        idx = NEXT_INDEX(idx);
    }

    if (self->keys[idx] == NULL) {
        self->keys[idx] = strdup(key);
        if (!self->keys[idx])
            return create_return_error(self, JMAP_UNINITIALIZED, "strdup failed for key");
        self->_length++;
    }

    memcpy((char*)self->data + idx * self->_elem_size, value, self->_elem_size);

    reset_error_trace();
}

static void* jmap_get(const JMAP *self, const char *key) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return NULL;
    }
    if (!key || key[0] == '\0') {
        create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");
        return NULL;
    }

    size_t idx = jmap_key_to_index(self, key);
    size_t probes = 0;

    while (probes < self->_capacity) {
        char *k = self->keys[idx];
        if (!k) {
            create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found" , key);
            return NULL;
        }
        if (strcmp(k, key) == 0) {
            reset_error_trace();
            return (char*)self->data + idx * self->_elem_size;
        }
        idx = NEXT_INDEX(idx);
        probes++;
    }

    create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found after full probe", key);
    return NULL;
}

static void jmap_clear(JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    for (size_t i = 0; i < self->_capacity; i++) {
        free(self->keys[i]);
        self->keys[i] = NULL;
    }
    memset(self->data, 0, self->_capacity * self->_elem_size);
    self->_length = 0;

    reset_error_trace();
}

static JMAP jmap_clone(const JMAP *self) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return *self;
    }

    JMAP clone;
    clone._elem_size = self->_elem_size;
    clone._capacity = self->_capacity;
    clone._length = self->_length;
    clone._key_max_length = self->_key_max_length;
    clone._load_factor = self->_load_factor;
    clone.user_callbacks = self->user_callbacks;
    clone.user_overrides = self->user_overrides;

    clone.data = malloc(clone._capacity * clone._elem_size);
    if (!clone.data) {
        free(clone.data);
        create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for clone data failed");
        return *self;
    }
    memcpy(clone.data, self->data, clone._capacity * clone._elem_size);

    clone.keys = malloc(clone._capacity * sizeof(char*));
    if (!clone.keys) {
        free(clone.data);
        create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for clone keys failed");
        return *self;
    }
    
    for (size_t i = 0; i < clone._capacity; i++) {
        if (self->keys[i]) {
            clone.keys[i] = strdup(self->keys[i]);
            if (!clone.keys[i]) {
                for (size_t j = 0; j < i; j++) {
                    free(clone.keys[j]);
                }
                free(clone.keys);
                free(clone.data);
                create_return_error(self, JMAP_UNINITIALIZED, "strdup failed for key in clone");
                return *self;
            }
        } else {
            clone.keys[i] = NULL;
        }
    }

    reset_error_trace();
    return clone;
}

static bool jmap_contains_key(const JMAP *self, const char *key) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return false;
    }
    if (!key || key[0] == '\0') {
        create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");
        return false;
    }

    size_t idx = jmap_key_to_index(self, key);
    size_t probes = 0;

    reset_error_trace();

    while (probes < self->_capacity) {
        char *k = self->keys[idx];
        if (!k) {
            return false;
        }
        if (strcmp(k, key) == 0) {
            return true;
        }
        idx = (idx + 1) & (self->_capacity - 1);
        probes++;
    }

    return false;
}

static bool jmap_contains_value(const JMAP *self, const void *value) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return false;
    }
    if (!value) {
        create_return_error(self, JMAP_INVALID_ARGUMENT, "Value cannot be NULL");
        return false;
    }

    reset_error_trace();

    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL && memcmp((char*)self->data + i * self->_elem_size, value, self->_elem_size) == 0) {
            return true;
        }
    }

    return false;
}

static char** jmap_get_keys(const JMAP *self) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return NULL;
    }

    if (self->_length == 0) {
        create_return_error(self, JMAP_EMPTY, "JMAP is empty => no keys");
        return NULL;
    }

    char **keys_array = malloc(self->_length * sizeof(char*));
    if (!keys_array) {
        create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for keys array failed");
        return NULL;
    }

    size_t count = 0;
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            keys_array[count++] = strdup(self->keys[i]);
            if (!keys_array[count - 1]) {
                for (size_t j = 0; j < count - 1; j++) {
                    free(keys_array[j]);
                }
                free(keys_array);
                create_return_error(self, JMAP_UNINITIALIZED, "strdup failed for key");
                return NULL;
            }
        }
    }

    reset_error_trace();
    return keys_array;
}

static void* jmap_get_values(const JMAP *self) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return NULL;
    }
    if (self->_length == 0) {
        create_return_error(self, JMAP_EMPTY, "JMAP is empty => no values");
        return NULL;
    }
    void *values_array = malloc(self->_length * self->_elem_size);
    if (!values_array) {
        create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for values array failed");
        return NULL;
    }

    size_t count = 0;
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            memcpy((char*)values_array + count * self->_elem_size,
                   (char*)self->data + i * self->_elem_size,
                   self->_elem_size);
            count++;
        }
    }

    reset_error_trace();
    return values_array;
}

static void jmap_for_each(const JMAP *self, void (*callback)(const char *key, void *value, const void *ctx), const void *ctx) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!callback)
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Callback function cannot be NULL");

    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            callback(self->keys[i], (char*)self->data + i * self->_elem_size, ctx);
        }
    }

    reset_error_trace();
}

static bool jmap_is_empty(const JMAP *self) {
    if (!self->data || !self->keys) {
        create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
        return false;
    }

    reset_error_trace();
    return (self->_length == 0);
}

static void jmap_put_if_absent(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    bool contains = jmap_contains_key(self, key);
    if (last_error_trace.has_error) return;

    if (contains) {
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" already exists", key);
    }

    reset_error_trace();
    jmap_put(self, key, value);
}

static void jmap_remove(JMAP *self, const char *key){
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");
    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no keys to remove");

    bool contains = jmap_contains_key(self, key);
    if (last_error_trace.has_error) return;

    if (contains) {
        jmap_get(self, key);
        if (last_error_trace.has_error) return;
        memset((char*)self->data + jmap_key_to_index(self, key) * self->_elem_size, 0, self->_elem_size); // Efface la valeur
        size_t index = jmap_key_to_index(self, key);
        size_t start_index = index;
        while (strcmp(self->keys[index], key) != 0) {
            index = NEXT_INDEX(index);
            if (index == start_index) {
                return create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found", key);
            }
        }
        free(self->keys[index]);
        self->keys[index] = NULL;
        self->_length--;
        // Réduire la taille si nécessaire
        if (self->_length < self->_capacity * self->_load_factor / 4 && self->_capacity > 16) {
            jmap_resize(self, self->_capacity / 2);
            if (last_error_trace.has_error) return;
        }
        reset_error_trace();
        return; 
    }

    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static void jmap_remove_if_value_match(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    bool exists = jmap_contains_key(self, key);
    if (last_error_trace.has_error) return;

    if (exists) {
        void *get = jmap_get(self, key);
        if (last_error_trace.has_error) return;
        if ((bool)self->user_callbacks.is_equal ? !self->user_callbacks.is_equal(get, value) : memcmp(get, value, self->_elem_size) != 0) {
            return create_return_error(self, JMAP_INVALID_ARGUMENT, "Values does not match for key \"%s\"", key);
        }
        memset((char*)self->data + jmap_key_to_index(self, key) * self->_elem_size, 0, self->_elem_size);
        size_t index = jmap_key_to_index(self, key);
        size_t start_index = index;
        while (strcmp(self->keys[index], key) != 0) {
            index = NEXT_INDEX(index);
            if (index == start_index) {
                return create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found", key);
            }
        }
        free(self->keys[index]);
        self->keys[index] = NULL;
        self->_length--;
 
        if (self->_length < self->_capacity * self->_load_factor / 4 && self->_capacity > 16) {
            jmap_resize(self, self->_capacity / 2);
            if (last_error_trace.has_error) return;
        }
        reset_error_trace();
        return;
    }
    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static void jmap_remove_if_value_not_match(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    bool exists = jmap_contains_key(self, key);
    if (last_error_trace.has_error) return;

    if (exists) {
        void *get = jmap_get(self, key);
        if (last_error_trace.has_error) return;
        if ((bool)self->user_callbacks.is_equal ? self->user_callbacks.is_equal(get, value) : memcmp(get, value, self->_elem_size) == 0) {
            return create_return_error(self, JMAP_INVALID_ARGUMENT, "Values match for key \"%s\"", key);
        }
        memset((char*)self->data + jmap_key_to_index(self, key) * self->_elem_size, 0, self->_elem_size);
        size_t index = jmap_key_to_index(self, key);
        size_t start_index = index;
        while (strcmp(self->keys[index], key) != 0) {
            index = NEXT_INDEX(index);
            if (index == start_index) {
                return create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found", key);
            }
        }
        free(self->keys[index]);
        self->keys[index] = NULL;
        self->_length--;

        if (self->_length < self->_capacity * self->_load_factor / 4 && self->_capacity > 16) {
            jmap_resize(self, self->_capacity / 2);
            if (last_error_trace.has_error) return;
        }
        reset_error_trace();
        return;
    }
    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static void jmap_remove_if(JMAP *self, bool (*predicate)(const char *key, const void *value, const void *ctx), const void *ctx) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!predicate)
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Predicate function cannot be NULL");

    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL && predicate(self->keys[i], (char*)self->data + i * self->_elem_size, ctx)) {
            free(self->keys[i]);
            self->keys[i] = NULL;
            memset((char*)self->data + i * self->_elem_size, 0, self->_elem_size);
            self->_length--;
        }
    }

    if (self->_length < self->_capacity * self->_load_factor / 4 && self->_capacity > 16) {
        jmap_resize(self, self->_capacity / 2);
        if (last_error_trace.has_error) return;
    }

    reset_error_trace();
}



JMAP_RETURN last_error_trace;
JMAP_INTERFACE jmap = {
    .init = jmap_init,
    .print_array_err = print_array_err,
    .print = jmap_print,
    .put = jmap_put,
    .get = jmap_get,
    .resize = jmap_resize,
    .clear = jmap_clear,
    .clone = jmap_clone,
    .free = jmap_free,
    .contains_key = jmap_contains_key,
    .contains_value = jmap_contains_value,
    .get_keys = jmap_get_keys,
    .get_values = jmap_get_values,
    .for_each = jmap_for_each,
    .is_empty = jmap_is_empty,
    .put_if_absent = jmap_put_if_absent,
    .remove = jmap_remove,
    .remove_if_value_match = jmap_remove_if_value_match,
    .remove_if_value_not_match = jmap_remove_if_value_not_match,
    .remove_if = jmap_remove_if,
};