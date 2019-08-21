#include "blather.h"
int signaled = 0;
server_t server_actual;
server_t *server = &server_actual;
void handle_signals(int signo){                                      // handler for some signals
//  char *msg = "signaled, setting flag\n";                 // print a message about the signal
//  write(STDERR_FILENO,msg,strlen(msg));                              // avoid fprintf() as it is not reentrant
  signaled = 1;                                                      // set global variable to indicate signal received
  return;
  }

int main(int argc, char*argv[]) {
  struct sigaction my_sa = {};                                       // portable signal handling setup with sigaction()
  my_sa.sa_handler = handle_signals;                                 // run function handle_signals
  sigemptyset(&my_sa.sa_mask);                                       // don't block any other signals during handling
  my_sa.sa_flags = SA_RESTART;                                       // always restart system calls on signals possible
  sigaction(SIGTERM, &my_sa, NULL);                                  // register SIGTERM with given action
  sigaction(SIGINT,  &my_sa, NULL);
  server_start(server, argv[1], DEFAULT_PERMS);
  while (!signaled) {
    server_check_sources(server);
    if(server_join_ready(server) == 1) {
      server_handle_join(server);
    }
    for (int i = 0; i < server->n_clients; i++) {
      if (server_client_ready(server, i) == 1) {
        server_handle_client(server, i);
        }
      }// put this so only one client is checked - remove it if you wanna keep testing
  }
  server_shutdown(server);
  return 0;
  } // end main
