#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        return NULL; // File could not be opened
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL)
    {
        fclose(file);
        return NULL; // Memory allocation failed
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0'; // Null-terminate the string
    fclose(file);
    return buffer;
}

int write_file(const char *filename, const char *data)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        return -1; // File could not be opened for writing
    }

    fwrite(data, sizeof(char), strlen(data), file);
    fclose(file);
    return 0; // File written successfully
}
