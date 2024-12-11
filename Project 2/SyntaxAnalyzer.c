#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "LinkedList.h"

#define MAX_LINE_LENGTH 72
#define MAX_STRINGS 15
#define MAX_STRING_LENGTH 100

typedef struct
{
  char **strings; // Pointer to an array of string pointers
  int count;      // Current number of strings
} SynchSet;

char line[MAX_LINE_LENGTH];
struct Node *tokList = NULL;
struct Node *tok = NULL;

// --------------------------------------------------------------------------------------------------------------------------------------
// START SYNCH SET

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

// END SYNCH SET
// --------------------------------------------------------------------------------------------------------------------------------------
// START TOKENS

void readToksIn(struct Node **head)
{
  // FILE *tokFile = fopen("TokenFile.txt", "r");
  // FILE *tokFile = fopen("TokenFileCorrectProgram.txt", "r");
  FILE *tokFile = fopen("TokenFileShenoiProg.txt", "r");

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
  printf("SYNERR: Expected %s, but Received %s. Line %d. Lex: %s\n", exp, rec, tok->lineNum, tok->lex);
}

// END TOKENS
// --------------------------------------------------------------------------------------------------------------------------------------
// START PARSE TABLE
void subprog_decs();
void decs();
void type();
void cmpd_stmt();
void stmt();
void expr();

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
    synerr("PROC-ERR", tok->tokStr);
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
  readToksIn(&tokList);
  parse();
  // clear(&head);
  return 0;
}
