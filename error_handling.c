#include "mpc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
  int type;
  union { // Add union to save memory since num and err are never used at the same time we can use the same memory location
    double num;
    int err;
  };
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(double x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%f", v.num);
    break;

  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division By Zero!");
    }

    if (v.err == LERR_BAD_OP) {
      printf("Error: Invalid Operator!");
    }

    if (v.err == LERR_BAD_NUM) {
      printf("Error: Invalid Number!");
    }

    break;
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char *op, lval y) {
  if (x.type == LVAL_ERR) {
    return y;
  }

  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "^") == 0) {
    return lval_num(pow(x.num, y.num));
  }

  if (strcmp(op, "%") == 0) {
    return lval_num((long) x.num % (long) y.num);
  }

  if (strcmp(op, "+") == 0) {
    return lval_num(x.num + y.num);
  }

  if (strcmp(op, "-") == 0) {
    return lval_num(x.num - y.num);
  }

  if (strcmp(op, "*") == 0) {
    return lval_num(x.num * y.num);
  }

  if (strcmp(op, "/") == 0) {
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
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

lval eval(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    errno = 0;

    double x = atof((*t).contents);

    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char *op = t->children[1]->contents;

  lval x = eval(t->children[2]);

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
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispify = mpc_new("lispify");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number   : /-?[0-9]+(\\.?[0-9]+)?/ ;                             \
    operator : '+' | '-' | '*' | '/' | '%' | '^' ;      \
    expr     : <number> | '(' <operator> <expr>+ ')';  \
    lispify    : /^/ <operator> <expr>+ /$/ ;             \
  ",
            Number, Operator, Expr, Lispify);

  puts("Lispify Version 0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispify> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispify, &r)) {
      /* On success print and delete the AST */
      /* mpc_ast_print(r.output); */

      lval result = eval(r.output);

      lval_println(result);

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
