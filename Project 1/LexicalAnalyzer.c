#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 72

FILE *listingFile;
char line[MAX_LINE_LENGTH];
char lex[MAX_LINE_LENGTH];
int f = 0;
int b = 0;

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

    printf("IDRES: %s\n", lex);
    if (strlen(lex) > 10)
    {
      fprintf(listingFile, "LEXERR: Extra Long ID: %s\n", lex);
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
    printf("CATCHALL: :=\n");
    return 0;
  }

  if (str[f] == '.' && str[f + 1] == '.')
  {
    f += 2;
    printf("CATCHALL: ..\n");
    return 0;
  }

  if (str[f] == '+')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '-')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '*')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '/')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == ';')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '.')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == ',')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '[')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == ']')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == '(')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == ')')
  {
    printf("CATCHALL: %c\n", str[f]);
    f++;
    return 0;
  }
  if (str[f] == ':')
  {
    printf("CATCHALL: %c\n", str[f]);
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
        printf("REAL: %s\n", lex);
        if (strlen(lex1) > 5)
        {
          fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
        }
        if (strlen(lex1) > 0 && lex1[0] == '0')
        {
          fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
        }
        if (strlen(lex2) > 5)
        {
          fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
        }
        if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
        {
          fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
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
        }
      }
      // Could be an int
      else
      {
        printf("INT: %s\n", lex);
        if (strlen(lex) > 10)
        {
          fprintf(listingFile, "LEXERR: Extra Long Integer: %s\n", lex);
        }
        if (strlen(lex) > 1 && lex[0] == '0')
        {
          fprintf(listingFile, "LEXERR: Leading 0 on Integer: %s\n", lex);
        }
      }
      return 0;
    }

    printf("LONGREAL: %s\n", lex);

    // Whole part
    if (strlen(lex1) > 5)
    {
      fprintf(listingFile, "LEXERR: Extra Long Whole Part: %s\n", lex);
    }
    if (strlen(lex1) > 0 && lex1[0] == '0')
    {
      fprintf(listingFile, "LEXERR: Leading 0 in Whole Part: %s\n", lex);
    }

    // Frational Part
    if (strlen(lex2) > 5)
    {
      fprintf(listingFile, "LEXERR: Extra Long Fractional Part: %s\n", lex);
    }
    if (strlen(lex2) > 0 && lex2[strlen(lex2) - 1] == '0')
    {
      fprintf(listingFile, "LEXERR: Trailing 0 in Fractional Part: %s\n", lex);
    }

    // Exponential Part
    if (strlen(lex3) > 2)
    {
      fprintf(listingFile, "LEXERR: Extra Long Exponential Part: %s\n", lex);
    }
    if (strlen(lex3) > 0 && lex3[0] == '0')
    {
      fprintf(listingFile, "LEXERR: Leading 0 in Exponential Part: %s\n", lex);
    }
    // printf("Before decimal (lex1): %s\n", lex1);
    // printf("After decimal (lex2): %s\n", lex2);
    // printf("Exponent part (lex3): %s\n", lex3);

    return 0;
  }

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
      printf("RELOP: %s\n", lex);
      return 0;
    }
    else if (str[f] == '>')
    {
      tmp[0] = str[f];
      strcat(lex, tmp);
      f++;
      printf("RELOP: %s\n", lex);
      return 0;
    }
    else
    {
      printf("RELOP: %s\n", lex);
      return 0;
    }
  }
  else if (str[f] == '=')
  {
    tmp[0] = str[f];
    strcat(lex, tmp);
    f++;
    printf("RELOP: %s\n", lex);
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
      printf("RELOP: %s\n", lex);
      return 0;
    }
    else
    {
      printf("RELOP: %s\n", lex);
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
  }
}

int main()
{
  FILE *file = fopen("SamplePascal.txt", "r");
  if (file == NULL)
  {
    return 1;
  }

  int lineNum = 0;
    listingFile = fopen("ListingFile.txt", "w");

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

  fclose(file);
  return 0;
}
