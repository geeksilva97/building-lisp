#include "mpc.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32

// gcc parsing.c mpc.c -ledit -lm -o parsing

static char buffer[2048];

char *readline(char *prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char *cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void add_history(char *unused) {}

#else
#include <editline/readline.h>
#endif


long eval_op(long x, char *op, long y) {
  if (strcmp(op, "^") == 0) {
    return pow(x, y);
  }

  if (strcmp(op, "%") == 0) {
    return x % y;
  }

  if (strcmp(op, "+") == 0) {
    return x + y;
  }

  if (strcmp(op, "-") == 0) {
    return x - y;
  }

  if (strcmp(op, "*") == 0) {
    return x * y;
  }

  if (strcmp(op, "/") == 0) {
    return x / y;
  }

  return 0;
}

long eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char *op = t->children[1]->contents;

  long x = eval(t->children[2]);

  int i = 3;

  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Stmt = mpc_new("stmt");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispify = mpc_new("lispify");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number   : /-?[0-9]+/ ;                             \
    stmt     : (\"max\" | \"min\") <expr>+ ;             \
    operator : '+' | '-' | '*' | '/' | '%' | '^' ;      \
    expr     : <number> | '(' <operator> <expr>+ ')' | <stmt>;  \
    lispify    : /^/ <operator> <expr>+ /$/ ;             \
  ",
            Number, Stmt, Operator, Expr, Lispify);

  puts("Lispify Version 0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispify> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispify, &r)) {
      /* On success print and delete the AST */
      mpc_ast_print(r.output);

      /* long result = eval(r.output); */

      /* printf("%li\n", result); */

      mpc_ast_delete(r.output);
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispify);

  return 0;
}
