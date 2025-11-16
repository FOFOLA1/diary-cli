#include "i18n.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// String hash function (djb2)
static unsigned long hash_string(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}

// Helper to duplicate a string
static char *custom_strdup(const char *s)
{
    if (!s)
        return NULL;
    char *d = malloc(strlen(s) + 1);
    if (!d)
        return NULL;
    strcpy(d, s);
    return d;
}

// Helper to trim whitespace from start and end of a string (in-place)
static char *trim_whitespace(char *str)
{
    char *end;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return str;
}

// Convert simple escape sequences (e.g., \n) into their literal characters in-place.
static void unescape_in_place(char *str)
{
    char *read = str;
    char *write = str;

    while (*read)
    {
        if (*read == '\\')
        {
            read++;
            if (*read == '\0')
                break;

            switch (*read)
            {
            case 'n':
                *write++ = '\n';
                break;
            case 't':
                *write++ = '\t';
                break;
            case 'r':
                *write++ = '\r';
                break;
            case '\\':
                *write++ = '\\';
                break;
            case '"':
                *write++ = '"';
                break;
            default:
                *write++ = '\\';
                *write++ = *read;
                break;
            }
        }
        else
        {
            *write++ = *read;
        }

        read++;
    }

    *write = '\0';
}

// --- Public Function Implementations ---

TranslationMap *i18n_create_map(int size)
{
    if (size <= 0)
        return NULL;

    TranslationMap *map = malloc(sizeof(TranslationMap));
    if (!map)
        return NULL;

    map->size = size;
    // Use calloc to zero-initialize all bucket pointers to NULL
    map->buckets = calloc(size, sizeof(TranslationEntry *));
    if (!map->buckets)
    {
        free(map);
        return NULL;
    }
    return map;
}

// Internal function to add a key/value pair to the map
static void map_put(TranslationMap *map, const char *key, const char *value)
{
    if (!map || !key || !value)
        return;

    unsigned long hash = hash_string(key);
    int index = hash % map->size;

    // Create the new entry
    TranslationEntry *new_entry = malloc(sizeof(TranslationEntry));
    if (!new_entry)
        return; // Allocation failed

    new_entry->key = custom_strdup(key);
    new_entry->value = custom_strdup(value);
    if (!new_entry->key || !new_entry->value)
    {
        free(new_entry->key);
        free(new_entry->value);
        free(new_entry);
        return; // Allocation failed
    }

    // Insert at the head of the chain (collision)
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
}

int i18n_load_translations(const char *filepath, TranslationMap *map, const char *language)
{
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        perror("Error opening translation file");
        return -1;
    }

    char line[512];
    char target_section[70];
    snprintf(target_section, sizeof(target_section), "[%s]", language);

    int reading_target_lang = 0;

    while (fgets(line, sizeof(line), file))
    {
        char *trimmed_line = trim_whitespace(line);

        // Skip comments and empty lines
        if (trimmed_line[0] == '#' || trimmed_line[0] == '\0')
        {
            continue;
        }

        // Check for a new section
        if (trimmed_line[0] == '[')
        {
            if (strcmp(trimmed_line, target_section) == 0)
            {
                reading_target_lang = 1; // Found our language
            }
            else
            {
                reading_target_lang = 0; // Not our language
            }
            continue;
        }

        // If we are in the target section, parse the key-value pair
        if (reading_target_lang)
        {
            char *equals = strchr(trimmed_line, '=');
            if (equals)
            {
                *equals = '\0'; // Split the string at '='
                char *key = trim_whitespace(trimmed_line);
                char *value = trim_whitespace(equals + 1);
                unescape_in_place(value);

                if (key[0] != '\0')
                {
                    map_put(map, key, value);
                }
            }
        }
    }

    fclose(file);
    return 0;
}

const char *i18n_get_string(const TranslationMap *map, const char *key)
{
    if (!map || !key)
        return key; // Fallback to key

    unsigned long hash = hash_string(key);
    int index = hash % map->size;

    TranslationEntry *entry = map->buckets[index];
    while (entry)
    {
        if (strcmp(entry->key, key) == 0)
        {
            return entry->value;
        }
        entry = entry->next;
    }

    return key; // Fallback: key not found
}

void i18n_free_map(TranslationMap *map)
{
    if (!map)
        return;

    for (int i = 0; i < map->size; i++)
    {
        TranslationEntry *entry = map->buckets[i];
        while (entry)
        {
            TranslationEntry *next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
    free(map);
}

int i18n_load_translations_from_memory(const char *buffer, TranslationMap *map, const char *language)
{
    if (!buffer || !map || !language)
        return -1;

    char line[512];
    char target_section[70];
    snprintf(target_section, sizeof(target_section), "[%s]", language);

    int reading_target_lang = 0;
    const char *line_start = buffer;

    while (*line_start != '\0')
    {
        const char *line_end = strchr(line_start, '\n');
        size_t line_len;

        if (line_end)
        {
            line_len = line_end - line_start;
        }
        else
        {
            line_len = strlen(line_start);
            line_end = line_start + line_len;
        }

        if (line_len < sizeof(line))
        {
            strncpy(line, line_start, line_len);
            line[line_len] = '\0';

            char *trimmed_line = trim_whitespace(line);

            if (trimmed_line[0] != '#' && trimmed_line[0] != '\0')
            {
                if (trimmed_line[0] == '[')
                {
                    reading_target_lang = (strcmp(trimmed_line, target_section) == 0);
                }
                else if (reading_target_lang)
                {
                    char *equals = strchr(trimmed_line, '=');
                    if (equals)
                    {
                        *equals = '\0';
                        char *key = trim_whitespace(trimmed_line);
                        char *value = trim_whitespace(equals + 1);
                        unescape_in_place(value);
                        if (key[0] != '\0')
                        {
                            map_put(map, key, value);
                        }
                    }
                }
            }
        }

        if (*line_end == '\n')
        {
            line_start = line_end + 1;
        }
        else
        {
            line_start = line_end;
        }
    }
    return 0;
}