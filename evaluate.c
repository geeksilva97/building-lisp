#include "mpc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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

long min(long *args, int num_args) {

  long min = args[0];

  for (int i = 1; i < num_args; i++) {
    if (args[i] < min) {
      min = args[i];
    }
  }

  return min;
}

long max(long *args, int num_args) {
  long max = args[0];

  for (int i = 1; i < num_args; i++) {
    if (args[i] > max) {
      max = args[i];
    }
  }

  return max;
}

long eval(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  if (strstr(t->tag, "stmt")) {
    char *op = t->children[0]->contents;
    int num_args = t->children_num - 1;
    long *func_args = malloc(sizeof(long) * num_args);

    mpc_ast_t **child_ptr = t->children;

    for (int i = 1; i < t->children_num; i++) {
      mpc_ast_t *child =
          *(child_ptr + i); // Use pointer arithmetic to access the child
      func_args[i - 1] = eval(child);
    }

    if (strcmp(op, "min") == 0) {
      return min(func_args, num_args);
    } else if (strcmp(op, "max") == 0) {
      return max(func_args, num_args);
    } else {
      puts("Unknown operator\n");
      return -1000;
    }

    return -1;
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
    operator : '+' | '-' | '*' | '/' | '%' | '^' ;      \
    stmt     : (\"max\" | \"min\");             \
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
      /* mpc_ast_print(r.output); */

      long result = eval(r.output);

      printf("%li\n", result);

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
