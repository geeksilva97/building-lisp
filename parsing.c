#include "mpc.h"
#include <stdio.h>

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

int main(int argc, char **argv) {
  /* Create Some Parsers */
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Lispify = mpc_new("lispify");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
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
      mpc_ast_t *a = r.output;

      printf("Tag: %s\n", a->tag);
      printf("Contents: %s\n", a->contents);
      printf("Number of children: %i\n", a->children_num);

      /* Get First Child */
      mpc_ast_t* c0 = a->children[0];
      printf("First Child Tag: %s\n", c0->tag);
      printf("First Child Contents: %s\n", c0->contents);
      printf("First Child Number of children: %i\n",
  c0->children_num);

      /* On success print and delete the AST */
      mpc_ast_print(r.output);
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
