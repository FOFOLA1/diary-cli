#include "linked_list.h"

#include <stdlib.h>
#include <string.h>

Node *ll_create_node(void *data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node)
    {
        return NULL; // Memory allocation failed
    }
    new_node->data = data;
    new_node->prev = NULL;
    new_node->next = NULL;
    return new_node;
}

void ll_insert_after(Node *current, void *data, Node **tail)
{
    if (current == NULL)
    {
        return; // Invalid current node
    }

    Node *new_node = ll_create_node(data);
    if (!new_node)
    {
        return; // Memory allocation failed
    }

    new_node->next = current->next;
    new_node->prev = current;
    if (current->next != NULL)
    {
        current->next->prev = new_node;
    }
    current->next = new_node;
    if (new_node->next == NULL && tail != NULL)
    {
        *tail = new_node;
    }
}

void ll_delete_node(Node **current, Node **head, Node **tail, free_data_func free_data)
{
    if (current == NULL || *current == NULL)
    {
        return; // Invalid current node
    }

    Node *to_delete = *current;
    Node *new_current = NULL;

    if (to_delete->next != NULL)
    {
        to_delete->next->prev = to_delete->prev;
        new_current = to_delete->next;
    }
    if (to_delete->prev != NULL)
    {
        to_delete->prev->next = to_delete->next;
        new_current = to_delete->prev;
    }

    if (to_delete == *head)
    {
        *head = to_delete->next;
    }
    if (to_delete == *tail)
    {
        *tail = to_delete->prev;
    }

    free_data(to_delete->data);
    free(to_delete);
    *current = new_current;
}

void ll_free_list(Node **head, void (*free_data)(void *))
{
    if (head == NULL || *head == NULL)
    {
        return; // Invalid node
    }

    if ((*head)->next != NULL)
    {
        ll_free_list(&(*head)->next, free_data);
    }
    if ((*head)->data != NULL)
    {
        free_data((*head)->data);
    }
    free(*head);
    *head = NULL;
}

void ll_prev_node(Node **current)
{
    if (current == NULL || *current == NULL)
    {
        return;
    }

    if ((*current)->prev != NULL)
    {
        *current = (*current)->prev;
    }
}

void ll_next_node(Node **current)
{
    if (current == NULL || *current == NULL)
    {
        return;
    }

    if ((*current)->next != NULL)
    {
        *current = (*current)->next;
    }
}

int ll_to_json_string(Node *head, char **json_str, json_serializer serializer)
{
    if (head == NULL || json_str == NULL || serializer == NULL)
    {
        return -1; // Invalid input
    }

    size_t total_size = 2048;
    size_t used_size = 0;
    *json_str = (char *)malloc(total_size);
    if (*json_str == NULL)
    {
        return -1; // Memory allocation failed
    }

    strcpy(*json_str, "["); // Start of JSON array
    used_size = strlen(*json_str);

    Node *current = head;
    while (current != NULL)
    {
        char buffer[512];
        if (serializer(current->data, buffer, sizeof(buffer)) == 0)
        {
            size_t buffer_len = strlen(buffer);

            // Check if we need to resize the buffer
            if (used_size + buffer_len + 2 > total_size) // +2 for ',' or ']' and null terminator
            {
                total_size *= 2;
                char *new_json_str = (char *)realloc(*json_str, total_size);
                if (new_json_str == NULL)
                {
                    free(*json_str);
                    *json_str = NULL;
                    return -1; // Memory allocation failed
                }
                *json_str = new_json_str;
            }

            strcat(*json_str, buffer);
            used_size += buffer_len;
        }

        current = current->next;
        if (current != NULL)
        {
            if (used_size + 1 > total_size) // +1 for ',' and null terminator
            {
                total_size *= 2;
                char *new_json_str = (char *)realloc(*json_str, total_size);
                if (new_json_str == NULL)
                {
                    free(*json_str);
                    *json_str = NULL;
                    return -1; // Memory allocation failed
                }
                *json_str = new_json_str;
            }

            strcat(*json_str, ",");
            used_size += 1;
        }
    }

    if (used_size + 1 > total_size) // +1 for ']' and null terminator
    {
        total_size += 1;
        char *new_json_str = (char *)realloc(*json_str, total_size);
        if (new_json_str == NULL)
        {
            free(*json_str);
            *json_str = NULL;
            return -1; // Memory allocation failed
        }
        *json_str = new_json_str;
    }

    strcat(*json_str, "]"); // End of JSON array

    return 0; // Success
}

int ll_from_json_string(const char *json_str,
                        Node **head,
                        Node **tail,
                        json_deserializer deserializer,
                        int *length,
                        size_t data_size)
{
    if (json_str == NULL || head == NULL || tail == NULL || deserializer == NULL || length == NULL)
    {
        return -1; // Invalid input
    }

    if (data_size == 0)
    {
        return -1; // Cannot allocate zero-sized records
    }

    const char *ptr = json_str;
    while (*ptr != '\0')
    {
        while (*ptr != '{' && *ptr != '\0')
        {
            ptr++;
        }
        if (*ptr == '\0')
        {
            break; // No more objects
        }

        const char *start = ptr;
        int brace_count = 0;
        do
        {
            if (*ptr == '{')
            {
                brace_count++;
            }
            else if (*ptr == '}')
            {
                brace_count--;
            }
            ptr++;
        } while (brace_count > 0 && *ptr != '\0');

        if (brace_count != 0)
        {
            return -1; // Mismatched braces
        }

        size_t obj_size = ptr - start;
        char *obj_str = (char *)malloc(obj_size + 1);
        if (obj_str == NULL)
        {
            return -1; // Memory allocation failed
        }
        strncpy(obj_str, start, obj_size);
        obj_str[obj_size] = '\0';

        void *data = calloc(1, data_size);
        if (data == NULL)
        {
            free(obj_str);
            return -1; // Memory allocation failed
        }

        if (deserializer(data, obj_str, obj_size) != 0)
        {
            free(obj_str);
            free(data);
            return -1; // Deserialization failed
        }

        Node *new_node = ll_create_node(data);
        if (new_node == NULL)
        {
            free(obj_str);
            free(data);
            return -1; // Memory allocation failed
        }

        if (*head == NULL)
        {
            *head = new_node;
            *tail = new_node;
        }
        else
        {
            (*tail)->next = new_node;
            new_node->prev = *tail;
            *tail = new_node;
        }

        free(obj_str);
        (*length)++;
    }

    return 0;
}