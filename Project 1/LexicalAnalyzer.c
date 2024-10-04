#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "LinkedList.h"

#define MAX_LINE_LENGTH 72

struct ResWord
{
  char resLex[10];
  char resTokStr[10];
  int resTokInt;
  char resAttrStr[10];
  int resAttrInt;
};

FILE *listingFile;
FILE *tokenFile;
char line[MAX_LINE_LENGTH];
char lex[MAX_LINE_LENGTH];
int f = 0;
int b = 0;
int lineNum = 0;
struct ResWord resWordArr[20];
struct Node *symTable = NULL;

void printTok(int lineNo, char *lexeme, char *tokName, int tokInt, char *attrName, int attrInt)
{
  fprintf(tokenFile, "%d %s %s %d %s %d\n", lineNo, lexeme, tokName, tokInt, attrName, attrInt);
}

int checkResWord(char *lexeme)
{
  // printf("Lex: %s\n", lexeme);
  for (int i = 0; i < 20; i++)
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
  while (str[f] == '\n' || str[f] == '\t' || str[f] == ' ')
  {
    f++;
  }
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

    // printf("IDRES: %s\n", lex);
    if (checkResWord(lex) == 1)
    {
      printTok(lineNum, lex, "ID", 53, "NIL", 44);
      if (isInList(&symTable, lex) == 1)
      {
        union Attr attr;
        char *symbol = malloc(strlen(lex) + 1);
        strncpy(symbol, lex, strlen(lex));
        symbol[strlen(lex)] = '\0';
        attr.symPtr = (void *)symbol;
        append(&symTable, lineNum, lex, "ID", 53, "NIL", attr);
      }
    }
    if (strlen(lex) > 10)
    {
      fprintf(listingFile, "LEXERR: Extra Long ID: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long ID", 99);
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

  // Step 1: Read digits before the decimal point (store in lex1)
  if (isdigit(str[f]))
  {
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
        // Could be a real
        // printf("REAL: %s\n", lex);
        printTok(lineNum, lex, "NUM", 16, "REAL", 19);
        if (strlen(lex1) > 5)
        {
          fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Extra Long Whole Part", 99);
        }
        if (strlen(lex1) > 0 && lex1[0] == '0')
        {
          fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Leading 0 in Whole Part", 99);
        }
        if (strlen(lex2) > 5)
        {
          fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Extra Long Fractional Part", 99);
        }
        if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
        {
          fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Trailing 0 in Fractional Part", 99);
        }
        return 0;
      }
    }
    else
    {
      // Check for malformed ids that begin with digits
      if (str[f] != ' ')
      {
        int alpha = 0;
        while (!isspace(str[f]))
        {
          if (isalpha(str[f]))
          {
            alpha = 1;
          }
          tmp[0] = str[f];
          strcat(lex, tmp);
          f++;
        }
        if (alpha == 1)
        {
          fprintf(listingFile, "LEXERR: Unrecognized Symbol: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Unrecognized Symbol", 99);
        }
      }
      // Could be an int
      else
      {
        // printf("INT: %s\n", lex);
        printTok(lineNum, lex, "NUM", 16, "INT", 18);
        if (strlen(lex) > 10)
        {
          fprintf(listingFile, "LEXERR: Extra Long Integer: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Extra Long Int", 99);
        }
        if (strlen(lex) > 1 && lex[0] == '0')
        {
          fprintf(listingFile, "LEXERR: Leading 0 on Integer: %s\n", lex);
          printTok(lineNum, lex, "LEXERR", 99, "Leading 0", 99);
        }
      }
      return 0;
    }

    // printf("LONGREAL: %s\n", lex);
    printTok(lineNum, lex, "NUM", 16, "REAL", 19);

    // Whole part
    if (strlen(lex1) > 5)
    {
      fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Whole Part", 99);
    }
    if (strlen(lex1) > 0 && lex1[0] == '0')
    {
      fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Leading 0", 99);
    }

    // Frational Part
    if (strlen(lex2) > 5)
    {
      fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Fractional Part", 99);
    }
    if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
    {
      fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Trailing 0 in Fractional Part", 99);
    }

    // Exponential Part
    if (strlen(lex3) > 2)
    {
      fprintf(listingFile, "LEXERR: Extra Long Exponential Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Extra Long Exponent", 99);
    }
    if (strlen(lex3) > 0 && lex3[0] == '0')
    {
      fprintf(listingFile, "LEXERR: Leading 0 in Exponential Part: %s\n", lex);
      printTok(lineNum, lex, "LEXERR", 99, "Leading 0 in Exponent", 99);
    }
    // printf("Before decimal (lex1): %s\n", lex1);
    // printf("After decimal (lex2): %s\n", lex2);
    // printf("Exponent part (lex3): %s\n", lex3);

    return 0;
  }

  return 1;
}

int relop(char *str) // HERE***
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

void checkerr(char *str)
{
  char tmp[2];
  tmp[1] = '\0';

  if (ispunct(str[f]))
  {
    memset(lex, 0, sizeof(lex));
    while (!isspace(str[f]))
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
    }
    fprintf(listingFile, "LEXERR: Unrecognized Symbol: %s\n", lex);
    printTok(lineNum, lex, "LEXERR", 99, "Unrecognized Symbol", 99);
  }
}

int main()
{
  FILE *file = fopen("SamplePascalWithErrors.txt", "r");
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

  listingFile = fopen("ListingFile.txt", "w");
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
      else if (relop(line) == 0)
      {
        b = f;
        continue;
      }
      else
      {
        checkerr(line);
        break;
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
  return 0;
}
