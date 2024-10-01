#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenarray.h"

struct Token *create(int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt)
{
    struct Token *newToken = malloc(sizeof(struct Token));
    if (newToken == NULL)
    {
        fprintf(stderr, "Memory allocation for new Token failed\n");
        exit(EXIT_FAILURE);
    }
    newToken->lineNum = lineNum;
    strncpy(newToken->lex, lex, MAX_TOKEN_LENGTH - 1);
    newToken->lex[MAX_TOKEN_LENGTH - 1] = '\0';  // Ensure null termination
    strncpy(newToken->tokName, tokName, MAX_TOKEN_LENGTH - 1);
    newToken->tokName[MAX_TOKEN_LENGTH - 1] = '\0';  // Ensure null termination
    newToken->tokInt = tokInt;
    strncpy(newToken->attrName, attrName, MAX_TOKEN_LENGTH - 1);
    newToken->attrName[MAX_TOKEN_LENGTH - 1] = '\0';  // Ensure null termination
    newToken->attrInt = attrInt;

    return newToken;
}


// Append a new token to the dynamic array
void append(struct Token **tokArr, int *tokArrSize, int *tokArrCap, int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt)
{
    // Check if we need to grow the array
    if (*tokArrSize >= *tokArrCap)
    {
        *tokArrCap *= 2;
        struct Token *temp = realloc(*tokArr, (*tokArrCap) * sizeof(struct Token));
        if (temp == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        *tokArr = temp;
    }

    // Create a new token and append it
    struct Token newToken = *create(lineNum, lex, tokName, tokInt, attrName, attrInt);
    (*tokArr)[*tokArrSize] = newToken;
    (*tokArrSize)++;
}

// Insert a token at a specific index
void insertN(struct Token ***tokArr, int *tokArrSize, int *tokArrCap, int idx, int lineNum, char *lex, char *tokName, int tokInt, char *attrName, int attrInt)
{
    if (idx < 0 || idx > *tokArrSize)
    {
        printf("Invalid Index (%d): Index must be between 0 and %d\n", idx, *tokArrSize);
        return;
    }
    if (*tokArrSize >= *tokArrCap)
    {
        *tokArrCap *= 2;
        struct Token **temp = realloc(*tokArr, (*tokArrCap) * sizeof(struct Token *));
        if (temp == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        *tokArr = temp; // Update the original pointer
    }

    // Shift elements to the right to make space
    for (int i = *tokArrSize; i > idx; i--)
    {
        (*tokArr)[i] = (*tokArr)[i - 1];
    }
    // Create a new token and insert at the specified index
    (*tokArr)[idx] = create(lineNum, lex, tokName, tokInt, attrName, attrInt);
    (*tokArrSize)++;
}

// Delete a token at a specific index
void deleteN(struct Token **tokArr, int *tokArrSize, int idx)
{
    if (idx < 0 || idx >= *tokArrSize)
    {
        printf("Invalid Index (%d): Index must be between 0 and %d\n", idx, *tokArrSize - 1);
        return;
    }
    for (int i = idx; i < *tokArrSize - 1; i++)
    {
        (*tokArr)[i] = (*tokArr)[i + 1];
    }
    (*tokArrSize)--;
}

// Clear the entire token array
void clear(struct Token **tokArr, int *tokArrSize)
{
    free(*tokArr);
    *tokArr = NULL;
    *tokArrSize = 0;
}

// Search for a token by lineNum
struct Token *search(struct Token *tokArr, int tokArrSize, int lineNum)
{
    for (int i = 0; i < tokArrSize; i++)
    {
        if (tokArr[i].lineNum == lineNum)
        {
            return &tokArr[i];
        }
    }
    printf("Token with lineNum %d not found\n", lineNum);
    return NULL;
}

// Print the token array
void printList(struct Token *tokArr, int tokArrSize)
{
    for (int i = 0; i < tokArrSize; i++)
    {
        printf("%d: %s -> ", tokArr[i].lineNum, tokArr[i].lex);
    }
    printf("END\n");
}
