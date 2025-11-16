#ifndef I18N_H
#define I18N_H

#include <stdlib.h>

// A single entry in the hash map (uses chaining for collisions)
typedef struct TranslationEntry
{
    char *key;
    char *value;
    struct TranslationEntry *next;
} TranslationEntry;

// The hash map structure
typedef struct
{
    int size;
    TranslationEntry **buckets;
} TranslationMap;

/**
 * @brief Creates a new, empty translation map.
 * @param size The number of buckets (a prime number is good, e.g., 101).
 * @return A pointer to the new TranslationMap, or NULL on failure.
 */
TranslationMap *i18n_create_map(int size);

/**
 * @brief Loads translations from a file for a specific language.
 * @param filepath Path to the .ini file.
 * @param map The map to load strings into.
 * @param language The language code to load (e.g., "en", "es").
 * @return 0 on success, -1 on failure (e.g., file not found).
 */
int i18n_load_translations(const char *filepath, TranslationMap *map, const char *language);

/**
 * @brief Gets a translated string from the map.
 * @param map The map containing the translations.
 * @param key The key of the string to find (e.g., "GREETING").
 * @return The translated string. If not found, returns the key itself as a fallback.
 */
const char *i18n_get_string(const TranslationMap *map, const char *key);

/**
 * @brief Frees all memory associated with the translation map.
 * @param map The map to free.
 */
void i18n_free_map(TranslationMap *map);

/**
 * @brief Loads translations from an in-memory string buffer.
 * @param buffer A null-terminated string containing the INI data.
 * @param map The map to load strings into.
 * @param language The language code to load (e.g., "en", "es").
 * @return 0 on success, -1 on failure.
 */
int i18n_load_translations_from_memory(const char *buffer, TranslationMap *map, const char *language);

#endif // I18N_H