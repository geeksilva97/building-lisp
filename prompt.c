#include <stdio.h>
#include <editline/readline.h>
#include <stdlib.h>
/* #include <editline/history.h> */ // does not exist in mac https://stackoverflow.com/questions/22886475/editline-history-h-and-editline-readline-h-not-found-working-on-macos-when-tryin

// Precisei compilar com: gcc prompt.c -ledit -o prompt
// ledit é a biblioteca que contém a função readline

// Macro para idenfiticar o sistema operacional
// #ifdef _WIN32
// #ifdef __APPLE__
// #ifdef __linux__

int main(int argc, char** argv) {

  puts("Lispify Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char* input = readline("lispify> ");

    add_history(input);

    printf("No you're a %s\n", input);

    free(input);
  }

  return 0;
}
