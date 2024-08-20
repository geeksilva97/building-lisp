#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *title;
  char **messages;
  size_t num_messages;
} channel_t;

void remove_message(channel_t *channel, size_t index) {
  if (index >= channel->num_messages) {
    perror("Error while removing message: Invalid index");
    return exit(EXIT_FAILURE);
  }

  free(channel->messages[index]);

  for (size_t i = index; i < channel->num_messages - 1; i++) {
    channel->messages[i] = channel->messages[i + 1];
  }

  channel->num_messages--;

  if (channel->num_messages == 0) {
    free(channel->messages);
    channel->messages = NULL;
  } else {
    channel->messages =
        realloc(channel->messages, sizeof(char *) * channel->num_messages);
  }
}

void update_message(channel_t *channel, size_t index, char *new_message) {
  if (index >= channel->num_messages) {
    perror("Error while removing message: Invalid index");
    return exit(EXIT_FAILURE);
  }

  free(channel->messages[index]);

  channel->messages[index] = malloc(strlen(new_message) + 1);

  strcpy(channel->messages[index], new_message);
}

void add_message(channel_t *channel, char *message) {
  printf("Adding message to the channel %s\n"
         "There are %lu messages at this moment.\n",
         channel->title, channel->num_messages);

  channel->num_messages++;
  channel->messages =
      realloc(channel->messages, sizeof(char *) * channel->num_messages);
  channel->messages[channel->num_messages - 1] = malloc(strlen(message) + 1);

  strcpy(channel->messages[channel->num_messages - 1], message);
}

channel_t *create_channel(char *channel_title) {
  channel_t *channel = malloc(sizeof(channel_t));
  /* Até é possivel setart a string sem o malloc, mas mudar depois falha
      Tomei um bus error */
  channel->title = malloc(strlen(channel_title) + 1);
  channel->title = strcpy(channel->title, channel_title);

  return channel;
}

void print_size_struct(channel_t *channel) {
  size_t s = sizeof(*(channel));

  printf("Struct size: %lu\n", s);

  printf("Size of title: %lu\n"
         "Size of num_messages: %lu\n"
         "Size of messages: %lu\n",
         sizeof(channel->title), sizeof(channel->messages),
         sizeof(channel->num_messages));
}

void print_channel(channel_t *channel) {
  printf("===== Printing channel =======\n"
         "[Title]: %s\n"
         "[N messages]: %lu\n",
         channel->title, channel->num_messages);

  for (int i = 0; i < channel->num_messages; i++) {
    printf("Message %i\n%s\n\n", i + 1, channel->messages[i]);
  }
}

channel_t *clone2(channel_t *channel) {
  size_t size = sizeof(*channel);

  channel_t *new_channel = malloc(size);

  memcpy(new_channel, channel, size);

  return new_channel;
}

size_t my_str_len(char *str) {
  size_t n = 0;

  while (*str) {
    printf("Contando char '%c'\n", *str);
    str++;
    n++;
  }

  return n;
}

channel_t *clone(channel_t *channel) {
  printf("Cloning chanel '%s'\n\n", channel->title);

  channel_t *new_channel = malloc(sizeof(channel_t));

  new_channel->num_messages = channel->num_messages;
  new_channel->title = malloc(strlen(channel->title) + 1);
  new_channel->messages = malloc(sizeof(char *) * channel->num_messages);

  stpcpy(new_channel->title, channel->title);

  int mem = 0;

  for (int i = 0; i < channel->num_messages; i++) {
    char *message = channel->messages[i];

    mem += strlen(message) + 1;
  }

  char **messages = malloc(sizeof(char *) * mem);

  for (int i = 0; i < channel->num_messages; i++) {
    char *message = channel->messages[i];

    strcpy(messages[i], message);
  }

  memcpy(new_channel->messages, messages, mem);

  /* for (int i = 0; i < channel->num_messages; i++) { */
  /*   char *message = channel->messages[i]; */
  /*   new_channel->messages[i] = malloc(strlen(message) + 1); */
  /*   strcpy(new_channel->messages[i], message); */
  /* } */

  return new_channel;
}

int compare(channel_t *channel_a, channel_t *channel_b) {
  printf("Comparing titles '%s' to '%s'\n\n", channel_a->title,
         channel_b->title);

  printf("Comparing total of messages '%lu' to '%lu'\n\n",
         channel_a->num_messages, channel_b->num_messages);
  if (channel_a->num_messages == channel_b->num_messages &&
      strcmp(channel_a->title, channel_b->title) == 0) {

    return 1;
  }

  return 0;
}

void free_channel(channel_t *channel) {
  free(channel->title);
  free(channel->messages);
  free(channel);
}

channel_t *copy_channel_with_memcpy(char *channel) {
  channel_t *new_channel = create_channel("Gotta copy this channel");

  return new_channel;
}

int main(void) {
  channel_t *channel = create_channel("This is my channel");

  add_message(channel, "A single message");

  print_channel(channel);

  remove_message(channel, 0);

  channel->title[0] = 'K';

  /* channel_t *cloned_channel = clone2(channel); */
  channel_t *cloned_channel = clone(channel);

  printf("Original title address: %u\n", *channel->title);
  printf("Cloned title address: %u\n", *cloned_channel->title);

  char *new_title = "Cloned channel";

  cloned_channel->title = realloc(cloned_channel->title, strlen(new_title) + 1);
  strcpy(cloned_channel->title, new_title);

  print_channel(channel);

  print_channel(cloned_channel);

  printf("Are channels equal? %s\n\n",
         compare(channel, cloned_channel) ? "True" : "False");

  free_channel(channel);
  free_channel(cloned_channel);

  size_t string_size = my_str_len("Edy\n");

  printf("A string tem tamanho %lu\n", string_size);

  return 0;
}
