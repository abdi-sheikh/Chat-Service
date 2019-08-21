#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t user_thread;              // thread managing user input
pthread_t server_thread;

// Worker thread to manage user input
void *user_worker(void *arg){
  while(!simpio->end_of_input){
    simpio_reset(simpio);
    iprintf(simpio, "");            // print prompt
    while(!simpio->line_ready && !simpio->end_of_input){          // read until line is complete
      simpio_get_char(simpio);
    }
    if(simpio->line_ready) {
      int fd = open(client->to_server_fname, O_RDWR);
      if (fd == -1) {perror("failed to open");}
      mesg_t user_temp;
      memset(&user_temp, 0, sizeof(mesg_t));

      user_temp.kind = BL_MESG;
      strcpy(user_temp.name, client->name);
      strcpy(user_temp.body, simpio->buf);
      write(fd, &user_temp, sizeof(mesg_t));
    }
  }
  mesg_t finished;
  memset(&finished, 0, sizeof(mesg_t));

  finished.kind = BL_DEPARTED;
  strcpy(finished.name, client->name);
  write(client->to_server_fd, &finished, sizeof(mesg_t));
  pthread_cancel(server_thread);  // kill the server thread
  return NULL;
}

// Worker thread to listen to the info from the server.
void *background_worker(void *arg){
  while(1) {
  mesg_t msg_temp;
  read(client->to_client_fd, &msg_temp, sizeof(mesg_t));
  if(msg_temp.kind == BL_JOINED) {
    iprintf(simpio,"-- %s JOINED --\n", msg_temp.name);
  }
  else if(msg_temp.kind == BL_DEPARTED) {
    iprintf(simpio,"-- %s DEPARTED --\n", msg_temp.name);
  }
  else if(msg_temp.kind == BL_MESG) {
    iprintf(simpio,"[%s] : %s\n", msg_temp.name,msg_temp.body);
  }
  else if(msg_temp.kind == BL_SHUTDOWN) {
    iprintf(simpio,"!!! server is shutting down !!!\n");
    break;
  }
}
  pthread_cancel(user_thread);
  return NULL;
}

int main(int argc, char *argv[]){
  char prompt[MAXNAME];

  snprintf(prompt, MAXNAME, "%s>> ",argv[2]); // create a prompt string
  simpio_set_prompt(simpio, prompt);         // set the prompt
  simpio_reset(simpio);                      // initialize io
  simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode

  strcpy(client->name, argv[2]);
  sprintf(client->to_client_fname, "%ldto_client.fifo", (long)getpid());
  sprintf(client->to_server_fname, "%ldto_server.fifo", (long)getpid());
  mkfifo(client->to_client_fname, S_IRUSR | S_IWUSR);
  mkfifo(client->to_server_fname, S_IRUSR | S_IWUSR);


  client->to_client_fd = open(client->to_client_fname, O_RDWR);
  client->to_server_fd = open(client->to_server_fname, O_RDWR);
  if(client->to_client_fd == -1) {return -1;}
  if(client->to_server_fd == -1) {return -1; }
  join_t request;
  memset(&request, 0, sizeof(join_t));

  strcpy(request.name, client->name);
  strcpy(request.to_client_fname, client->to_client_fname);
  strcpy(request.to_server_fname, client->to_server_fname);
  char *server_name = strcat(argv[1], ".fifo");
  int fd = open(server_name, O_RDWR);
  if (fd == -1) {return -1;}
  if((write(fd, &request, sizeof(join_t))) == -1) {return -1;}
  if(close(fd) == -1) {return -1;}

  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input
  pthread_create(&server_thread, NULL, background_worker, NULL);
  pthread_join(user_thread, NULL);
  pthread_join(server_thread, NULL);
  simpio_reset_terminal_mode();
  printf("\n");                 // newline just to make returning to the terminal prettier
  return 0;
}
