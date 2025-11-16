#ifndef FILE_H
#define FILE_H

/**
 * @brief Reads the contents of a file.
 * @param filename The name of the file to read.
 * @return buffer containing the file contents, or NULL on failure.
 */
char *read_file(const char *filename);

/**
 * @brief Writes the contents of a string to a file.
 * @param filename The name of the file to write to.
 * @param data The string to write to the file.
 * @return 0 on success, or -1 on failure.
 */
int write_file(const char *filename, const char *data);

#endif // FILE_H