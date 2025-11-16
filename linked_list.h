#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

// A node in a doubly linked list
typedef struct Node
{
    void *data;
    struct Node *prev;
    struct Node *next;
} Node;

/**
 * @brief Creates a new node with the given data.
 * @param data The data to store in the new node.
 * @return A pointer to the new node, or NULL on failure.
 */
Node *ll_create_node(void *data);

/**
 * @brief Inserts a new node with the given data after the current node.
 * @param current A pointer to the current node.
 * @param data The data to store in the new node.
 * @param tail A pointer to the tail pointer of the list, updated if a new tail is created.
 */
void ll_insert_after(Node *current, void *data, Node **tail);

/**
 * @brief A function pointer type for a function that frees a node's data.
 * @param data A pointer to the data to be freed.
 */
typedef void (*free_data_func)(void *);

/**
 * @brief Deletes a node, frees its memory, updates current pointer and links the neighboring nodes.
 * @param current A pointer to the node to delete.
 */
void ll_delete_node(Node **current, Node **head, Node **tail, free_data_func free_data);

/**
 * @brief Frees whole Linked list.
 * @param head A pointer to the head of the linked list to delete.
 */
void ll_free_list(Node **head, void (*free_data)(void *));

/**
 * @brief Moves pointer to the previous node.
 * @param current A pointer to the current node.
 */
void ll_prev_node(Node **current);

/**
 * @brief Moves pointer to the next node.
 * @param current A pointer to the current node.
 */
void ll_next_node(Node **current);

/**
 * @brief A function pointer type for a function that serializes a node's data to a JSON string.
 * @param data A pointer to the data to be serialized.
 * @param buffer The buffer to write the JSON string into.
 * @param buffer_size The size of the buffer.
 * @return 0 on success, non-zero on failure.
 */
typedef int (*json_serializer)(void *data, char *buffer, size_t buffer_size);

/**
 * @brief A function pointer type for a function that deserializes a JSON string into a node's data.
 * @param data A pointer to the data to be deserialized.
 * @param buffer The JSON string to read from.
 * @param buffer_size The size of the buffer.
 * @return 0 on success, non-zero on failure.
 */
typedef int (*json_deserializer)(void *data, const char *buffer, size_t buffer_size);

/**
 * @brief Converts a linked list to a JSON array string.
 * @param head The head of the list.
 * @param json_str A pointer to the char* that will hold the resulting JSON string.
 * @param serializer The function to use for serializing each node's data.
 * @return 0 on success, -1 on failure.
 */
int ll_to_json_string(Node *head, char **json_str, json_serializer serializer);

/**
 * @brief Parses a JSON array string and populates a linked list.
 * @param json_str The JSON string to parse.
 * @param head A pointer to the head of the list to populate.
 * @param tail A pointer to the tail of the list to populate.
 * @param deserializer The function to use for deserializing each node's data.
 * @return 0 on success, -1 on failure.
 */
int ll_from_json_string(const char *json_str,
                        Node **head,
                        Node **tail,
                        json_deserializer deserializer,
                        int *length,
                        size_t data_size);

#endif // LINKED_LIST_H