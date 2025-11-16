#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "i18n.h"
#include "strings.h"
#include "file.h"
#include "linked_list.h"

#if defined(_WIN32)
static ssize_t portable_getline(char **lineptr, size_t *n, FILE *stream)
{
    if (lineptr == NULL || n == NULL || stream == NULL)
    {
        return -1;
    }

    if (*lineptr == NULL || *n == 0)
    {
        size_t initial_size = 128;
        char *new_buffer = (char *)malloc(initial_size);
        if (new_buffer == NULL)
        {
            return -1;
        }
        *lineptr = new_buffer;
        *n = initial_size;
    }

    size_t position = 0;
    int ch = 0;

    while ((ch = fgetc(stream)) != EOF)
    {
        if (position + 1 >= *n)
        {
            size_t new_size = (*n) * 2;
            char *resized = (char *)realloc(*lineptr, new_size);
            if (resized == NULL)
            {
                return -1;
            }
            *lineptr = resized;
            *n = new_size;
        }

        (*lineptr)[position++] = (char)ch;

        if (ch == '\n')
        {
            break;
        }
    }

    if (position == 0 && ch == EOF)
    {
        return -1;
    }

    (*lineptr)[position] = '\0';
    return (ssize_t)position;
}

#define getline portable_getline
#endif

#define _(s) i18n_get_string(translations, s)

typedef struct Record
{
    char day;
    char month;
    short year;
    char *note;
} Record;

int serialize_record(void *data, char *buffer, size_t buffer_size);
int deserialize_record(void *data, const char *json_str, size_t json_size);
static void rtrim(char *str);
static int command_matches(char *input, const char *key);
static int new_entry();
static void print_help();
static void clear_screen();
static int get_date(char *date, int *day, int *month, int *year);
void free_record(Record *rec);
static int del_entry();
static void save_data();

static TranslationMap *translations = NULL;

// DECLARATIONS
char *separator_string = "------------------------------------------------------";
char *data_file = "diary.json";

char *line = NULL;
size_t line_capacity = 0;

Node *head = NULL;
Node *tail = NULL;
Node *current = NULL;
int num_records = 0;

int main()
{
    // LANG
    const char *lang_env = getenv("LANG");
    const char *lang = (lang_env && strncmp(lang_env, "cs", 2) == 0) ? "cs" : "en";

    translations = i18n_create_map(21);
    if (translations == NULL)
    {
        fprintf(stderr, "Failed to allocate translation map.\n");
        return EXIT_FAILURE;
    }

    // if (i18n_load_translations("strings.ini", translations, lang) != 0)
    if (i18n_load_translations_from_memory((const char *)strings_ini, translations, lang) != 0)
    {
        fprintf(stderr, "Failed to load translations for language '%s'.\n", lang);
        i18n_free_map(translations);
        translations = NULL;
        return EXIT_FAILURE;
    }

    // LINKED LIST
    char *file_content = read_file(data_file);
    if (file_content != NULL)
    {
        if (ll_from_json_string(file_content,
                                &head,
                                &tail,
                                deserialize_record,
                                &num_records,
                                sizeof(Record)) != 0)
        {
            fprintf(stderr, "Failed to load diary entries from file.\n");
            i18n_free_map(translations);
            translations = NULL;
            return EXIT_FAILURE;
        }
        free(file_content);
        current = tail;
    }

    // MAIN LOGIC
    while (1)
    {
        clear_screen();
        print_help();

        if (current != NULL)
        {
            printf("%s: %d.%d.%d\n\n%s\n%s\n\n", _("date"),
                   ((Record *)current->data)->day,
                   ((Record *)current->data)->month,
                   ((Record *)current->data)->year,
                   ((Record *)current->data)->note ? ((Record *)current->data)->note : "",
                   separator_string);
        }

        printf("%s: ", _("enter_command"));
        ssize_t read = getline(&line, &line_capacity, stdin);

        if (read == -1)
        {
            break; // EOF or error
        }
        else if (command_matches(line, "cmd_prev"))
        {
            ll_prev_node(&current);
        }
        else if (command_matches(line, "cmd_next"))
        {
            ll_next_node(&current);
        }
        else if (command_matches(line, "cmd_new"))
        {
            new_entry();
        }
        else if (command_matches(line, "cmd_save"))
        {
            save_data();
        }
        else if (command_matches(line, "cmd_delete"))
        {
            del_entry();
        }
        else if (command_matches(line, "cmd_close"))
        {
            break;
        }
        else
        {
        }
    }

    // CLEANUP
    ll_free_list(&head, (free_data_func)free_record);
    free(line);
    line = NULL;
    line_capacity = 0;
    i18n_free_map(translations);
    translations = NULL;

    return 0;
}

// Serializer function for the Record struct
int serialize_record(void *data, char *buffer, size_t buffer_size)
{
    if (data == NULL || buffer == NULL)
    {
        return -1;
    }
    Record *rec = (Record *)data;
    // Use snprintf for safe string formatting
    int result = snprintf(buffer, buffer_size, "{\"day\": %d, \"month\": %d, \"year\": %d, \"note\": \"%s\"}",
                          rec->day, rec->month, rec->year, rec->note ? rec->note : "");

    if (result < 0 || (size_t)result >= buffer_size)
    {
        return -1; // Encoding error or buffer too small
    }
    return 0;
}

int deserialize_record(void *data, const char *json_str, size_t json_size)
{
    (void)json_size;
    if (data == NULL || json_str == NULL)
    {
        return -1;
    }
    Record *rec = (Record *)data;

    int day = 0;
    int month = 0;
    short year = 0;
    char note_buffer[512] = {0};

    if (sscanf(json_str, "{\"day\": %d, \"month\": %d, \"year\": %hd, \"note\": \"%511[^\"]\"}",
               &day, &month, &year, note_buffer) != 4)
    {
        return -1;
    }

    rec->day = (char)day;
    rec->month = (char)month;
    rec->year = year;

    free(rec->note);
    rec->note = NULL;

    size_t note_len = strlen(note_buffer);
    rec->note = (char *)malloc(note_len + 1);
    if (rec->note == NULL)
    {
        return -1;
    }
    memcpy(rec->note, note_buffer, note_len + 1);

    return 0;
}

static void rtrim(char *str)
{
    if (!str)
    {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }
}

static int command_matches(char *input, const char *key)
{
    if (!input || !key)
    {
        return 0;
    }
    const char *localized = _(key);
    if (!localized)
    {
        return 0;
    }
    rtrim(input);
    return strcmp(input, localized) == 0;
}

static int new_entry()
{
    clear_screen();
    print_help();

    printf("\n%s: ", _("enter_date"));
    ssize_t read = getline(&line, &line_capacity, stdin);
    if (read == -1)
    {
        return -1;
    }

    int day = 0;
    int month = 0;
    int year = 0;
    if (!get_date(line, &day, &month, &year))
    {
        return -1;
    }

    char *note_buffer = NULL;
    size_t note_len = 0;

    printf("%s:\n", _("enter_note"));
    while (1)
    {
        read = getline(&line, &line_capacity, stdin);
        if (read == -1)
        {
            free(note_buffer);
            return -1;
        }

        size_t line_bytes = (size_t)read;
        char *line_copy = (char *)malloc(line_bytes + 1);
        if (line_copy == NULL)
        {
            free(note_buffer);
            return -1;
        }
        memcpy(line_copy, line, line_bytes + 1);

        int should_save = command_matches(line_copy, "cmd_save");
        free(line_copy);
        if (should_save)
        {
            break;
        }

        char *temp_ptr = (char *)realloc(note_buffer, note_len + line_bytes + 1);
        if (temp_ptr == NULL)
        {
            free(note_buffer);
            return -1;
        }
        note_buffer = temp_ptr;

        memcpy(note_buffer + note_len, line, line_bytes);
        note_len += line_bytes;
        note_buffer[note_len] = '\0';
    }

    Record *new_record = (Record *)malloc(sizeof(Record));
    if (new_record == NULL)
    {
        free(note_buffer);
        return -1;
    }
    new_record->day = (char)day;
    new_record->month = (char)month;
    new_record->year = (short)year;
    new_record->note = note_buffer;

    if (current == NULL)
    {
        Node *new_node = ll_create_node(new_record);
        if (new_node == NULL)
        {
            free(note_buffer);
            free(new_record);
            return -1;
        }
        head = new_node;
        tail = new_node;
        current = new_node;
    }
    else
    {
        Node *previous_next = current->next;
        ll_insert_after(current, new_record, &tail);
        if (current->next == previous_next)
        {
            free(note_buffer);
            free(new_record);
            return -1;
        }
        ll_next_node(&current);
    }

    num_records++;

    save_data();

    return 0;
}

static void print_help()
{
    printf("%s\n%s\n%s\n\n%s: %d\n", separator_string, _("help"), separator_string, _("record_num"), num_records);
}

static void clear_screen()
{
    printf("\e[1;1H\e[2J");
}

static int get_date(char *date, int *day, int *month, int *year)
{
    rtrim(date);
    int sday, smonth, syear, chars_read;
    int result = sscanf(date, "%d.%d.%d%n", &sday, &smonth, &syear, &chars_read);

    if (result != 3 || (size_t)chars_read != strlen(date))
    {
        return 0;
    }

    if (syear < 1)
    {
        return 0;
    }
    if (smonth < 1 || smonth > 12)
    {
        return 0;
    }

    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if ((syear % 4 == 0 && syear % 100 != 0) || (syear % 400 == 0))
    {
        days_in_month[2] = 29; // February has 29 days
    }

    if (sday < 1 || sday > days_in_month[smonth])
    {
        return 0;
    }
    *day = sday;
    *month = smonth;
    *year = syear;
    return 1;
}

void free_record(Record *rec)
{
    if (rec != NULL)
    {
        free(rec->note);
        free(rec);
    }
}

static int del_entry()
{
    if (current == NULL)
    {
        return -1;
    }

    clear_screen();

    printf("\n%s: %d.%d.%d\n\n%s\n%s\n\n%s: ", _("date"),
           ((Record *)current->data)->day,
           ((Record *)current->data)->month,
           ((Record *)current->data)->year,
           ((Record *)current->data)->note ? ((Record *)current->data)->note : "",
           separator_string,
           _("delete_confirm"));

    ssize_t read = getline(&line, &line_capacity, stdin);
    if (read == -1)
    {
        return -1;
    }

    const char *confirm = _("cmd_confirm");
    if (command_matches(line, "cmd_confirm") ||
        (confirm && confirm[0] != '\0' && read > 0 && confirm[0] == line[0]))
    {
        ll_delete_node(&current, &head, &tail, (free_data_func)free_record);
        num_records--;
    }
    save_data();
    return 0;
}

static void save_data()
{
    char *json = NULL;
    if (ll_to_json_string(head, &json, serialize_record) == 0)
    {
        write_file(data_file, json);
    }
    free(json);
}