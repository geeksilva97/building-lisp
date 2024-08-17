#include "mpc.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LASSERT(args, cond, fmt, ...)                                          \
  if (!(cond)) {                                                               \
    lval *err = lval_err(fmt, ##__VA_ARGS__);                                  \
    lval_del(args);                                                            \
    return err;                                                                \
  }

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

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef lval *(*lbuiltin)(lenv *, lval *);

void lval_del(lval *v);

struct lenv {
  int count;
  char **syms;
  lval **vals;
};

typedef struct lval {
  int type;
  double num;

  /* Error and Symbol types have some string data */
  char *err;
  char *sym;
  lbuiltin fun;

  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval **cell;
} lval;

void lval_expr_print(lval *v, char open, char close);

lval *lval_copy(lval *v);

lval *builtin(lenv *e, lval *a, char *func);

lval *lval_err(char *fmt, ...);

char *ltype_name(int t) {
  switch (t) {
  case LVAL_FUN:
    return "Function";
  case LVAL_NUM:
    return "Number";
  case LVAL_ERR:
    return "Error";
  case LVAL_SYM:
    return "Symbol";
  case LVAL_SEXPR:
    return "S-Expression";
  case LVAL_QEXPR:
    return "Q-Expression";
  default:
    return "Unknown";
  }
}

/* Environment */

lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));

  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;

  return e;
}

void lenv_del(lenv *env) {
  for (int i = 0; i < env->count; i++) {
    free(env->syms[i]);
    lval_del(env->vals[i]);
  }

  free(env->syms);
  free(env->vals);
  free(env);
}

lval *lenv_get(lenv *e, lval *k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }

  return lval_err("Unbound Symbol '%s'", k->sym);
}

void lenv_put(lenv *env, lval *key, lval *value) {
  for (int i = 0; i < env->count; i++) {
    if (strcmp(env->syms[i], key->sym) == 0) {
      lval_del(env->vals[i]);

      env->vals[i] = lval_copy(value);
    }
  }

  env->count++;
  env->vals = realloc(env->vals, sizeof(lval *) * env->count);
  env->syms = realloc(env->syms, sizeof(char *) * env->count);

  env->vals[env->count - 1] = lval_copy(value);
  env->syms[env->count - 1] = malloc(strlen(key->sym) + 1);

  strcpy(env->syms[env->count - 1], key->sym);
}

/* Environment */

lval *lval_fun(lbuiltin func) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;

  return v;
}

lval *lval_num(double x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;

  return v;
}

lval *lval_err(char *fmt, ...) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  v->err = malloc(512);

  vsnprintf(v->err, 512, fmt, va);

  v->err = realloc(v->err, strlen(v->err) + 1);

  va_end(va);

  return v;
}

lval *lval_sexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval *lval_qexpr(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;

  return v;
}

lval *lval_sym(char *s) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);

  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_FUN:
  case LVAL_NUM:
    break;

  case LVAL_ERR:
    free(v->err);
    break;

  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }

    free(v->cell);
    break;
  }

  free(v);
}

lval *lval_read_num(mpc_ast_t *t) {
  errno = 0;
  double x = atof(t->contents);

  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_add(lval *v, lval *x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval *) * v->count);
  v->cell[v->count - 1] = x;

  return v;
}

lval *lval_read(mpc_ast_t *t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval *x = NULL;

  /* If root (>) or sexpr then create empty list */
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }

  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  if (strstr(t->tag, "qexpr")) {
    x = lval_qexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->contents, ")") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->contents, "{") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->contents, "}") == 0) {
      continue;
    }

    if (strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval *lval_copy(lval *v) {
  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
  case LVAL_FUN:
    x->fun = v->fun;
    break;

  case LVAL_NUM:
    x->num = v->num;
    break;

  case LVAL_ERR:
    x->err = malloc(strlen(v->err) + 1);
    strcpy(x->err, v->err);
    break;

  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym) + 1);
    strcpy(x->sym, v->sym);
    break;

  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval *) * x->count);

    for (int i = 0; i < x->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }

    break;
  }

  return x;
}

void lval_print(lval *v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%f", v->num);
    break;
  case LVAL_ERR:
    printf("Error: %s", v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_FUN:
    printf("<function>");
    break;
  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
  }
}

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);

  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_println(lval *v) {
  lval_print(v);
  putchar('\n');
}

lval *lval_eval(lenv *env, lval *v);

lval *lval_pop(lval *v, int i) {
  lval *x = v->cell[i];

  // Shift the memory after the item at i over the top of it
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval *) * v->count);

  return x;
}

lval *lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);

  lval_del(v);

  return x;
}

lval *builtin_op(lenv *env, lval *a, char *op) {
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval *err =
          lval_err("Function '%s' passed incorrect type for argument %i. "
                   "Got %s, Expected %s",
                   op, i, ltype_name(a->cell[i]->type), ltype_name(LVAL_NUM));
      lval_del(a);

      return err;
    }
  }

  lval *x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval *y = lval_pop(a, 0);

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
      x->num = (long)x->num % (long)y->num;
    }

    if (strcmp(op, "^") == 0) {
      x->num = pow(x->num, y->num);
    }

    lval_del(y);
  }

  lval_del(a);

  return x;
}

lval *builtin_add(lenv *env, lval *a) { return builtin_op(env, a, "+"); }

lval *builtin_sub(lenv *env, lval *a) { return builtin_op(env, a, "-"); }

lval *builtin_mul(lenv *env, lval *a) { return builtin_op(env, a, "*"); }

lval *builtin_div(lenv *env, lval *a) { return builtin_op(env, a, "/"); }

void lenv_add_builtin(lenv *env, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);
  lenv_put(env, k, v);
  lval_del(k);
  lval_del(v);
}

lval *lval_eval_sexpr(lenv *e, lval *v) {
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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

  lval *f = lval_pop(v, 0);

  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v);

    return lval_err("First element is not a function!");
  }

  lval *result = f->fun(e, v);
  lval_del(f);

  return result;
}

lval *lval_eval(lenv *env, lval *v) {
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(env, v);
    lval_del(v);

    return x;
  }

  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(env, v);
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

lval *builtin_len(lenv *env, lval *a) {
  LASSERT(a, a->count == 1, "Function 'len' expects only one argument!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'len' passed incorrect type. A Q-Expression is expected!");

  lval *v = lval_num(a->cell[0]->count);

  lval_del(a);

  return v;
}

lval *builtin_def(lenv *e, lval *a) {
  // def {a b} 1 2
  // a->cell[0] -> {a b}
  // a->cell[1] -> 1
  // a->cell[2] -> 2
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'def' passed incorrect type!");

  // first is the symbol list (in a Q-Expression)
  lval *syms = a->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM,
            "FUNCTION 'def' cannot define non-symbol");
  }

  LASSERT(a, syms->count == a->count - 1,
          "FUNCTION 'def' cannot define incorrect number of values to symbols");

  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i + 1]);
  }

  lval_del(a);

  return lval_sexpr();
}

lval *builtin_head(lenv *env, lval *a) {
  LASSERT(a, a->count == 1,
          "Function 'head' passed too many arguments!"
          "Got %i, Expected %i",
          a->count, 1);
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'head' passed incorrect type for argument 0"
          "Got %s, Expected %s",
          ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!");

  lval *v = lval_take(a, 0);

  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval *builtin_tail(lenv *env, lval *a) {
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'tail' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");

  lval *v = lval_take(a, 0);

  lval_del(lval_pop(v, 0));

  return v;
}

lval *builtin_list(lenv *env, lval *a) {
  a->type = LVAL_QEXPR;

  return a;
}

lval *builtin_eval(lenv *env, lval *a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
          "Function 'eval' passed incorrect type!");

  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(env, x);
}

lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);

  return x;
}

lval *builtin_join(lenv *env, lval *a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
            "Function 'join' passed incorrect type!");
  }

  lval *x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);

  return x;
}

void lenv_add_builtins(lenv *e) {
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);

  /**/
  lenv_add_builtin(e, "def", builtin_def);
}

lval *builtin(lenv *e, lval *a, char *func) {
  if (strcmp("len", func) == 0) {
    return builtin_len(e, a);
  }

  if (strcmp("list", func) == 0) {
    return builtin_list(e, a);
  }

  if (strcmp("head", func) == 0) {
    return builtin_head(e, a);
  }

  if (strcmp("tail", func) == 0) {
    return builtin_tail(e, a);
  }

  if (strcmp("eval", func) == 0) {
    return builtin_eval(e, a);
  }

  if (strcmp("join", func) == 0) {
    return builtin_join(e, a);
  }

  if (strstr("+-/*%^", func)) {
    return builtin_op(e, a, func);
  }

  lval_del(a);

  return lval_err("Unknown Function!");
}

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol = mpc_new("symbol");
  mpc_parser_t *SExpr = mpc_new("sexpr");
  mpc_parser_t *QExpr = mpc_new("qexpr");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispify = mpc_new("lispify");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                     \
    number     : /-?[0-9]+(\\.?[0-9]+)?/ ;                             \
    symbol     :  /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;      \
    sexpr      : '(' <expr>* ')';  \
    qexpr      : '{' <expr>* '}';  \
    expr       : <number> | <symbol> | <sexpr> | <qexpr>;  \
    lispify    : /^/ <expr>* /$/ ;             \
  ",
            Number, Symbol, SExpr, QExpr, Expr, Lispify);

  puts("Lispify Version 0.0.5");
  puts("Press Ctrl+c to Exit\n");

  lenv *e = lenv_new();
  lenv_add_builtins(e);

  while (1) {
    char *input = readline("lispify> ");

    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispify, &r)) {
      /* On success print and delete the AST */
      /* mpc_ast_print(r.output); */

      lval *x = lval_eval(e, lval_read(r.output));

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

  mpc_cleanup(6, Number, Symbol, SExpr, QExpr, Expr, Lispify);

  return 0;
}
