#include "jmap.h"
#include <stdio.h>
#include <stdarg.h>
#include "third_party/murmur3-master/murmur3.h"

#define NEXT_INDEX(index) ((index + 1) & (self->_capacity - 1))

static const char *enum_to_string[] = {
    [JMAP_UNINITIALIZED]                   = "JMAP uninitialized",
    [JMAP_DATA_NULL]                             = "Data is null",
    [JMAP_PRINT_ELEMENT_CALLBACK_UNINTIALIZED]   = "Print callback not set",
    [JMAP_EMPTY]                           = "Empty jmap",
    [JMAP_INVALID_ARGUMENT]                      = "Invalid argument",
    [JMAP_ELEMENT_NOT_FOUND]                     = "Element not found",
    [JMAP_UNIMPLEMENTED_FUNCTION]                = "Function not implemented",
};

static JMAP_RETURN create_return_error(const JMAP* ret_source, JMAP_ERROR error_code, const char* fmt, ...) {
    JMAP_RETURN ret;
    ret.has_value = false;
    ret.has_error = true;
    ret.ret_source = ret_source;
    va_list args;
    va_start(args, fmt);

    char *buf = malloc(256);
    if (!buf) {
        ret.error.error_msg = "Memory allocation failed";
        ret.error.error_code = JMAP_UNINITIALIZED;
        va_end(args);
        return ret;
    }

    vsnprintf(buf, 256, fmt, args);
    va_end(args);

    ret.error.error_code = error_code;
    ret.error.error_msg = buf;
    return ret;
}

static void print_array_err(const JMAP_RETURN ret, const char *file, int line) {
    if (ret.ret_source->user_implementation.print_error_callback) {
        ret.ret_source->user_implementation.print_error_callback(ret.error);
        return;
    }
    if (ret.has_value) return;
    if (!ret.has_error) return;
    if (ret.error.error_code < 0 || ret.error.error_code >= sizeof(enum_to_string) / sizeof(enum_to_string[0]) || enum_to_string[ret.error.error_code] == NULL) {
        fprintf(stderr, "%s:%d [\033[31mUnknown error: %d\033[0m] : ", file, line, ret.error.error_code);
    } else {
        fprintf(stderr, "%s:%d [\033[31mError: %s\033[0m] : ", file, line, enum_to_string[ret.error.error_code]);
    }
    if (ret.error.error_msg) {
        fprintf(stderr, "%s\n", ret.error.error_msg);
    }
}

static JMAP_RETURN jmap_print(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    if (self->user_implementation.print_element_callback == NULL)
        return create_return_error(self, JMAP_PRINT_ELEMENT_CALLBACK_UNINTIALIZED, "Print element callback not set");

    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no print\n");

    printf("JMAP [size: %zu, capacity: %zu, load factor:%.2f] =>\n", self->_length, self->_capacity, self->_load_factor);
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            printf("{%zu, %s -> ", i, self->keys[i]);
            self->user_implementation.print_element_callback((char*)self->data + i * self->_elem_size);
            printf("}\n");
        }
    }

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    return ok;
}

static JMAP_RETURN jmap_init(JMAP *array, size_t _elem_size) {
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
    

    array->user_implementation.print_element_callback = NULL;
    array->user_implementation.print_error_callback = NULL;
    array->user_implementation.is_equal_callback = NULL;

    JMAP_RETURN ret;
    ret.has_value = false;
    ret.has_error = false;
    ret.value = NULL;
    return ret;
}

static size_t jmap_key_to_index(const JMAP *self, const char *key) {
    if (key == NULL) create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL");
    if (strlen(key) == 0) create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be empty");
    size_t output;
    uint32_t hash;
    MurmurHash3_x86_32(key, strlen(key), 42, &hash);
    output = hash & (self->_capacity - 1);
    return output;
}

static JMAP_RETURN jmap_resize(JMAP *self, size_t new_length) {
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

    size_t old_count = self->_length;
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

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    (void)old_count;
    return ok;
}


static JMAP_RETURN jmap_put(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    if (self->_length + 1 > (self->_capacity * self->_load_factor)) {
        JMAP_RETURN rr = jmap_resize(self, self->_capacity * 2);
        if (rr.has_error) return rr;
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

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    return ok;
}

static JMAP_RETURN jmap_get(const JMAP *self, const char *key) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    size_t idx = jmap_key_to_index(self, key);
    size_t probes = 0;

    while (probes < self->_capacity) {
        char *k = self->keys[idx];
        if (!k) {
            return create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found" , key);
        }
        if (strcmp(k, key) == 0) {
            JMAP_RETURN ret = {0};
            ret.has_value = true;
            ret.has_error = false;
            ret.value = (char*)self->data + idx * self->_elem_size;
            return ret;
        }
        idx = NEXT_INDEX(idx);
        probes++;
    }

    return create_return_error(self, JMAP_ELEMENT_NOT_FOUND, "Key \"%s\" not found after full probe", key);
}

static JMAP_RETURN jmap_clear(JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    for (size_t i = 0; i < self->_capacity; i++) {
        free(self->keys[i]);
        self->keys[i] = NULL;
    }
    memset(self->data, 0, self->_capacity * self->_elem_size);
    self->_length = 0;

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    return ok;
}

static JMAP_RETURN jmap_clone(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    JMAP *clone = (JMAP*)malloc(sizeof(JMAP));
    if (!clone) return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for clone failed");

    clone->_elem_size = self->_elem_size;
    clone->_capacity = self->_capacity;
    clone->_length = self->_length;
    clone->_key_max_length = self->_key_max_length;
    clone->_load_factor = self->_load_factor;
    clone->user_implementation = self->user_implementation;

    clone->data = malloc(clone->_capacity * clone->_elem_size);
    if (!clone->data) {
        free(clone);
        return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for clone data failed");
    }
    memcpy(clone->data, self->data, clone->_capacity * clone->_elem_size);

    clone->keys = malloc(clone->_capacity * sizeof(char*));
    if (!clone->keys) {
        free(clone->data);
        free(clone);
        return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for clone keys failed");
    }
    
    for (size_t i = 0; i < clone->_capacity; i++) {
        if (self->keys[i]) {
            clone->keys[i] = strdup(self->keys[i]);
            if (!clone->keys[i]) {
                for (size_t j = 0; j < i; j++) {
                    free(clone->keys[j]);
                }
                free(clone->keys);
                free(clone->data);
                free(clone);
                return create_return_error(self, JMAP_UNINITIALIZED, "strdup failed for key in clone");
            }
        } else {
            clone->keys[i] = NULL;
        }
    }

    JMAP_RETURN ret;
    ret.has_value = true;
    ret.has_error = false;
    ret.value = clone;
    ret.ret_source = self;
    return ret;
}

static JMAP_RETURN jmap_contains_key(const JMAP *self, const char *key) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    size_t idx = jmap_key_to_index(self, key);
    size_t probes = 0;

    while (probes < self->_capacity) {
        char *k = self->keys[idx];
        if (!k) {
            JMAP_RETURN ret = { .has_value = false, .has_error = false, .value = JMAP_DIRECT_INPUT(bool, false) };
            return ret;
        }
        if (strcmp(k, key) == 0) {
            JMAP_RETURN ret = { .has_value = true, .has_error = false, .value = JMAP_DIRECT_INPUT(bool, true) };
            return ret;
        }
        idx = (idx + 1) & (self->_capacity - 1);
        probes++;
    }

    JMAP_RETURN ret = { .has_value = false, .has_error = false, .value = JMAP_DIRECT_INPUT(bool, false), .ret_source = self };
    return ret;
}

static JMAP_RETURN jmap_contains_value(const JMAP *self, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!value)
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Value cannot be NULL");

    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL && memcmp((char*)self->data + i * self->_elem_size, value, self->_elem_size) == 0) {
            JMAP_RETURN ret = { .has_value = true, .has_error = false, .value = JMAP_DIRECT_INPUT(bool, true) };
            return ret;
        }
    }

    JMAP_RETURN ret = { .has_value = false, .has_error = false, .value = JMAP_DIRECT_INPUT(bool, false), .ret_source = self  };
    return ret;
}

static JMAP_RETURN jmap_get_keys(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no keys");

    char **keys_array = malloc(self->_length * sizeof(char*));
    if (!keys_array) {
        return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for keys array failed");
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
                return create_return_error(self, JMAP_UNINITIALIZED, "strdup failed for key");
            }
        }
    }

    JMAP_RETURN ret;
    ret.has_value = true;
    ret.has_error = false;
    ret.value = keys_array;
    ret.ret_source = self;
    return ret;
}

static JMAP_RETURN jmap_get_values(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no values");

    void *values_array = malloc(self->_length * self->_elem_size);
    if (!values_array) {
        return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for values array failed");
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

    JMAP_RETURN ret;
    ret.has_value = true;
    ret.has_error = false;
    ret.value = values_array;
    ret.ret_source = self;
    return ret;
}

static JMAP_RETURN jmap_for_each(const JMAP *self, void (*callback)(const char *key, void *value, const void *ctx), const void *ctx) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!callback)
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Callback function cannot be NULL");

    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            callback(self->keys[i], (char*)self->data + i * self->_elem_size, ctx);
        }
    }

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL, .ret_source = self };
    return ok;
}

static JMAP_RETURN jmap_is_empty(const JMAP *self) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");

    JMAP_RETURN ret;
    ret.has_value = true;
    ret.has_error = false;
    ret.value = JMAP_DIRECT_INPUT(bool, self->_length == 0);
    ret.ret_source = self;
    return ret;
}

static JMAP_RETURN jmap_put_if_absent(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    JMAP_RETURN exists = jmap_contains_key(self, key);
    if (exists.has_error) return exists;

    if (exists.has_value && JMAP_RET_GET_VALUE(bool, exists)) {
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" already exists", key);
    }

    return jmap_put(self, key, value);
}

static JMAP_RETURN jmap_remove(JMAP *self, const char *key){
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");
    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no keys to remove");

    JMAP_RETURN exists = jmap_contains_key(self, key);
    if (exists.has_error) return exists;

    if (exists.has_value && JMAP_RET_GET_VALUE(bool, exists)) {
        JMAP_RETURN get = jmap_get(self, key);
        if (get.has_error) return get;
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
            JMAP_RETURN rr = jmap_resize(self, self->_capacity / 2);
            if (rr.has_error) return rr;
        }
        JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL, .ret_source = self };
        return ok;
    }

    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static JMAP_RETURN jmap_remove_if_value_match(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    JMAP_RETURN exists = jmap_contains_key(self, key);
    if (exists.has_error) return exists;

    if (exists.has_value && JMAP_RET_GET_VALUE(bool, exists)) {
        JMAP_RETURN get = jmap_get(self, key);
        if (get.has_error) return get;
        if ((bool)self->user_implementation.is_equal_callback ? !self->user_implementation.is_equal_callback(get.value, value) : memcmp(get.value, value, self->_elem_size) != 0) {
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
            JMAP_RETURN rr = jmap_resize(self, self->_capacity / 2);
            if (rr.has_error) return rr;
        }
        JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL, .ret_source = self };
        return ok;
    }
    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static JMAP_RETURN jmap_remove_if_value_not_match(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key cannot be NULL or empty");

    JMAP_RETURN exists = jmap_contains_key(self, key);
    if (exists.has_error) return exists;

    if (exists.has_value && JMAP_RET_GET_VALUE(bool, exists)) {
        JMAP_RETURN get = jmap_get(self, key);
        if (get.has_error) return get;
        if ((bool)self->user_implementation.is_equal_callback ? self->user_implementation.is_equal_callback(get.value, value) : memcmp(get.value, value, self->_elem_size) == 0) {
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
            JMAP_RETURN rr = jmap_resize(self, self->_capacity / 2);
            if (rr.has_error) return rr;
        }
        JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL, .ret_source = self };
        return ok;
    }
    return create_return_error(self, JMAP_INVALID_ARGUMENT, "Key \"%s\" does not exist", key);
}

static JMAP_RETURN jmap_remove_if(JMAP *self, bool (*predicate)(const char *key, const void *value, const void *ctx), const void *ctx) {
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
        JMAP_RETURN rr = jmap_resize(self, self->_capacity / 2);
        if (rr.has_error) return rr;
    }

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL, .ret_source = self };
    return ok;
}

static void jmap_quick_sort(
    char **keys, 
    void *values, 
    size_t elem_size, 
    int (*compare)(const void *key_a, const void *value_a, const void *key_b, const void *value_b, const void *ctx),
    const void *ctx,
    int left, 
    int right
) {
    if (left >= right) return;

    int i = left, j = right;
    char *pivot_key = keys[(left + right) / 2];
    void *pivot_value = malloc(elem_size);
    memcpy(pivot_value, (char*)values + ((left + right) / 2) * elem_size, elem_size);

    while (i <= j) {
        while (compare(keys[i], (char*)values + i * elem_size, pivot_key, pivot_value, ctx) < 0) i++;
        while (compare(keys[j], (char*)values + j * elem_size, pivot_key, pivot_value, ctx) > 0) j--;

        if (i <= j) {
            // swap keys
            char *tmp_key = keys[i];
            keys[i] = keys[j];
            keys[j] = tmp_key;

            // swap values
            void *tmp_value = malloc(elem_size);
            memcpy(tmp_value, (char*)values + i * elem_size, elem_size);
            memcpy((char*)values + i * elem_size, (char*)values + j * elem_size, elem_size);
            memcpy((char*)values + j * elem_size, tmp_value, elem_size);
            free(tmp_value);

            i++;
            j--;
        }
    }

    free(pivot_value);

    if (left < j) jmap_quick_sort(keys, values, elem_size, compare, ctx, left, j);
    if (i < right) jmap_quick_sort(keys, values, elem_size, compare, ctx, i, right);
}


static JMAP_RETURN jmap_sort(
    JMAP *self,
    int (*compare)(const void *key_a, const void *value_a, const void *key_b, const void *value_b, const void *ctx),
    const void *ctx
) {
    if (!self->data || !self->keys)
        return create_return_error(self, JMAP_UNINITIALIZED, "JMAP is uninitialized");
    if (!compare)
        return create_return_error(self, JMAP_INVALID_ARGUMENT, "Compare function cannot be NULL");
    if (self->_length == 0)
        return create_return_error(self, JMAP_EMPTY, "JMAP is empty => no keys to sort");

    size_t count = self->_length;
    char **keys_array = malloc(count * sizeof(char*));
    void *values_array = malloc(count * self->_elem_size);
    if (!keys_array || !values_array) {
        free(keys_array);
        free(values_array);
        return create_return_error(self, JMAP_UNINITIALIZED, "Memory allocation for sorting failed");
    }

    size_t index = 0;
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            keys_array[index] = self->keys[i];
            memcpy((char*)values_array + index * self->_elem_size,
                   (char*)self->data + i * self->_elem_size,
                   self->_elem_size);
            index++;
        }
    }

    jmap_quick_sort(keys_array, values_array, self->_elem_size, compare, ctx, 0, (int)count - 1);

    index = 0;
    for (size_t i = 0; i < self->_capacity; i++) {
        if (self->keys[i] != NULL) {
            self->keys[i] = keys_array[index];
            memcpy((char*)self->data + i * self->_elem_size,
                   (char*)values_array + index * self->_elem_size,
                   self->_elem_size);
            index++;
        }
    }

    free(keys_array);
    free(values_array);

    return (JMAP_RETURN){
        .has_value = false,
        .has_error = false,
        .value = NULL,
        .ret_source = self
    };
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
}


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
    .sort = jmap_sort,
};