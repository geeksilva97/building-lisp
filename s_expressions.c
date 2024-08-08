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

typedef struct lval {
  int type;
  double num;

  /* Error and Symbol types have some string data */
  char *err;
  char *sym;

  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval **cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

void lval_expr_print(lval* v, char open, char close);

lval* lval_num(double x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;

  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);

  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);

  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
  case LVAL_NUM:
    break;

  case LVAL_ERR:
    free(v->err);
    break;

  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }

    free(v->cell);
    break;
  }

  free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  double x = atof(t->contents);

  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;

  return v;
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval* x = NULL;

  /* If root (>) or sexpr then create empty list */
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }

  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%f", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}


void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_eval(lval* v);

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];

  // Shift the memory after the item at i over the top of it
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);

  lval_del(v);

  return x;
}


lval* builtin_op(lval* a, char* op) {
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);

      return lval_err("Cannot operate on non-number!");
    }
  }

  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }

    if (strcmp(op, "-") == 0) {
      x->num -= y->num;
    }

    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }

    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);

        x = lval_err("Division By Zero!");

        break;
      }

      x->num /= y->num;
    }

    if (strcmp(op, "%") == 0) {
      x->num = (long) x->num % (long) y->num;
    }

    if (strcmp(op, "^") == 0) {
      x->num = pow(x->num, y->num);
    }

    lval_del(y);
  }

  lval_del(a);

  return x;
}

lval* lval_eval_sexpr(lval* v) {
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->count == 0) {
    return v;
  }

  if (v->count == 1) {
    return lval_take(v, 0);
  }

  lval* f = lval_pop(v, 0);

  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);

    return lval_err("S-expression does not start with symbol");
  }

  lval* result = builtin_op(v, f->sym);

  lval_del(f);

  return result;
}

lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  return v;
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

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *SExpr = mpc_new("sexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispify = mpc_new("lispify");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number   : /-?[0-9]+(\\.?[0-9]+)?/ ;                             \
    symbol : '+' | '-' | '*' | '/' | '%' | '^' ;      \
    sexpr     : '(' <expr>* ')';  \
    expr      : <number> | <symbol> | <sexpr>;  \
    lispify    : /^/ <expr>* /$/ ;             \
  ",
            Number, Symbol, SExpr, Expr, Lispify);

  puts("Lispify Version 0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispify> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispify, &r)) {
      /* On success print and delete the AST */
      /* mpc_ast_print(r.output); */

      lval* x = lval_eval(lval_read(r.output));

      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, SExpr, Expr, Lispify);

  return 0;
}
