#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "LinkedList.h"

#define MAX_LINE_LENGTH 72
#define MAX_STRINGS 15
#define MAX_STRING_LENGTH 100

struct ResWord
{
  char resLex[10];
  char resTokStr[10];
  int resTokInt;
  char resAttrStr[10];
  int resAttrInt;
};

typedef struct
{
  char **strings; // Pointer to an array of string pointers
  int count;      // Current number of strings
} SynchSet;

// Define a structure for lexical errors
typedef struct LexErr
{
  int lineNum;
  char err[256];
  // char rec[256];
  char lex[72];
} LexErr;

// Define a structure for syntax errors
typedef struct SynErr
{
  int lineNum;
  char exp[256];
  char rec[256];
  char lex[72];
} SynErr;

// Define a structure for lexical errors
typedef struct SemErr
{
  int lineNum;
  char err[256];
  char lex[72];
} SemErr;

struct Profile
{
  char lex[100];
  char profStr[20];
  struct Profile *next;
};

struct Symbol
{
  char lexeme[100];
  int symbolType;
  struct Symbol *next;
  struct Symbol *prev;
};

// Global variables to store errors
LexErr *lexErrors = NULL; // Dynamic array of errors
SynErr *synErrors = NULL; // Dynamic array of errors
SemErr *semErrors = NULL; // Dynamic array of errors

int lexErrCount = 0;    // Number of errors
int lexErrCapacity = 0; // Current capacity of the errors array
int synErrCount = 0;    // Number of errors
int synErrCapacity = 0; // Current capacity of the errors array
char lexErrMsg[256];
int semErrCount = 0;    // Number of errors
int semErrCapacity = 0; // Current capacity of the errors array
char semErrMsg[256];

FILE *listingFile;
FILE *tokenFile;
char line[MAX_LINE_LENGTH];
char lex[MAX_LINE_LENGTH];
int f = 0;
int b = 0;
int lineNum = 0;
struct ResWord resWordArr[21];
struct Node *symTable = NULL;
struct Node *tokList = NULL;
struct Node *tok = NULL;

struct Profile *profileHead = NULL;
struct Symbol *symtop = NULL;
struct Symbol *symbottom = NULL;
struct Symbol *greenNodeStack = NULL; // green node stack
struct Symbol *greenNode = NULL;      // green node
char *profile = NULL;
char *paramList = NULL;
int offset = 0;
int typeSize = 0;
FILE *outTable;

void subprog_decs();
void decs();
int type();
void cmpd_stmt();
void stmt();
int expr();

SynchSet *createSynchSet()
{
  SynchSet *set = (SynchSet *)malloc(sizeof(SynchSet));
  set->strings = (char **)malloc(MAX_STRINGS * sizeof(char *));
  set->count = 0;
  return set;
}

// Add a string to the set
int addSynch(SynchSet *set, const char *str)
{
  if (set->count >= MAX_STRINGS)
  {
    printf("Error: SynchSet is full.\n");
    return 0;
  }
  set->strings[set->count] = (char *)malloc((strlen(str) + 1) * sizeof(char));
  strcpy(set->strings[set->count], str);
  set->count++;
  return 1;
}

// Check if a string exists in the set
int searchSynch(SynchSet *set, const char *str)
{
  for (int i = 0; i < set->count; i++)
  {
    if (strcmp(set->strings[i], str) == 0)
    {
      return 1; // Found
    }
  }
  return 0; // Not found
}

// Empty the set (but retain the structure)
void emptySynchSet(SynchSet *set)
{
  for (int i = 0; i < set->count; i++)
  {
    free(set->strings[i]); // Free each string
    set->strings[i] = NULL;
  }
  set->count = 0; // Reset the count
}

// Free the memory allocated for the set
void freeSynchSet(SynchSet *set)
{
  emptySynchSet(set); // Free individual strings first
  free(set->strings);
  free(set);
}

void readToksIn(struct Node **head)
{
  FILE *tokFile = fopen("TokenFile.txt", "r");
  // FILE *tokFile = fopen("TokenFileCorrectProgram.txt", "r");
  // FILE *tokFile = fopen("TokenFileShenoiProg.txt", "r");

  if (!tokFile)
  {
    perror("Error opening file");
    return;
  }
  while (fgets(line, MAX_LINE_LENGTH, tokFile))
  {
    int lineNum, tokInt, attrInt = 0;
    char lex[MAX_LINE_LENGTH], tokStr[10], attrStr[MAX_LINE_LENGTH] = "";
    union Attr attr;

    // Tokenize the line
    char *str = strtok(line, " ");
    if (!str)
      continue;
    lineNum = atoi(str);

    str = strtok(NULL, " ");
    if (!str)
      continue;
    strncpy(lex, str, MAX_LINE_LENGTH);

    str = strtok(NULL, " ");
    if (!str)
      continue;
    strncpy(tokStr, str, sizeof(tokStr));

    str = strtok(NULL, " ");
    if (!str)
      continue;
    tokInt = atoi(str);

    str = strtok(NULL, " ");
    if (!str)
      continue;

    if (strcmp(tokStr, "LEXERR") == 0)
    {
      while (str != NULL)
      {
        if (strcmp(str, "99\n") == 0)
        {
          break;
        }

        if (strlen(attrStr) + strlen(str) + 1 < MAX_LINE_LENGTH)
        {
          if (strlen(attrStr) > 0)
          {
            strncat(attrStr, " ", MAX_LINE_LENGTH - strlen(attrStr) - 1);
          }
          strncat(attrStr, str, MAX_LINE_LENGTH - strlen(attrStr) - 1);
        }
        str = strtok(NULL, " ");
      }
    }
    else
    {
      // printf("str: %s\n", str);
      strcpy(attrStr, str);
      str = strtok(NULL, " ");
      if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
      {
        attr.symPtr = (void *)strtoul(str, NULL, 16);
      }
      else
      {
        attr.attrInt = atoi(str);
      }
    }

    append(head, lineNum, lex, tokStr, tokInt, attrStr, attr);
  }
}

struct Node *getNextTok()
{
  if (tokList == NULL)
  {
    printf("No more tokens.\n");
    return NULL;
  }

  struct Node *returnNode = tokList;
  tokList = returnNode->next;
  // printf("NewToken: %s, Lex:%s\n", returnNode->tokStr, returnNode->lex);
  return returnNode;
}

void synerr(char *exp, char *rec)
{
  // Ensure there is enough capacity in the array
  if (synErrCount >= synErrCapacity)
  {
    synErrCapacity = (synErrCapacity == 0) ? 10 : synErrCapacity * 2; // Double capacity
    synErrors = realloc(synErrors, synErrCapacity * sizeof(SynErr));
    if (!synErrors)
    {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
  }

  // Add the new error to the array
  SynErr *newError = &synErrors[synErrCount++];
  newError->lineNum = tok->lineNum;
  strncpy(newError->exp, exp, sizeof(newError->exp) - 1);
  strncpy(newError->rec, rec, sizeof(newError->rec) - 1);
  strncpy(newError->lex, tok->lex, sizeof(newError->lex) - 1);

  // fprintf(listingFile, "SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", exp, rec, tok->lineNum, tok->lex);
  // printf("SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", exp, rec, tok->lineNum, tok->lex);
}

void lexerr(char *err, char *lex, int lineNum)
{
  // Ensure there is enough capacity in the array
  if (lexErrCount >= lexErrCapacity)
  {
    lexErrCapacity = (lexErrCapacity == 0) ? 10 : lexErrCapacity * 2; // Double capacity
    lexErrors = realloc(lexErrors, lexErrCapacity * sizeof(LexErr));
    if (!lexErrors)
    {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
  }

  // Add the new error to the array
  strcpy(lexErrMsg, "");
  strcat(lexErrMsg, err);
  strcat(lexErrMsg, lex);
  strcat(lexErrMsg, "\n");
  LexErr *newError = &lexErrors[lexErrCount++];
  strncpy(newError->err, lexErrMsg, sizeof(newError->err) - 1);
  newError->lineNum = lineNum;
  // strncpy(newError->exp, exp, sizeof(newError->exp) - 1);
  // strncpy(newError->rec, rec, sizeof(newError->rec) - 1);
  // strncpy(newError->lex, tok->lex, sizeof(newError->lex) - 1);
  // printf(newError->err);
  // fprintf(listingFile, "SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", exp, rec, tok->lineNum, tok->lex);
  // printf("SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", exp, rec, tok->lineNum, tok->lex);
}

void semerr(char *err, char *lex, int lineNum)
{
  // Ensure there is enough capacity in the array
  if (semErrCount >= semErrCapacity)
  {
    semErrCapacity = (semErrCapacity == 0) ? 10 : semErrCapacity * 2; // Double capacity
    semErrors = realloc(semErrors, semErrCapacity * sizeof(LexErr));
    if (!semErrors)
    {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
  }

  // Add the new error to the array
  strcpy(semErrMsg, "");
  strcat(semErrMsg, "SEMERR: ");
  strcat(semErrMsg, err);
  // strcat(semErrMsg, lex);
  // strcat(semErrMsg, ", Lex: ");
  // strcat(semErrMsg, lex);
  strcat(semErrMsg, "\n");
  SemErr *newError = &semErrors[semErrCount++];
  strncpy(newError->err, semErrMsg, sizeof(newError->err) - 1);
  newError->lineNum = lineNum;
}

// Function to insert syntax errors into the listing file
void insertErrs()
{
  FILE *listingFile = fopen("ListingFile.txt", "r");
  if (!listingFile)
  {
    printf("Error opening listing file\n");
    return;
  }

  // Read the original file into memory
  char **lines = NULL;
  int lineCount = 0;
  char buffer[1024];

  while (fgets(buffer, sizeof(buffer), listingFile))
  {
    lines = realloc(lines, sizeof(char *) * (lineCount + 1));
    lines[lineCount] = strdup(buffer);
    lineCount++;
  }
  fclose(listingFile);

  // Open the listing file for writing
  listingFile = fopen("ListingFile.txt", "w");

  for (int i = 0; i < lineCount; i++)
  {
    fprintf(listingFile, "%s", lines[i]);

    for (int j = 0; j < lexErrCount; j++)
    {
      if (lexErrors[j].lineNum == i + 1)
      {
        fprintf(listingFile, "%s", lexErrors[j].err);
      }
    }

    for (int j = 0; j < synErrCount; j++)
    {
      if (synErrors[j].lineNum == i + 1)
      {
        fprintf(listingFile, "SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", synErrors[j].exp, synErrors[j].rec, synErrors[j].lineNum, synErrors[j].lex);
      }
    }

    for (int j = 0; j < semErrCount; j++)
    {
      if (semErrors[j].lineNum == i + 1)
      {
        fprintf(listingFile, "%s", semErrors[j].err);
      }
    }
  }

  // Clean up memory
  for (int i = 0; i < lineCount; i++)
  {
    free(lines[i]);
  }
  free(lines);
  fclose(listingFile);
}

void pop()
{
  if (greenNodeStack != NULL)
  {
    struct Symbol *temp = NULL;
    temp = (struct Symbol *)malloc(sizeof(struct Symbol));
    temp = symbottom;
    while (temp->prev != NULL)
    {
      if (strcmp(greenNodeStack->lexeme, temp->lexeme) == 0)
      {
        struct Symbol *temp2 = NULL;
        temp2 = (struct Symbol *)malloc(sizeof(struct Symbol));
        temp2 = temp->next;
        free(temp2);
        temp->next = NULL;
        symbottom = temp;

        struct Symbol *tempStack = NULL;
        tempStack = (struct Symbol *)malloc(sizeof(struct Symbol));
        tempStack = greenNodeStack;
        greenNodeStack = greenNodeStack->next;
        free(tempStack);

        return;
      }

      temp = temp->prev;
    }

    printf("did not find matching green node in list\n");
    return;
  }
  else
  {
    printf("green node stack is empty\n");
    return;
  }
}

void checkAddGreenNode(char *currentSymbol, int currSymbolType)
{
  struct Symbol *currentNode = NULL;                            // new node
  currentNode = (struct Symbol *)malloc(sizeof(struct Symbol)); // make space in memory
  strcpy(currentNode->lexeme, currentSymbol);
  currentNode->symbolType = currSymbolType;
  currentNode->next = NULL;
  symbottom = symtop;

  if (symtop == NULL)
  {
    currentNode->prev = NULL;
    symtop = currentNode;
    greenNode = currentNode;
    symtop = currentNode;

    struct Symbol *stackNode = (struct Symbol *)malloc(sizeof(struct Symbol));
    strcpy(stackNode->lexeme, currentSymbol);
    stackNode->symbolType = currSymbolType;
    stackNode->next = greenNodeStack;
    greenNodeStack = stackNode;
    return;
  }
  else
  {
    while (symbottom->next != NULL)
    {
      if (strcmp(symbottom->lexeme, currentSymbol) == 0)
      {
        semerr("attempted to redeclare ID with same procedure or program name", tok->lex, tok->lineNum);
        return;
      }

      symbottom = symbottom->next;
    }

    symbottom->next = currentNode;
    currentNode->prev = symbottom;
    greenNode = currentNode;

    struct Symbol *stackNode = (struct Symbol *)malloc(sizeof(struct Symbol));
    strcpy(stackNode->lexeme, currentSymbol);
    stackNode->symbolType = currSymbolType;
    stackNode->next = greenNodeStack;
    greenNodeStack = stackNode;
  }
}

void checkAddBlueNode(char *currentSymbol, int currSymbolType)
{
  struct Symbol *currentNode = NULL;
  currentNode = (struct Symbol *)malloc(sizeof(struct Symbol));
  strcpy(currentNode->lexeme, currentSymbol);
  currentNode->symbolType = currSymbolType;
  currentNode->next = NULL;
  symbottom = greenNode;

  while (symbottom->next != NULL)
  {
    if (strcmp(symbottom->lexeme, currentSymbol) == 0)
    {
      semerr("attempted to redeclare ID with same procedure or program name", tok->lex, tok->lineNum);
      return;
    }
    symbottom = symbottom->next;
  }

  if (strcmp(symbottom->lexeme, currentSymbol) == 0)
  {
    semerr("attempted to redeclare ID with same procedure or program name", tok->lex, tok->lineNum);
    return;
  }
  symbottom->next = currentNode;
  currentNode->prev = symbottom;
}

void getProfile(char *lexeme)
{
  struct Profile *temp = profileHead;
  while (temp->next != NULL)
  {
    if (strcmp(temp->lex, lexeme) == 0)
    {
      struct Symbol *temp2 = symtop;

      while (temp2->next != NULL)
      {

        if (strcmp(temp2->lexeme, lexeme) == 0)
        {
          strcpy(profile, temp->profStr);
          return;
        }

        temp2 = temp2->next;
      }
      if (strcmp(temp2->lexeme, lexeme) == 0)
      {
        strcpy(profile, temp->profStr);
        return;
      }
    }

    temp = temp->next;
  }

  if (strcmp(temp->lex, lexeme) == 0)
  {
    struct Symbol *temp2 = symtop;

    while (temp2->next != NULL)
    {
      if (strcmp(temp2->lexeme, lexeme) == 0)
      {
        strcpy(profile, temp->profStr);
        return;
      }

      temp2 = temp2->next;
    }
    if (strcmp(temp2->lexeme, lexeme) == 0)
    {
      strcpy(profile, temp->profStr);
      return;
    }
  }

  strcpy(profile, "999");
  return;
}

int getType(char *currentLexeme)
{
  // printf("here\n");
  struct Symbol *temp = symbottom; // start from end of list
  int id_t;

  while (temp != NULL)
  {
    if (strcmp(temp->lexeme, currentLexeme) == 0)
    {
      switch (temp->symbolType)
      {
      case 1:
        id_t = 1;
        break;
      case 2:
        id_t = 2;
        break;
      case 3:
        id_t = 3;
        break;
      case 4:
        id_t = 4;
        break;
      case 5:
        id_t = 1;
        break;
      case 6:
        id_t = 2;
        break;
      case 7:
        id_t = 3;
        break;
      case 8:
        id_t = 4;
        break;
      default:
        printf("id type not INT, REAL, AINT, AREAL");
        break;
      }
      return id_t;
    }
    temp = temp->prev;
  }
  semerr("undeclared variable", tok->lex, tok->lineNum);
  return 0;
}

void printSymbolTable(struct Symbol *head)
{
  int count = 0;
  struct Symbol *currNode = head;
  while (currNode != NULL)
  {
    printf("%-15s %-3d\n", currNode->lexeme, currNode->symbolType);
    // fprintf(outList, "%-15s %-3d\n", currNode->lexeme, currNode->symbolType); // print id lexeme, id type
    currNode = currNode->next;
    count++;
  }
}

void printTok(int lineNo, char *lexeme, char *tokName, int tokInt, char *attrName, int attrInt)
{
  fprintf(tokenFile, "%d %s %s %d %s %d\n", lineNo, lexeme, tokName, tokInt, attrName, attrInt);
}

void printTokUnion(int lineNo, char *lexeme, char *tokName, int tokInt, char *attrName, union Attr attr)
{
  fprintf(tokenFile, "%d %s %s %d %s %p\n", lineNo, lexeme, tokName, tokInt, attrName, attr.symPtr);
}

int checkResWord(char *lexeme)
{
  // printf("Lex: %s\n", lexeme);
  for (int i = 0; i < 21; i++)
  {
    // printf("  Compare %s to %s\n", lexeme, resWordArr[i].resLex);
    if (strcmp(lexeme, resWordArr[i].resLex) == 0)
    {
      // printf("HERE\n");
      printTok(lineNum, resWordArr[i].resLex, resWordArr[i].resTokStr, resWordArr[i].resTokInt, resWordArr[i].resAttrStr, resWordArr[i].resAttrInt);
      return 0;
    }
  }
  return 1;
}

int whitespace(char *str)
{
  // printf("whitespace");
  if (str[f] == '\n' || str[f] == '\t' || str[f] == ' ')
  {
    f++;
    return 0;
  }
  else
    return 1;
}

int idRes(char *str)
{
  char tmp[2];
  tmp[1] = '\0';
  memset(lex, 0, sizeof(lex));

  // Check if the first character is a letter (start of a valid word)
  if (isalpha(str[f]))
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;

    // Continue appending alphanumeric characters to form a word
    while (isalnum(str[f]))
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
    }

    if (strlen(lex) > 10)
    {
      // fprintf(listingFile, "LEXERR: Extra Long ID: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long ID", 99);
      // strcat("LEXERR: Extra Long ID: ", lex);
      // strcpy(lexErrMsg, "LEXERR: Extra Long ID: ", lex, "\n");
      lexerr("LEXERR: Extra Long ID: ", lex, lineNum);
      return 0;
    }

    // printf("IDRES: %s\n", lex);
    if (checkResWord(lex) == 1)
    {
      union Attr attr;
      char *symbol = malloc(strlen(lex) + 1);
      strncpy(symbol, lex, strlen(lex));
      symbol[strlen(lex)] = '\0';
      attr.symPtr = (void *)symbol;

      if (isInList(&symTable, lex) == 1)
      {
        append(&symTable, lineNum, lex, "ID", 53, "NIL", attr);
        printTokUnion(lineNum, lex, "ID", 53, "NIL", attr);
      }
      else
      {
        struct Node *symNode = search(&symTable, symbol);
        // printf("Node found: %s\n", symNode->lex);
        printTokUnion(lineNum, lex, "ID", 53, "NIL", symNode->attr);
        free(symbol);
      }
    }
    return 0;
  }
  else
  {
    return 1;
  }
}

int catchall(char *str)
{

  if (str[f] == ':' && str[f + 1] == '=')
  {
    f += 2;
    // printf("CATCHALL: :=\n");
    printTok(lineNum, ":=", "ASSIGN", 24, "NIL", 44);
    return 0;
  }

  if (str[f] == '.' && str[f + 1] == '.')
  {
    f += 2;
    // printf("CATCHALL: ..\n");
    printTok(lineNum, "..", "DOTDOT", 52, "NIL", 44);
    return 0;
  }

  if (str[f] == '+')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "+", "ADDOP", 55, "PLUS", 35);
    f++;
    return 0;
  }
  if (str[f] == '-')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "-", "ADDOP", 55, "MINUS", 36);
    f++;
    return 0;
  }
  if (str[f] == '*')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "*", "MULOP", 56, "STAR", 38);
    f++;
    return 0;
  }
  if (str[f] == '/')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "/", "MULOP", 56, "SLASH", 39);
    f++;
    return 0;
  }
  if (str[f] == ';')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, ";", "SEMI", 45, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == '.')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, ".", "DOT", 46, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == ',')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, ",", "COMMA", 47, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == '[')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "[", "LBRACK", 48, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == ']')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "]", "RBRACK", 49, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == '(')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, "(", "LPAR", 50, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == ')')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, ")", "RPAR", 51, "NIL", 44);
    f++;
    return 0;
  }
  if (str[f] == ':')
  {
    // printf("CATCHALL: %c\n", str[f]);
    printTok(lineNum, ":", "COLON", 57, "NIL", 44);
    f++;
    return 0;
  }

  return 1;
}

int longReal(char *str)
{
  char tmp[2];
  tmp[1] = '\0';

  // Declare lex1, lex2, and lex3 locally within the function
  char lex1[MAX_LINE_LENGTH] = {0}; // Store digits before the decimal
  char lex2[MAX_LINE_LENGTH] = {0}; // Store digits after the decimal before 'E'
  char lex3[MAX_LINE_LENGTH] = {0}; // Store digits after 'E' (exponent part)

  memset(lex, 0, sizeof(lex));
  // printf("Try LongReal: %c\n", str[f]);
  // Step 1: Read digits before the decimal point (store in lex1)
  if (isdigit(str[f]))
  {
    // printf("\t%c is a digit\n", str[f]);
    while (isdigit(str[f]))
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      strcat(lex1, tmp);
      f++;
    }

    // Step 2: Check for the decimal point and read digits after it (store in lex2)
    if (str[f] == '.')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;

      if (isdigit(str[f]))
      {
        // Step 3: Read digits after the decimal point
        while (isdigit(str[f]))
        {
          tmp[0] = str[f];
          strcat(lex, tmp);
          strcat(lex2, tmp);
          f++;
        }

        // Step 4: Look for 'E' to handle the exponent part (store in lex3)
        if (str[f] == 'E')
        {
          tmp[0] = str[f];
          strcat(lex, tmp);
          f++;

          // Optional: Check for '+' or '-' sign in the exponent
          if (str[f] == '+' || str[f] == '-')
          {
            tmp[0] = str[f];
            strcat(lex, tmp);
            f++;
          }

          // Step 5: Read digits after 'E' (exponent part, store in lex3)
          if (isdigit(str[f]))
          {
            while (isdigit(str[f]))
            {
              tmp[0] = str[f];
              strcat(lex, tmp);
              strcat(lex3, tmp);
              f++;
            }
          }
          else
          {
            f = b;
            return 1;
          }
        }
        else
        {
          f = b;
          return 1;
        }
      }
      else
      {
        f = b;
        return 1;
      }
    }
    else
    {
      f = b;
      return 1;
    }
    // printf("here");
    // printf("LONGREAL: %s\n", lex);

    // Whole part
    if (strlen(lex1) > 5)
    {
      // fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Whole Part", 99);
      lexerr("LEXERR: Extra Long Whole Part: ", lex, lineNum);
      return 0;
    }
    if (strlen(lex1) > 0 && lex1[0] == '0')
    {
      // fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Leading 0", 99);
      lexerr("LEXERR: Leading 0 in Whole Part: ", lex, lineNum);
      return 0;
    }

    // Frational Part
    if (strlen(lex2) > 5)
    {
      // fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Fractional Part", 99);
      lexerr("LEXERR: Extra Long Fractional Part: ", lex, lineNum);
      return 0;
    }
    if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
    {
      // fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Trailing 0 in Fractional Part", 99);
      lexerr("LEXERR: Trailing 0 in Fractional Part: ", lex, lineNum);
      return 0;
    }

    // Exponential Part
    if (strlen(lex3) > 2)
    {
      // fprintf(listingFile, "LEXERR: Extra Long Exponential Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Exponent", 99);
      lexerr("LEXERR: Extra Long Exponential Part: ", lex, lineNum);
      return 0;
    }
    if (strlen(lex3) > 0 && lex3[0] == '0')
    {
      // fprintf(listingFile, "LEXERR: Leading 0 in Exponential Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Leading 0 in Exponent", 99);
      lexerr("LEXERR: Leading 0 in Exponential Part: ", lex, lineNum);
      return 0;
    }
    // printf("Before decimal (lex1): %s\n", lex1);
    // printf("After decimal (lex2): %s\n", lex2);
    // printf("Exponent part (lex3): %s\n", lex3);

    printTok(lineNum, lex, "NUM", 16, "REAL", 2);
    return 0;
  }
  return 1;
}

int real(char *str)
{
  char tmp[2];
  tmp[1] = '\0';

  memset(lex, 0, sizeof(lex));

  char lex1[MAX_LINE_LENGTH] = {0};
  char lex2[MAX_LINE_LENGTH] = {0};

  if (isdigit(str[f]))
  {
    while (isdigit(str[f]))
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      strcat(lex1, tmp);
      f++;
    }

    if (str[f] == '.')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;

      if (isdigit(str[f]))
      {
        // Step 3: Read digits after the decimal point
        while (isdigit(str[f]))
        {
          tmp[0] = str[f];
          strcat(lex, tmp);
          strcat(lex2, tmp);
          f++;
        }

        if (strlen(lex1) > 5)
        {
          // fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Extra Long Whole Part", 99);
          lexerr("LEXERR: Extra Long Whole Part: ", lex, lineNum);
          return 0;
        }
        if (strlen(lex1) > 0 && lex1[0] == '0')
        {
          // fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Leading 0 in Whole Part", 99);
          lexerr("LEXERR: Leading 0 in Whole Part: ", lex, lineNum);
          return 0;
        }
        if (strlen(lex2) > 5)
        {
          // fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Extra Long Fractional Part", 99);
          lexerr("LEXERR: Extra Long Fractional Part: ", lex, lineNum);
          return 0;
        }
        if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
        {
          // fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Trailing 0 in Fractional Part", 99);
          lexerr("LEXERR: Trailing 0 in Fractional Part: ", lex, lineNum);
          return 0;
        }
        printTok(lineNum, lex, "NUM", 16, "REAL", 2);
      }
      else
      {
        f = b;
        return 1;
      }
    }
    else
    {
      f = b;
      return 1;
    }

    return 0;
  }
  else
    return 1;
}

int intMach(char *str)
{
  char tmp[2];
  tmp[1] = '\0';

  memset(lex, 0, sizeof(lex));
  // printf("Try intMach: %c\n", str[f]);
  if (isdigit(str[f]))
  {
    // printf("got here");
    while (isdigit(str[f]) || str[f] == '0')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
    }

    if (strlen(lex) > 10)
    {
      // fprintf(listingFile, "LEXERR: Extra Long Integer: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Int", 99);
      lexerr("LEXERR: Extra Long Integer: ", lex, lineNum);
      return 0;
    }
    if (strlen(lex) > 1 && lex[0] == '0')
    {
      // fprintf(listingFile, "LEXERR: Leading 0 on Integer: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Leading 0", 99);
      lexerr("LEXERR: Leading 0 on Integer: ", lex, lineNum);
      return 0;
    }

    printTok(lineNum, lex, "NUM", 16, "INT", 1);
    return 0;
  }
  else
    return 1;
}

int relop(char *str)
{
  char tmp[2];
  tmp[1] = '\0';

  memset(lex, 0, sizeof(lex));

  if (str[f] == '<')
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;
    if (str[f] == '=')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
      // printf("RELOP: %s\n", lex);
      printTok(lineNum, lex, "RELOP", 54, "LTEQ", 32);
      return 0;
    }
    else if (str[f] == '>')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
      // printf("RELOP: %s\n", lex);
      printTok(lineNum, lex, "RELOP", 54, "NEQ", 30);
      return 0;
    }
    else
    {
      // printf("RELOP: %s\n", lex);
      printTok(lineNum, lex, "RELOP", 54, "LT", 31);
      return 0;
    }
  }
  else if (str[f] == '=')
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;
    // printf("RELOP: %s\n", lex);
    printTok(lineNum, lex, "RELOP", 54, "EQ", 29);
    return 0;
  }
  else if (str[f] == '>')
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;
    if (str[f] == '=')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
      // printf("RELOP: %s\n", lex);
      printTok(lineNum, lex, "RELOP", 54, "GTEQ", 33);
      return 0;
    }
    else
    {
      // printf("RELOP: %s\n", lex);
      printTok(lineNum, lex, "RELOP", 54, "GT", 34);
      return 0;
    }
  }
  else
  {
    return 1;
  }
}

int checkerr(char *str)
{
  char tmp[2];
  tmp[1] = '\0';
  memset(lex, 0, sizeof(lex));

  if (ispunct(str[f]))
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;
    // fprintf(listingFile, "LEXERR: Unrecognized Symbol: %s\n", lex);
    printTok(lineNum, lex, "LEXERR", 99, "Unrecognized Symbol", 99);
    lexerr("LEXERR: Unrecognized Symbol: ", lex, lineNum);
    return 0;
  }
  else
    return 1;
}

void match(char *t)
{
  if (tok == NULL)
  {
    printf("Error: Attempted to match but no token is available.\n");
    return;
  }
  if (strcmp(tok->tokStr, t) == 0 && strcmp(t, "EOF") != 0)
  {
    // printf("  Matched %s\n", t);
    tok = getNextTok();
  }
  else if (strcmp(tok->tokStr, t) == 0 && strcmp(t, "EOF") == 0)
  {
    printf("END OF PARSE\n");
  }
  else
  {
    // printf("SYNERR: Expected %s, Received %s\n", t, tok->lex);
    synerr(t, tok->tokStr);
    tok = getNextTok();
  }
}

int checkFactorPrime(int factorPrime_i, int expr_t)
{
  int factorPrime_t;

  if (factorPrime_i == 99)
  {
    factorPrime_t = 99;
    return factorPrime_t;
  }
  if (expr_t == 99 || expr_t == 99)
  {
    factorPrime_t = 99;
    return factorPrime_t;
  }
  else if (expr_t == 1)
  {
    if (factorPrime_i == 3)
    {
      factorPrime_t = 1;
    }
    else if (factorPrime_i == 4)
    {
      factorPrime_t = 2;
    }
    else
    {
      factorPrime_t = 99;
      semerr("factor tail type is not an array type", tok->lex, tok->lineNum);
    }
    return factorPrime_t;
  }
  else
  {
    factorPrime_t = 99;
    semerr("expression type is not int", tok->lex, tok->lineNum);
    return factorPrime_t;
  }
}

int checkFactorPrimeI(int id_t)
{
  int factorPrime_i;

  if (id_t == 99)
  {
    factorPrime_i = 99;
  }
  else if (id_t == 1)
  {
    factorPrime_i = 1;
  }
  else if (id_t == 2)
  {
    factorPrime_i = 2;
  }
  else if (id_t == 3)
  {
    factorPrime_i = 3;
  }
  else if (id_t == 4)
  {
    factorPrime_i = 4;
  }
  else
  {
    factorPrime_i = 99;
    semerr("variable is undeclared or its type was not int, real, aint, or areal", tok->lex, tok->lineNum);
  }

  return factorPrime_i;
}

int checkNumAttr(int num_attr)
{
  int factor_t;
  if (num_attr == 99)
  {
    factor_t = 99;
  }
  else if (num_attr == 1)
  {
    factor_t = 1;
  }
  else if (num_attr == 2)
  {
    factor_t = 2;
  }
  else
  {
    factor_t = 99;
    semerr("factor type was not int or real", tok->lex, tok->lineNum);
  }
  return factor_t;
}

int checkFactor1T(int factor1_t)
{
  int factor_t;

  if (factor1_t == 99)
  {
    factor_t = 99;
  }
  else if (factor1_t == 9)
  {
    factor_t = 9;
  }
  else
  {
    factor_t = 99;
    semerr("factor type is not bool", tok->lex, tok->lineNum);
  }
  return factor_t;
}

int checkMulopAttr(int mulop_attr, int factor_t, int termPrime_i)
{
  int termPrime1_i;

  if (termPrime_i == 99 || factor_t == 99)
  {
    termPrime1_i = 99;
    return termPrime1_i;
  }
  if (mulop_attr == 99)
  {
    termPrime1_i = 99;
    return termPrime1_i;
  }
  else if (mulop_attr == 38)
  {
    if (factor_t == 1 && termPrime_i == 1)
    {
      termPrime1_i = 1;
    }
    else if (factor_t == 2 && termPrime_i == 2)
    {
      termPrime1_i = 2;
    }
    else
    {
      termPrime1_i = 99;
      semerr("expected factor type and term prime i to both be int or real", tok->lex, tok->lineNum);
    }
    return termPrime1_i;
  }
  else if (mulop_attr == 39)
  {
    if (factor_t == 2 && termPrime_i == 2)
    {
      termPrime1_i = 2;
    }
    else
    {
      termPrime1_i = 99;
      semerr("expected factor type and term prime i to both be real", tok->lex, tok->lineNum);
    }
    return termPrime1_i;
  }
  else if (mulop_attr == 40)
  {
    if (factor_t == 1 && termPrime_i == 1)
    {
      termPrime1_i = 1;
    }
    else
    {
      termPrime1_i = 99;
      semerr("expected factor type and term prime i to both be int", tok->lex, tok->lineNum);
    }
    return termPrime1_i;
  }
  else if (mulop_attr == 42)
  {
    if (factor_t == 9 && termPrime_i == 9)
    {
      termPrime1_i = 9;
    }
    else
    {
      termPrime1_i = 99;
      semerr("expected factor type and term prime i to both be bool", tok->lex, tok->lineNum);
    }
    return termPrime1_i;
  }
  else if (mulop_attr == 41)
  {
    if (factor_t == 1 && termPrime_i == 1)
    {
      termPrime1_i = 1;
    }
    else
    {
      termPrime1_i = 99;
      semerr("expected factor type and term prime i to both be int", tok->lex, tok->lineNum);
    }
    return termPrime1_i;
  }
  else
  {
    printf("Big error in checkMulopAttr()");
    return 0;
  }
}

int checkAddopAttr(int addop_attr, int simple_exprPrime_i, int term_t)
{
  int simple_exprPrime1_i;

  if (term_t == 99 || simple_exprPrime_i == 99)
  {
    simple_exprPrime1_i = 99;
    return simple_exprPrime1_i;
  }
  if (addop_attr == 99)
  {
    simple_exprPrime1_i = 99;
    return simple_exprPrime1_i;
  }
  else
  {
    if (addop_attr == 35)
    {
      if (term_t == 1 && simple_exprPrime_i == 1)
      {
        simple_exprPrime1_i = 1;
      }
      else if (term_t == 2 && simple_exprPrime_i == 2)
      {
        simple_exprPrime1_i = 2;
      }
      else
      {
        simple_exprPrime1_i = 99;
        semerr("expected term t and simple_exprPrime i to both be int or both real", tok->lex, tok->lineNum);
      }
      return simple_exprPrime1_i;
    }
    else if (addop_attr == 36)
    {
      if (term_t == 1 && simple_exprPrime_i == 1)
      {
        simple_exprPrime1_i = 1;
      }
      else if (term_t == 2 && simple_exprPrime_i == 2)
      {
        simple_exprPrime1_i = 2;
      }
      else
      {
        simple_exprPrime1_i = 99;
        semerr("expected term t and simple_exprPrime i to both be int or both real", tok->lex, tok->lineNum);
      }
      return simple_exprPrime1_i;
    }
    else if (addop_attr == 37)
    {
      if (term_t == 9 && simple_exprPrime_i == 9)
      {
        simple_exprPrime1_i = 9;
      }
      else
      {
        simple_exprPrime1_i = 99;
        semerr("expected term t and simple_exprPrime i to both be bool", tok->lex, tok->lineNum);
      }
      return simple_exprPrime1_i;
    }
    else
    {
      printf("Big error in checkAddopAttr()");
      return 0;
    }
  }
}

int checkTermT(int term_t)
{
  int simple_exprPrime_i;

  if (term_t == 99)
  {
    simple_exprPrime_i = 99;
  }
  else if (term_t == 1)
  {
    simple_exprPrime_i = 1;
  }
  else if (term_t == 2)
  {
    simple_exprPrime_i = 2;
  }
  else
  {
    simple_exprPrime_i = 99;
    semerr("expected term t int or real", tok->lex, tok->lineNum);
  }
  return simple_exprPrime_i;
}

void checkProc(char *lexeme)
{
  struct Symbol *temp2 = symtop;
  while (temp2->next != NULL)
  {
    if (strcmp(temp2->lexeme, lexeme) == 0)
    {
      return;
    }
    temp2 = temp2->next;
  }
  if (strcmp(temp2->lexeme, lexeme) == 0)
  {
    return;
  }
  semerr("undeclared procedure", tok->lex, tok->lineNum);
  return;
}

int checkRelopAttr(int simple_expr_t, int exprPrime_i)
{
  int exprPrime_t;

  if (exprPrime_i == 99 || simple_expr_t == 99)
  {
    exprPrime_t = 99;
    return exprPrime_t;
  }
  if (exprPrime_i == 1)
  {
    if (simple_expr_t == 1)
    {
      exprPrime_t = 9;
    }
    else
    {
      exprPrime_t = 99;
      semerr("expected exprPrime i and simple_expr t to both be int", tok->lex, tok->lineNum);
    }
  }
  else if (exprPrime_i == 2)
  {
    if (simple_expr_t == 2)
    {
      exprPrime_t = 9;
    }
    else
    {
      exprPrime_t = 99;
      semerr("expected exprPrime i and simple_expr t to both be real", tok->lex, tok->lineNum);
    }
  }
  else
  {
    exprPrime_t = 99;
    semerr("expected exprPrime i and simple_expr t to both be int or both real", tok->lex, tok->lineNum);
  }
  return exprPrime_t;
}

int checkVarPrime(int expr_t, int varPrime_i)
{
  int varPrime_t;

  if (expr_t == 99 || varPrime_i == 99)
  {
    varPrime_t = 99;
    return varPrime_t;
  }
  if (varPrime_i == 3)
  {
    if (expr_t == 1)
    {
      varPrime_t = 1;
    }
    else
    {
      varPrime_t = 99;
      semerr("expected expr type int", tok->lex, tok->lineNum);
    }
  }
  else if (varPrime_i == 4)
  {
    if (expr_t == 1)
    {
      varPrime_t = 2;
    }
    else
    {
      varPrime_t = 99;
      semerr("expected expr type int", tok->lex, tok->lineNum);
    }
  }
  else
  {
    varPrime_t = 99;
    semerr("expected varPrime i aint or areal1", tok->lex, tok->lineNum);
  }
  return varPrime_t;
}

void checkVarT(int var_t, int expr_t)
{
  if (expr_t == 1)
  {
    if (var_t == 2)
    {
      semerr("expected var type int and expr type int", tok->lex, tok->lineNum);
    }
  }
  else if (expr_t == 2)
  {
    if (var_t == 1)
    {
      semerr("expected var type real and expr type real", tok->lex, tok->lineNum);
    }
  }
  else if (expr_t == 3 || expr_t == 4)
  {
    semerr("expected var type and expr type to both be int or both real", tok->lex, tok->lineNum);
  }
}

void checkExprT(int expr_t)
{
  if (expr_t != 9 && expr_t != 99)
  {
    semerr("expected expr type bool", tok->lex, tok->lineNum);
  }
}

int checkStdTypeT(int std_type_t)
{
  int type_t;

  if (std_type_t == 99)
  {
    type_t = 99;
  }
  else if (std_type_t == 1)
  {
    type_t = 3;
  }
  else if (std_type_t == 2)
  {
    type_t = 4;
  }
  else
  {
    printf("Big error in checkStdTypeT()");
    exit(0);
  }
  return type_t;
}

int sign()
{
  int sign_attr;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ID");
  addSynch(set, "NUM");
  addSynch(set, "LPAR");
  addSynch(set, "NOT");

  if (strcmp(tok->tokStr, "PLUS") == 0)
  {
    match("PLUS");
    sign_attr = tok->attr.attrInt;
    freeSynchSet(set);
    return sign_attr;
  }
  else if (strcmp(tok->tokStr, "MINUS") == 0)
  {
    match("MINUS");
    sign_attr = tok->attr.attrInt;
    freeSynchSet(set);
    return sign_attr;
  }
  else
  {
    synerr("PLUS or MINUS", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int factorPrime(int factorPrime_i)
{
  int factorPrime_t;
  int expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "MULOP");
  addSynch(set, "ADDOP");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "LBRACK") == 0)
  {
    match("LBRACK");
    expr_t = expr();
    factorPrime_t = checkFactorPrime(factorPrime_i, expr_t);
    match("RBRACK");
    freeSynchSet(set);
    return factorPrime_t;
  }
  else if (strcmp(tok->tokStr, "MULOP") == 0 ||
           strcmp(tok->tokStr, "ADDOP") == 0 ||
           strcmp(tok->tokStr, "RELOP") == 0 ||
           strcmp(tok->tokStr, "THEN") == 0 ||
           strcmp(tok->tokStr, "DO") == 0 ||
           strcmp(tok->tokStr, "COMMA") == 0 ||
           strcmp(tok->tokStr, "RPAR") == 0 ||
           strcmp(tok->tokStr, "RBRACK") == 0 ||
           strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
    factorPrime_t = factorPrime_i;
    freeSynchSet(set);
    return factorPrime_t;
  }
  else
  {
    synerr("LBRACK, MULOP, ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int factor()
{
  int factor_t;
  int id_t;
  int factorPrime_i;
  int factorPrime_t;
  int num_attr;
  int expr_t;
  int factor1_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "MULOP");
  addSynch(set, "ADDOP");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    id_t = getType(tok->lex);
    match("ID");
    factorPrime_i = checkFactorPrimeI(id_t);
    factorPrime_t = factorPrime(factorPrime_i);
    factor_t = factorPrime_t;
    freeSynchSet(set);
    return factor_t;
  }
  else if (strcmp(tok->tokStr, "NUM") == 0)
  {
    num_attr = tok->attr.attrInt;
    match("NUM");
    factor_t = checkNumAttr(num_attr);
    freeSynchSet(set);
    return factor_t;
  }
  else if (strcmp(tok->tokStr, "LPAR") == 0)
  {
    match("LPAR");
    expr_t = expr();
    match("RPAR");
    factor_t = expr_t;
    freeSynchSet(set);
    return factor_t;
  }
  else if (strcmp(tok->tokStr, "NOT") == 0)
  {
    match("NOT");
    factor1_t = factor();
    factor_t = checkFactor1T(factor1_t);
    freeSynchSet(set);
    return factor_t;
  }
  else
  {
    synerr("ID, NUM, LPAR, or NOT", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int termPrime(int termPrime_i)
{
  int termPrime_t;
  int mulop_attr;
  int factor_t;
  int termPrime1_i;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ADDOP");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "MULOP") == 0)
  {
    mulop_attr = tok->attr.attrInt;
    match("MULOP");
    factor_t = factor();
    termPrime1_i = checkMulopAttr(mulop_attr, factor_t, termPrime_i);
    termPrime_t = termPrime(termPrime1_i);
    freeSynchSet(set);
    return termPrime_t;
  }
  else if (strcmp(tok->tokStr, "ADDOP") == 0 ||
           strcmp(tok->tokStr, "RELOP") == 0 ||
           strcmp(tok->tokStr, "THEN") == 0 ||
           strcmp(tok->tokStr, "DO") == 0 ||
           strcmp(tok->tokStr, "COMMA") == 0 ||
           strcmp(tok->tokStr, "RPAR") == 0 ||
           strcmp(tok->tokStr, "RBRACK") == 0 ||
           strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
    termPrime_t = termPrime_i;
    freeSynchSet(set);
    return termPrime_t;
  }
  else
  {
    synerr("MULOP, ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int term()
{
  int factor_t;
  int termPrime_i;
  int termPrime_t;
  int term_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ADDOP");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "NUM") == 0 ||
      strcmp(tok->tokStr, "LPAR") == 0 ||
      strcmp(tok->tokStr, "NOT") == 0)
  {
    factor_t = factor();
    termPrime_i = factor_t;
    termPrime_t = termPrime(termPrime_i);
    term_t = termPrime_t;
    freeSynchSet(set);
    return term_t;
  }
  else
  {
    synerr("ID, NUM, LPAR, NOT", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int simple_exprPrime(int simple_exprPrime_i)
{
  int simple_exprPrime_t;
  int addop_attr;
  int term_t;
  int simple_exprPrime1_i;
  int simple_exprPrime1_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ADDOP") == 0)
  {
    addop_attr = tok->attr.attrInt;
    match("ADDOP");
    term_t = term();
    simple_exprPrime1_i = checkAddopAttr(addop_attr, simple_exprPrime_i, term_t);
    simple_exprPrime1_t = simple_exprPrime(simple_exprPrime1_i);
    simple_exprPrime_t = simple_exprPrime1_t;
    freeSynchSet(set);
    return simple_exprPrime_t;
  }
  else if (strcmp(tok->tokStr, "RELOP") == 0 ||
           strcmp(tok->tokStr, "THEN") == 0 ||
           strcmp(tok->tokStr, "DO") == 0 ||
           strcmp(tok->tokStr, "COMMA") == 0 ||
           strcmp(tok->tokStr, "RPAR") == 0 ||
           strcmp(tok->tokStr, "RBRACK") == 0 ||
           strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
    simple_exprPrime_t = simple_exprPrime_i;
    freeSynchSet(set);
    return simple_exprPrime_t;
  }
  else
  {
    synerr("ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int simple_expr()
{
  int term_t;
  int simple_exprPrime_i;
  int simple_exprPrime_t;
  int simple_expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RELOP");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "NUM") == 0 ||
      strcmp(tok->tokStr, "LPAR") == 0 ||
      strcmp(tok->tokStr, "NOT") == 0)
  {
    term_t = term();
    simple_exprPrime_i = term_t;
    simple_exprPrime_t = simple_exprPrime(simple_exprPrime_i);
    simple_expr_t = simple_exprPrime_t;
    freeSynchSet(set);
    return simple_expr_t;
  }
  else if (strcmp(tok->tokStr, "PLUS") == 0 ||
           strcmp(tok->tokStr, "MINUS") == 0)
  {
    sign();
    term_t = term();
    simple_exprPrime_i = checkTermT(term_t);
    simple_exprPrime_t = simple_exprPrime(simple_exprPrime_i);
    simple_expr_t = simple_exprPrime_t;
    freeSynchSet(set);
    return simple_expr_t;
  }
  else
  {
    synerr("ID, NUM, LPAR, NOT, PLUS, MINUS", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int exprPrime(int exprPrime_i)
{
  int exprPrime_t;
  int relop_attr;
  int simple_expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "RELOP") == 0)
  {
    relop_attr = tok->attr.attrInt;
    match("RELOP");
    simple_expr_t = simple_expr();
    exprPrime_t = checkRelopAttr(simple_expr_t, exprPrime_i);
    freeSynchSet(set);
    return exprPrime_t;
  }
  else if (strcmp(tok->tokStr, "THEN") == 0 ||
           strcmp(tok->tokStr, "DO") == 0 ||
           strcmp(tok->tokStr, "COMMA") == 0 ||
           strcmp(tok->tokStr, "RPAR") == 0 ||
           strcmp(tok->tokStr, "RBRACK") == 0 ||
           strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
    exprPrime_t = exprPrime_i;
    freeSynchSet(set);
    return exprPrime_t;
  }
  else
  {
    synerr("RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int expr()
{
  int simple_expr_t;
  int exprPrime_i;
  int exprPrime_t;
  int expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "THEN");
  addSynch(set, "DO");
  addSynch(set, "COMMA");
  addSynch(set, "RPAR");
  addSynch(set, "RBRACK");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "PLUS") == 0 ||
      strcmp(tok->tokStr, "MINUS") == 0 ||
      strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "NUM") == 0 ||
      strcmp(tok->tokStr, "LPAR") == 0 ||
      strcmp(tok->tokStr, "NOT") == 0)
  {
    simple_expr_t = simple_expr();
    exprPrime_i = simple_expr_t;
    exprPrime_t = exprPrime(exprPrime_i);
    expr_t = exprPrime_t;
    freeSynchSet(set);
    return expr_t;
  }
  else
  {
    synerr("PLUS, MINUS, ID, NUM, LPAR, or NOT", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

void expr_listPrime()
{
  int expr_t;
  char exprStr[10] = "";

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "COMMA") == 0)
  {
    match("COMMA");
    expr_t = expr();
    sprintf(exprStr, "%d", expr_t);
    strcat(paramList, exprStr);
    expr_listPrime();
  }
  else if (strcmp(tok->tokStr, "RPAR") == 0)
  {
  }
  else
  {
    synerr("COMMA or RPAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void expr_list()
{
  int expr_t;
  char exprStr[10] = "";

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "PLUS") == 0 ||
      strcmp(tok->tokStr, "MINUS") == 0 ||
      strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "NUM") == 0 ||
      strcmp(tok->tokStr, "LPAR") == 0 ||
      strcmp(tok->tokStr, "NOT") == 0)
  {
    expr_t = expr();
    sprintf(exprStr, "%d", expr_t);
    strcat(paramList, exprStr);
    expr_listPrime();
  }
  else
  {
    synerr("PLUS, MINUS, ID, NUM, LPAR, or NOT", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void proc_stmtPrime()
{

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "LPAR") == 0)
  {
    match("LPAR");
    expr_list();
    match("RPAR");

    if (strcmp(profile, "999") == 0)
    {
      semerr("undeclared procedure parameters", tok->lex, tok->lineNum);
    }
    else if (strlen(paramList) > strlen(profile))
    {
      semerr("too many procedure parameters", tok->lex, tok->lineNum);
    }
    else if (strlen(paramList) < strlen(profile))
    {
      semerr("too few procedure parameters", tok->lex, tok->lineNum);
    }
    else if (strcmp(profile, paramList) != 0)
    {
      semerr("procedure parameters do not match", tok->lex, tok->lineNum);
    }
  }
  else if (strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
    strcpy(paramList, "0");
    if (strcmp(profile, paramList) != 0)
    {
      semerr("procedure parameters do not match", tok->lex, tok->lineNum);
    }
  }
  else
  {
    synerr("LPAR, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void proc_stmt()
{
  char currentProfile[25] = "";

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "CALL") == 0)
  {
    match("CALL");
    getProfile(tok->lex);
    checkProc(tok->lex);
    match("ID");
    paramList = (char *)malloc(sizeof(char) * 25);
    strcpy(paramList, "");
    proc_stmtPrime();
  }
  else
  {
    synerr("CALL", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

int varPrime(int varPrime_i)
{
  int varPrime_t;
  int expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ASSIGN");

  if (strcmp(tok->tokStr, "LBRACK") == 0)
  {
    match("LBRACK");
    expr_t = expr();
    match("RBRACK");
    varPrime_t = checkVarPrime(expr_t, varPrime_i);
    freeSynchSet(set);
    return varPrime_t;
  }
  else if (strcmp(tok->tokStr, "ASSIGN") == 0)
  {
    varPrime_t = varPrime_i;
    freeSynchSet(set);
    return varPrime_t;
  }
  else
  {
    synerr("LBRACK or ASSIGN", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int var()
{
  int id_t;
  int varPrime_i;
  int varPrime_t;
  int var_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ASSIGN");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    id_t = getType(tok->lex);
    match("ID");
    varPrime_i = id_t;
    varPrime_t = varPrime(varPrime_i);
    var_t = varPrime_t;
    freeSynchSet(set);
    return var_t;
  }
  else
  {
    synerr("ID", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

void stmtPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ELSE") == 0)
  {
    match("ELSE");
    stmt();
  }
  else if (strcmp(tok->tokStr, "SEMI") == 0 || strcmp(tok->tokStr, "END") == 0)
  {
  }
  else
  {
    synerr("ELSE, SEMI, or END", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void stmt()
{
  int var_t;
  int expr_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    var_t = var();
    match("ASSIGN");
    expr_t = expr();
    checkVarT(var_t, expr_t);
  }
  else if (strcmp(tok->tokStr, "CALL") == 0)
  {
    proc_stmt();
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    cmpd_stmt();
  }
  else if (strcmp(tok->tokStr, "IF") == 0)
  {
    match("IF");
    expr_t = expr();
    checkExprT(expr_t);
    match("THEN");
    stmt();
    stmtPrime();
  }
  else if (strcmp(tok->tokStr, "WHILE") == 0)
  {
    match("WHILE");
    expr_t = expr();
    checkExprT(expr_t);
    match("DO");
    stmt();
  }
  else
  {
    synerr("ID, CALL, BEG, IF, or WHILE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }
  freeSynchSet(set);
}

void stmt_listPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "END");

  if (strcmp(tok->tokStr, "SEMI") == 0)
  {
    match("SEMI");
    stmt();
    stmt_listPrime();
  }
  else if (strcmp(tok->tokStr, "END") == 0)
  {
  }
  else
  {
    synerr("SEMI or END", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void stmt_list()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "END");

  if (strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "CALL") == 0 ||
      strcmp(tok->tokStr, "BEG") == 0 ||
      strcmp(tok->tokStr, "IF") == 0 ||
      strcmp(tok->tokStr, "WHILE") == 0)
  {
    stmt();
    stmt_listPrime();
  }
  else
  {
    synerr("ID, CALL, BEG, IF, or WHILE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void opt_stmts()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "END");

  if (strcmp(tok->tokStr, "ID") == 0 ||
      strcmp(tok->tokStr, "CALL") == 0 ||
      strcmp(tok->tokStr, "BEG") == 0 ||
      strcmp(tok->tokStr, "IF") == 0 ||
      strcmp(tok->tokStr, "WHILE") == 0)
  {
    stmt_list();
  }
  else
  {
    synerr("ID, CALL, BEG, IF, or WHILE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void cmpd_stmtPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "DOT");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "END") == 0)
  {
    match("END");
  }
  else if (strcmp(tok->tokStr, "ID") == 0 ||
           strcmp(tok->tokStr, "CALL") == 0 ||
           strcmp(tok->tokStr, "BEG") == 0 ||
           strcmp(tok->tokStr, "IF") == 0 ||
           strcmp(tok->tokStr, "WHILE") == 0)
  {
    opt_stmts();
    match("END");
  }
  else
  {
    synerr("END, ID, CALL, BEG, IF, or WHILE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void cmpd_stmt()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "DOT");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "BEG") == 0)
  {
    match("BEG");
    cmpd_stmtPrime();
  }
  else
  {
    synerr("BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void param_listPrime()
{
  int type_t;
  char currLex[100] = "";
  int currType;
  char typeStr[10] = "";

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "SEMI") == 0)
  {
    match("SEMI");
    strcpy(currLex, tok->lex);
    currType = tok->tokInt;
    match("ID");
    match("COLON");
    type_t = type();
    if (currType != 99 && type_t != 99)
    {
      if (type_t <= 4 && type_t >= 1)
      {
        checkAddBlueNode(currLex, type_t + 4);
        sprintf(typeStr, "%d", type_t);
        strcat(profile, typeStr);
      }
      else
      {
        printf("error in param_listPrime");
      }
    }
    param_listPrime();
  }
  else if (strcmp(tok->tokStr, "RPAR") == 0)
  {
  }
  else
  {
    synerr("SEMI or RPAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void param_list()
{
  int type_t;
  char currLex[100] = "";
  int currType;
  char typeStr[10] = "";

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    strcpy(currLex, tok->lex);
    currType = tok->tokInt;
    match("ID");
    match("COLON");
    type_t = type();
    if (currType != 99 && type_t != 99)
    {
      if (type_t <= 4 && type_t >= 1)
      {
        checkAddBlueNode(currLex, type_t + 4);
        sprintf(typeStr, "%d", type_t);
        strcat(profile, typeStr);
      }
      else
      {
        printf("error in param_list");
      }
    }
    param_listPrime();
  }
  else
  {
    synerr("ID", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void args()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");

  if (strcmp(tok->tokStr, "LPAR") == 0)
  {
    match("LPAR");
    param_list();
    match("RPAR");
  }
  else
  {
    synerr("LPAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_headPrime()
{
  struct Profile *currentProfile = NULL;
  currentProfile = (struct Profile *)malloc(sizeof(struct Profile));

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "VAR");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "SEMI") == 0)
  {
    strcpy(profile, "0");
    match("SEMI");
  }
  else if (strcmp(tok->tokStr, "LPAR") == 0)
  {
    profile = (char *)malloc(sizeof(char) * 25);
    strcpy(profile, "");
    args();
    match("SEMI");
  }
  else
  {
    synerr("SEMI or LPAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_head()
{
  char currLex[100] = "";
  struct Profile *currentProfile = NULL;
  currentProfile = (struct Profile *)malloc(sizeof(struct Profile));

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "VAR");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    match("PROC");
    if (tok->tokInt != 99)
    {
      strcpy(currLex, tok->lex);
      checkAddGreenNode(tok->lex, 100);
    }
    offset = 0;
    fprintf(outTable, "%s\n", tok->lex);

    match("ID");
    subprog_headPrime();

    if (profileHead == NULL)
    {
      strcpy(currentProfile->lex, currLex);
      strcpy(currentProfile->profStr, profile);
      profileHead = currentProfile;
    }
    else
    {
      strcpy(currentProfile->lex, currLex);
      strcpy(currentProfile->profStr, profile);
      struct Profile *temp = profileHead;
      while (temp->next != NULL)
      {
        temp = temp->next;
      }
      temp->next = currentProfile;
      currentProfile->next = NULL;
    }
  }
  else
  {
    synerr("PROC", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_decDPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_decs();
    cmpd_stmt();
    pop();
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    cmpd_stmt();
    pop();
  }
  else
  {
    synerr("PROC or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_decPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_decs();
    cmpd_stmt();
    pop();
  }
  else if (strcmp(tok->tokStr, "VAR") == 0)
  {
    decs();
    subprog_decDPrime();
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    cmpd_stmt();
    pop();
  }
  else
  {
    synerr("PROC, VAR, or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_dec()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_head();
    subprog_decPrime();
  }
  else
  {
    synerr("PROC", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_decsPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_dec();
    match("SEMI");
    subprog_decsPrime();
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
  }
  else
  {
    synerr("PROC or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void subprog_decs()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_dec();
    match("SEMI");
    subprog_decsPrime();
  }
  else
  {
    synerr("PROC", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

int std_type()
{
  int std_type_t;
  int std_type_s;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "INT") == 0)
  {
    std_type_t = 1;
    typeSize = 4;
    match("INT");
    freeSynchSet(set);
    return std_type_t;
  }
  else if (strcmp(tok->tokStr, "REAL") == 0)
  {
    std_type_t = 2;
    typeSize = 8;
    match("REAL");
    freeSynchSet(set);
    return std_type_t;
  }
  else
  {
    synerr("INT or REAL", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

int type()
{
  int std_type_t;
  int type_t;
  int num1;
  int num1_t;
  int num2;
  int num2_t;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "INT") == 0)
  {
    std_type_t = std_type();
    type_t = std_type_t;
    typeSize = 4;
    freeSynchSet(set);
    return type_t;
  }
  else if (strcmp(tok->tokStr, "REAL") == 0)
  {
    std_type_t = std_type();
    type_t = std_type_t;
    typeSize = 8;
    freeSynchSet(set);
    return type_t;
  }
  else if (strcmp(tok->tokStr, "ARR") == 0)
  {
    match("ARR");
    match("LBRACK");
    num1 = atoi(tok->lex);
    num1_t = tok->attr.attrInt;
    match("NUM");
    match("DOTDOT");
    num2 = atoi(tok->lex);
    num2_t = tok->attr.attrInt;
    match("NUM");
    match("RBRACK");
    match("OF");
    if (num2 >= num1)
    {
      if (num1_t == 1 && num2_t == 1)
      {
        std_type_t = std_type();
        typeSize = ((num2 - num1) + 1) * typeSize;
      }
      else if (num1_t == 99 || num2_t == 99)
      {
        std_type_t = 99;
        typeSize = 0;
      }
      else
      {
        std_type_t = 99;
        typeSize = 0;
        semerr("expected num type int1", tok->lex, tok->lineNum);
      }
    }
    else
    {
      std_type_t = 99;
      typeSize = 0;
      semerr("expected num2 to be greater than or equal to num1", tok->lex, tok->lineNum);
    }
    type_t = checkStdTypeT(std_type_t);
    freeSynchSet(set);
    return type_t;
  }
  else
  {
    synerr("INT, REAL, or ARR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
    freeSynchSet(set);
    return 0;
  }
}

void decsPrime()
{
  int type_t;
  char currentLex[100] = "";
  int currentType;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "VAR") == 0)
  {
    match("VAR");
    strcpy(currentLex, tok->lex);
    currentType = tok->tokInt;
    match("ID");
    match("COLON");
    type_t = type();
    if (currentType != 99 && type_t != 99)
    {
      checkAddBlueNode(currentLex, type_t);
      fprintf(outTable, "%s\t%d\n", currentLex, offset);
      offset = offset + typeSize;
    }
    match("SEMI");
    decsPrime();
  }
  else if (strcmp(tok->tokStr, "PROC") == 0 || strcmp(tok->tokStr, "BEG") == 0)
  {
  }
  else
  {
    synerr("VAR, PROC, or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void decs()
{
  int type_t;
  char currentLex[100] = "";
  int currentType;

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "VAR") == 0)
  {
    match("VAR");
    strcpy(currentLex, tok->lex);
    currentType = tok->tokInt;
    match("ID");
    match("COLON");
    type_t = type();
    if (currentType != 99 && type_t != 99)
    {
      checkAddBlueNode(currentLex, type_t);
      fprintf(outTable, "%s\t%d\n", currentLex, offset);
      offset = offset + typeSize;
    }
    match("SEMI");
    decsPrime();
  }
  else
  {
    synerr("VAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void id_listPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "COMMA") == 0)
  {
    match("COMMA");
    if (tok->tokInt != 99)
    {
      checkAddBlueNode(tok->lex, 101);
    }
    match("ID");
    id_listPrime();
  }
  else if (strcmp(tok->tokStr, "RPAR") == 0)
  {
  }
  else
  {
    synerr("COMMA or RPAR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void id_list()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    if (tok->tokInt != 99)
    {
      checkAddBlueNode(tok->lex, 101);
    }
    match("ID");
    id_listPrime();
  }
  else
  {
    synerr("ID", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void programDPrime() // This might have a problem
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");

  if (strcmp(tok->tokStr, "PROC") == 0 || strcmp(tok->tokStr, "BEG") == 0)
  {
    subprog_decs();
    cmpd_stmt();
    match("DOT");
  }
  else
  {
    synerr("PROC or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void programPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");

  if (strcmp(tok->tokStr, "VAR") == 0)
  {
    decs();
    programDPrime();
  }
  else if (strcmp(tok->tokStr, "PROC") == 0)
  {
    subprog_decs();
    cmpd_stmt();
    match("DOT");
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    cmpd_stmt();
    match("DOT");
  }
  else
  {
    synerr("VAR, PROC, or BEG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void program()
{

  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");

  if (strcmp(tok->tokStr, "PROG") == 0)
  {
    match("PROG");
    if (tok->tokInt != 99)
    {
      checkAddGreenNode(tok->lex, 102);
    }
    fprintf(outTable, "%s\n", tok->lex);
    match("ID");
    match("LPAR");
    id_list();
    match("RPAR");
    match("SEMI");
    programPrime();
  }
  else
  {
    synerr("PROG", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void parse()
{
  tok = getNextTok();
  program();
  match("EOF");
}

int main()
{
  // FILE *file = fopen("SamplePascalNoErrors.txt", "r");
  // FILE *file = fopen("SamplePascalWithErrors.txt", "r");
  // FILE *file = fopen("Stub.txt", "r");
  FILE *file = fopen("ShenoiProg.txt", "r");
  // FILE *file = fopen("Prog10SynErrs.txt", "r");
  // FILE *file = fopen("ProgBothErrs.txt", "r");
  // FILE *file = fopen("ShenoiProgSynErrs.txt", "r");
  // FILE *file = fopen("ShenoiProgBothErrs.txt", "r");

  listingFile = fopen("ListingFile.txt", "w");

  if (file == NULL)
  {
    return 1;
  }
  FILE *ResWordFile = fopen("ReservedWords.txt", "r");
  if (ResWordFile == NULL)
  {
    return 1;
  }

  int i = 0;
  while (fgets(line, MAX_LINE_LENGTH, ResWordFile))
  {
    sscanf(line, "%s %s %d %s %d", resWordArr[i].resLex, resWordArr[i].resTokStr, &(resWordArr[i].resTokInt), resWordArr[i].resAttrStr, &(resWordArr[i].resAttrInt));
    i++;
  }

  tokenFile = fopen("TokenFile.txt", "w");

  while (fgets(line, MAX_LINE_LENGTH, file))
  {
    lineNum++;
    fprintf(listingFile, "%d  %s", lineNum, line);
    f = 0;
    b = 0;

    while (line[f] != '\0')
    {
      f = b;
      if (whitespace(line) == 0)
      {
        b = f;
        continue;
      }
      else if (idRes(line) == 0)
      {
        b = f;
        continue;
      }
      else if (catchall(line) == 0)
      {
        b = f;
        continue;
      }
      else if (longReal(line) == 0)
      {
        b = f;
        continue;
      }
      else if (real(line) == 0)
      {
        b = f;
        continue;
      }
      else if (intMach(line) == 0)
      {
        b = f;
        continue;
      }
      else if (relop(line) == 0)
      {
        b = f;
        continue;
      }
      else if (checkerr(line) == 0)
      {
        b = f;
        continue;
      }
    }
  }

  printTok(lineNum, "EOF", "EOF", 58, "NIL", 44);
  // printList(symTable);
  clear(&symTable);
  fclose(file);
  fclose(ResWordFile);
  fclose(tokenFile);
  fclose(listingFile);

  readToksIn(&tokList);
  outTable = fopen("symbolTable.txt", "w+");
  parse();

  insertErrs();
  free(lexErrors);
  free(synErrors);
  free(semErrors);

  return 0;
}
