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

// Global variables to store errors
LexErr *lexErrors = NULL; // Dynamic array of errors
SynErr *synErrors = NULL; // Dynamic array of errors
int lexErrCount = 0;      // Number of errors
int lexErrCapacity = 0;   // Current capacity of the errors array
int synErrCount = 0;      // Number of errors
int synErrCapacity = 0;   // Current capacity of the errors array
char lexErrMsg[256];

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

void subprog_decs();
void decs();
void type();
void cmpd_stmt();
void stmt();
void expr();

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

// Function to insert syntax errors into the listing file
void insertErrs()
{
  FILE *listingFile = fopen("ListingFile.txt", "r");
  if (!listingFile)
  {
    printf("Error opening listing file\n");
    return;
  }

  // printf("%d Errors\n", synErrCount);

  // Read the original file into memory
  char **lines = NULL;
  int lineCount = 0;
  char buffer[1024];

  while (fgets(buffer, sizeof(buffer), listingFile))
  {
    // printf("here\n");
    lines = realloc(lines, sizeof(char *) * (lineCount + 1));
    lines[lineCount] = strdup(buffer);
    lineCount++;
  }
  fclose(listingFile);

  // Open the listing file for writing
  listingFile = fopen("ListingFile.txt", "w");
  // if (!listingFile)
  // {
  //   perror("Error reopening listing file for writing");
  //   return;
  // }

  // int x = 0;

  // int lenLexErrs = sizeof(lexErrors) / sizeof(lexErrors[0]);
  // int lenSynErrs = sizeof(synErrors) / sizeof(synErrors[0]);
  // Loop through lines and insert errors where needed
  for (int i = 0; i < lineCount; i++)
  {
    // printf("got here\n");
    // Write the current line
    fprintf(listingFile, "%s", lines[i]);

    // printf("%s, %d", lexErrors[0].err, lexErrCount);

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

    // Check if any LEXERR or SYNERR belongs to this line
    // int lexErrWritten = 0; // To track if LEXERRs are written
    // for (int j = 0; j < errorCount; j++)
    // {
    // printf("%d\n", j);
    // if (errors[j].lineNum == i)
    // { // Line numbers are 1-based
    // printf("%s\n", lines[i + 1]);
    // printf("Error on Line: i = %d, lineNum = %d\n", i, errors[j].lineNum);

    // if (strstr(lines[i], "LEXERR:"))
    // {
    //   x--;
    //   // printf("%s\n", lines[i]);
    //   // printf("There is a LEXERR on line %d\n", i);
    //   lexErrWritten = 1; // LEXERR already exists
    // }
    // }
    // }

    // Now write SYNERRs, ensuring they follow any LEXERRs
    // for (int j = 0; j < synErrCount; j++)
    // {
    //   if (synErrors[j].lineNum == i + 1 + x)
    //   {
    //     if (lexErrWritten == 1)
    //     {
    //       lexErrWritten = 0;
    //     }
    //     else
    //     {
    //       printf(lines[i]);
    //       printf("SYNERR on line %d\n", synErrors[j].lineNum);
    //       fprintf(listingFile, "SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", synErrors[j].exp, synErrors[j].rec, synErrors[j].lineNum, synErrors[j].lex);
    //     }
    //     // errors[j].lineNum = errors[j].lineNum + 500;
    //   }
    // if (errors[j].lineNum == i && strstr(lines[i], "LEXERR:"))
    // {
    //   // if (!lexErrWritten || strstr(lines[i+1], "LEXERR:"))
    //   // {
    //   printf("Actual LineNum: %d\n", errors[j].lineNum);
    //   // fprintf(listingFile, "SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", errors[j].exp, errors[j].rec, errors[j].lineNum, errors[j].lex);
    //   // }
    // }
    // else if (errors[j].lineNum == i + 1 && !lexErrWritten)
    // {
    //   // fprintf(listingFile, "SYNERRx: Expected %s, but Received %s. Line %d. Lex: %s\n", errors[j].exp, errors[j].rec, errors[j].lineNum, errors[j].lex);
    // }
    // }
  }

  // Clean up memory
  for (int i = 0; i < lineCount; i++)
  {
    free(lines[i]);
  }
  free(lines);
  fclose(listingFile);
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

    printTok(lineNum, lex, "NUM", 16, "REAL", 19);
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
        printTok(lineNum, lex, "NUM", 16, "REAL", 19);
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

    printTok(lineNum, lex, "NUM", 16, "INT", 18);
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

void sign()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ID");
  addSynch(set, "NUM");
  addSynch(set, "LPAR");
  addSynch(set, "NOT");

  if (strcmp(tok->tokStr, "PLUS") == 0)
  {
    match("PLUS");
  }
  else if (strcmp(tok->tokStr, "MINUS") == 0)
  {
    match("MINUS");
  }
  else
  {
    synerr("PLUS or MINUS", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void factorPrime()
{
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
    expr();
    match("RBRACK");
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
  }
  else
  {
    synerr("LBRACK, MULOP, ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void factor()
{
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
    match("ID");
    factorPrime();
  }
  else if (strcmp(tok->tokStr, "NUM") == 0)
  {
    match("NUM");
  }
  else if (strcmp(tok->tokStr, "LPAR") == 0)
  {
    match("LPAR");
    expr();
    match("RPAR");
  }
  else if (strcmp(tok->tokStr, "NOT") == 0)
  {
    match("NOT");
    factor();
  }
  else
  {
    synerr("ID, NUM, LPAR, or NOT", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void termPrime()
{
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
    match("MULOP");
    factor();
    termPrime();
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
  }
  else
  {
    synerr("MULOP, ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void term()
{
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
    factor();
    termPrime();
  }

  freeSynchSet(set);
}

void simple_exprPrime()
{
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
    match("ADDOP");
    term();
    simple_exprPrime();
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
  }
  else
  {
    synerr("ADDOP, RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void simple_expr()
{
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
    term();
    simple_exprPrime();
  }
  else if (strcmp(tok->tokStr, "PLUS") == 0 ||
           strcmp(tok->tokStr, "MINUS") == 0)
  {
    sign();
    term();
    simple_exprPrime();
  }
  else
  {
    synerr("ID, NUM, LPAR, NOT, PLUS, MINUS", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void exprPrime()
{
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
    match("RELOP");
    simple_expr();
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
  }
  else
  {
    synerr("RELOP, THEN, DO, COMMA, RPAR, RBRACK, SEMI, END, or ELSE", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void expr()
{
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
    simple_expr();
    exprPrime();
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

void expr_listPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "COMMA") == 0)
  {
    match("COMMA");
    expr();
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
    expr();
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
  }
  else if (strcmp(tok->tokStr, "SEMI") == 0 ||
           strcmp(tok->tokStr, "END") == 0 ||
           strcmp(tok->tokStr, "ELSE") == 0)
  {
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "CALL") == 0)
  {
    match("CALL");
    match("ID");
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

void varPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ASSIGN");

  if (strcmp(tok->tokStr, "LBRACK") == 0)
  {
    match("LBRACK");
    expr();
    match("RBRACK");
  }
  else if (strcmp(tok->tokStr, "ASSIGN") == 0)
  {
  }
  else
  {
    synerr("LBRACK or ASSIGN", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void var()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "ASSIGN");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    match("ID");
    varPrime();
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "END");
  addSynch(set, "ELSE");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    var();
    match("ASSIGN");
    expr();
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
    expr();
    match("THEN");
    stmt();
    stmtPrime();
  }
  else if (strcmp(tok->tokStr, "WHILE") == 0)
  {
    match("WHILE");
    expr();
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "SEMI") == 0)
  {
    match("SEMI");
    match("ID");
    match("COLON");
    type();
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "ID") == 0)
  {
    match("ID");
    match("COLON");
    type();
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "VAR");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "SEMI") == 0)
  {
    match("SEMI");
  }
  else if (strcmp(tok->tokStr, "LPAR") == 0)
  {
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "VAR");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "PROC") == 0)
  {
    match("PROC");
    match("ID");
    subprog_headPrime();
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
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    // subprog_decs();
    cmpd_stmt();
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
  }
  else if (strcmp(tok->tokStr, "VAR") == 0)
  {
    decs();
    subprog_decDPrime();
  }
  else if (strcmp(tok->tokStr, "BEG") == 0)
  {
    cmpd_stmt();
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

void std_type()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "INT") == 0)
  {
    match("INT");
  }
  else if (strcmp(tok->tokStr, "REAL") == 0)
  {
    match("REAL");
  }
  else
  {
    synerr("INT or REAL", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void type()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "SEMI");
  addSynch(set, "RPAR");

  if (strcmp(tok->tokStr, "INT") == 0 || strcmp(tok->tokStr, "REAL") == 0)
  {
    std_type();
  }
  else if (strcmp(tok->tokStr, "ARR") == 0)
  {
    match("ARR");
    match("LBRACK");
    match("NUM");
    match("DOTDOT");
    match("NUM");
    match("RBRACK");
    match("OF");
    std_type();
  }
  else
  {
    synerr("INT, REAL, or ARR", tok->tokStr);
    while (searchSynch(set, tok->tokStr) != 1)
    {
      tok = getNextTok();
    }
  }

  freeSynchSet(set);
}

void decsPrime()
{
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "VAR") == 0)
  {
    match("VAR");
    match("ID");
    match("COLON");
    type();
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
  SynchSet *set = createSynchSet();
  addSynch(set, "EOF");
  addSynch(set, "PROC");
  addSynch(set, "BEG");

  if (strcmp(tok->tokStr, "VAR") == 0)
  {
    match("VAR");
    match("ID");
    match("COLON");
    type();
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

void programDPrime()
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
  // FILE *file = fopen("ShenoiProg.txt", "r");
  // FILE *file = fopen("Prog10SynErrs.txt", "r");
  FILE *file = fopen("ProgBothErrs.txt", "r");
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
  printList(symTable);
  clear(&symTable);
  fclose(file);
  fclose(ResWordFile);
  fclose(tokenFile);
  fclose(listingFile);

  readToksIn(&tokList);
  parse();

  insertErrs();
  free(lexErrors);
  free(synErrors);

  return 0;
}
