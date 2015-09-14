#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void process_conn_server(int s)
{
  ssize_t size = 0;
  char buffer[1024];
  while(1) {
    size = read(s, buffer, 1024);
    if (size == 0) {
      return;
    }
    sprintf(buffer, "%d bytes altogether\n", (int)size);
    write(s, buffer, strlen(buffer)+1);
  }
}

void process_conn_client(int s)
{
  ssize_t size = 0;
  char buffer[1024];

  while(1) {
    size = read(0, buffer, 1024);
    if (size > 0) {
      write(s, buffer, size);
      size = read(s, buffer, 1024);
      write(1, buffer, size);
    }
  }
}
