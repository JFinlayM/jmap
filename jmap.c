#include "jmap.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "third_party/murmur3-master/murmur3.h"

static const char *enum_to_string[] = {
    [INDEX_OUT_OF_BOUND]                    = "Index out of bound",
    [ARRAY_UNINITIALIZED]                   = "JARRAY uninitialized",
    [DATA_NULL]                             = "Data is null",
    [PRINT_ELEMENT_CALLBACK_UNINTIALIZED]   = "Print callback not set",
    [EMPTY_ARRAY]                           = "Empty array",
    [INVALID_ARGUMENT]                      = "Invalid argument",
    [ELEMENT_NOT_FOUND]                     = "Element not found",
    [UNIMPLEMENTED_FUNCTION]                = "Function not implemented",
};


static void print_array_err(const JMAP_RETURN ret) {
    if (ret.has_value) return;
    if (!ret.has_error) return;
    if (ret.error.error_code < 0 || ret.error.error_code >= sizeof(enum_to_string) / sizeof(enum_to_string[0]) || enum_to_string[ret.error.error_code] == NULL) {
        fprintf(stderr, "[\033[31mUnknown error code: %d\033[0m]", ret.error.error_code);
    } else {
        fprintf(stderr, "[\033[31mError: %s\033[0m]", enum_to_string[ret.error.error_code]);
    }
    if (ret.error.error_msg) {
        fprintf(stderr, "\t%s\n", ret.error.error_msg);
    }
    free((void*)ret.error.error_msg);
}

static JMAP_RETURN create_return_error(JMAP_ERROR error_code, const char* fmt, ...) {
    JMAP_RETURN ret;
    ret.has_value = false;
    ret.has_error = true;
    va_list args;
    va_start(args, fmt);

    char *buf = malloc(256);
    if (!buf) {
        ret.error.error_msg = "Memory allocation failed";
        ret.error.error_code = ARRAY_UNINITIALIZED;
        va_end(args);
        return ret;
    }

    vsnprintf(buf, 256, fmt, args);
    va_end(args);

    ret.error.error_code = error_code;
    ret.error.error_msg = buf;
    return ret;
}

static JMAP_RETURN jmap_init(JMAP *array, size_t _elem_size) {
    array->_length = 16;
    array->_elem_size = _elem_size;
    array->_key_max_length = 50;
    array->data = malloc(array->_length * array->_elem_size);
    if (array->data == NULL) {
        return create_return_error(ARRAY_UNINITIALIZED, "Memory allocation for data failed");
    }
    memset(array->data, 0, array->_length * array->_elem_size);
    array->keys = malloc(array->_length * sizeof(char*));
    if (array->keys == NULL) {
        free(array->data);
        return create_return_error(ARRAY_UNINITIALIZED, "Memory allocation for keys failed");
    }
    for (size_t i = 0; i < array->_length; i++) {
        array->keys[i] = NULL;  // libre
    }
    

    array->user_implementation.print_element_callback = NULL;

    JMAP_RETURN ret;
    ret.has_value = false;
    ret.has_error = false;
    ret.value = NULL;
    return ret;
}

static size_t jmap_key_to_index(const JMAP *self, const char *key) {
    if (key == NULL) create_return_error(INVALID_ARGUMENT, "Key cannot be NULL");
    if (strlen(key) == 0) create_return_error(INVALID_ARGUMENT, "Key cannot be empty");
    size_t output;
    uint32_t hash;
    MurmurHash3_x86_32(key, strlen(key), 42, &hash);
    output = hash & (self->_length - 1); // Assuming _length is a power of 2
    return output;
}

static JMAP_RETURN jmap_resize(JMAP *self, size_t new_length) {
    // new_length doit être une puissance de 2 pour le mask (& (len-1))
    if ((new_length & (new_length - 1)) != 0) {
        return create_return_error(INVALID_ARGUMENT, "new_length must be power of two");
    }

    char **old_keys   = self->keys;
    void  *old_data   = self->data;
    size_t old_length = self->_length;

    // Nouvelles tables vides
    char **new_keys = calloc(new_length, sizeof(char*));
    if (!new_keys) return create_return_error(ARRAY_UNINITIALIZED, "alloc keys failed");
    void *new_data = calloc(new_length, self->_elem_size);
    if (!new_data) {
        free(new_keys);
        return create_return_error(ARRAY_UNINITIALIZED, "alloc data failed");
    }

    // Prépare l’objet pour réinsertion
    self->keys    = new_keys;
    self->data    = new_data;
    self->_length = new_length;

    size_t old_count = self->_elem_nb;
    self->_elem_nb   = 0;

    // Réinsérer toutes les clés existantes (on réutilise les pointeurs strdupés)
    for (size_t i = 0; i < old_length; i++) {
        char *k = old_keys[i];
        if (!k) continue;

        size_t idx = jmap_key_to_index(self, k);
        while (self->keys[idx] != NULL) {
            idx = (idx + 1) & (self->_length - 1);
        }

        self->keys[idx] = k; // on déplace le pointeur, pas de strdup ici
        // Copier la valeur associée à l’ancienne position i
        memcpy((char*)self->data + idx * self->_elem_size,
               (char*)old_data    + i   * self->_elem_size,
               self->_elem_size);
        self->_elem_nb++;
    }

    // On libère juste les conteneurs (les chaînes ont été déplacées)
    free(old_keys);
    free(old_data);

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    // Sanity: on s’attend à récupérer le même nombre d’éléments
    (void)old_count; // si tu veux, tu peux assert(old_count == self->_elem_nb);
    return ok;
}


static JMAP_RETURN jmap_put(JMAP *self, const char *key, const void *value) {
    if (!self->data || !self->keys)
        return create_return_error(ARRAY_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(INVALID_ARGUMENT, "Key cannot be NULL or empty");

    // Agrandir si load factor > 0.5
    if (self->_elem_nb + 1 > (self->_length / 2)) {
        JMAP_RETURN rr = jmap_resize(self, self->_length * 2);
        if (rr.has_error) return rr;
    }

    // Insertion / update
    size_t idx = jmap_key_to_index(self, key);
    while (self->keys[idx] != NULL && strcmp(self->keys[idx], key) != 0) {
        idx = (idx + 1) & (self->_length - 1);
    }

    if (self->keys[idx] == NULL) {
        // Nouvelle clé
        self->keys[idx] = strdup(key);
        if (!self->keys[idx])
            return create_return_error(ARRAY_UNINITIALIZED, "strdup failed for key");
        self->_elem_nb++;
    } else {
        // Mise à jour : on peut remplacer la valeur sans toucher la clé
        // (ou free + strdup si tu veux permettre une nouvelle adresse)
    }

    memcpy((char*)self->data + idx * self->_elem_size, value, self->_elem_size);

    JMAP_RETURN ok = { .has_value = false, .has_error = false, .value = NULL };
    return ok;
}


static JMAP_RETURN jmap_get(JMAP *self, const char *key) {
    if (!self->data || !self->keys)
        return create_return_error(ARRAY_UNINITIALIZED, "JMAP is uninitialized");
    if (!key || key[0] == '\0')
        return create_return_error(INVALID_ARGUMENT, "Key cannot be NULL or empty");

    size_t idx = jmap_key_to_index(self, key);
    size_t probes = 0;

    while (probes < self->_length) {
        char *k = self->keys[idx];
        if (!k) {
            // trou vide → la clé n’est pas dans la table
            return create_return_error(ELEMENT_NOT_FOUND, "Key not found");
        }
        if (strcmp(k, key) == 0) {
            JMAP_RETURN ret = {0};
            ret.has_value = true;
            ret.has_error = false;
            ret.value = (char*)self->data + idx * self->_elem_size; // pointeur dans la table
            return ret;
        }
        idx = (idx + 1) & (self->_length - 1);
        probes++;
    }

    return create_return_error(ELEMENT_NOT_FOUND, "Key not found after full probe");
}


static void jmap_free(JMAP *self) {
    if (self->data != NULL) {
        free(self->data);
        self->data = NULL;
    }
    if (self->keys != NULL) {
        for (size_t i = 0; i < self->_length; i++) {
            free(self->keys[i]);
        }
        free(self->keys);
        self->keys = NULL;
    }
    self->_elem_nb = 0;
    self->_length = 0;
    self->_elem_size = 0;
    self->_key_max_length = 0;
}

JMAP_INTERFACE jmap = {
    .init = jmap_init,
    .print_array_err = print_array_err,
    .put = jmap_put,
    .get = jmap_get,
    .free = jmap_free,
};