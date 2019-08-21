#include "blather.h"
client_t *server_get_client(server_t *server, int idx){
  if(idx <= server->n_clients) {
    return &server->client[idx];  // pointer to specified index
  }
  return NULL;
}

void server_start(server_t *server, char *server_name, int perms) {
  server->join_ready = 0;
  server->n_clients = 0;
  char *temp = strcat(server_name, ".fifo");
  strcpy(server->server_name, server_name);
  while (access(temp, F_OK) != -1) {
    remove(temp);
    }
  if (mkfifo(temp, perms) != -1) {
    server->join_fd = open(temp, O_RDWR);
    }
}  // end server_start

void server_shutdown(server_t *server) {
  if (close(server->join_fd) == -1) {return perror ("failed to close ");}
  remove(server->server_name);
  mesg_kind_t msg_kind = BL_SHUTDOWN;
  mesg_t end_msg;
  memset(&end_msg, 0, sizeof(mesg_t));
  end_msg.kind = msg_kind;
  // broadcast the shutdown message to all clients
  server_broadcast(server, &end_msg);
  for (int i = 0; i < server->n_clients; i++) {
    server_remove_client(server, i);
  }
}

int server_add_client(server_t *server, join_t *join) {
  if(server->n_clients != MAXCLIENTS) {
    strncpy(server->client[server->n_clients].name, join->name, MAXNAME);
    strncpy(server->client[server->n_clients].to_server_fname,
      join->to_server_fname, MAXNAME);
    strncpy(server->client[server->n_clients].to_client_fname,
      join->to_client_fname, MAXNAME);
    server->client[server->n_clients].data_ready = 0;
    server->client[server->n_clients].to_server_fd =
      open(server->client[server->n_clients].to_server_fname, O_RDWR);
    if(server->client[server->n_clients].to_server_fd == -1) {
      return -1; }
    server->client[server->n_clients].to_client_fd =
      open(server->client[server->n_clients].to_client_fname, O_RDWR);
    if(server->client[server->n_clients].to_client_fd == -1) {
      return -1; }
    server->n_clients++;
    return 0;
    }
  // max clients exceeded
  return -1;
}
int server_remove_client(server_t *server, int idx) {
  if(idx <= server->n_clients) {
    client_t *c_remove = server_get_client(server, idx);
    if(close(c_remove->to_client_fd) == -1) {return -1;}
    if(close(c_remove->to_server_fd) == -1) {return -1;}
    for(int i = idx; i < server->n_clients; i++) {
      server->client[i] = server->client[i+1];
    }
    server->n_clients--;
    return 0;
  }
  return -1;
  }

int server_broadcast(server_t *server, mesg_t *mesg) {
  for (int i = 0; i<server->n_clients; i++) {
    if (write(server->client[i].to_client_fd, mesg, sizeof(mesg_t)) == -1) {
      return -1;
      }
    }
  return 0;
  }

void server_check_sources(server_t *server) {
  int max_fd = server->join_fd;
  fd_set read_set;
  FD_ZERO(&read_set);
  FD_SET(server->join_fd, &read_set);
  for (int i = 0; i<server->n_clients; i++) {
    FD_SET(server->client[i].to_server_fd, &read_set);
    if(max_fd < server->client[i].to_server_fd) {
      max_fd = server->client[i].to_server_fd;
      }
    }
  int ret = select(max_fd + 1, &read_set, NULL, NULL, NULL);  // sleep, wake if clients
  if(ret != -1) {
    if(FD_ISSET(server->join_fd, &read_set)) {
      server->join_ready = 1;
      }
    for (int i = 0; i<server->n_clients; i++) {       // have anything
      if(FD_ISSET(server->client[i].to_server_fd,
        &read_set)) {
        server->client[i].data_ready = 1;
        }
    }
  }
}
int server_join_ready(server_t *server) {
  return server->join_ready;
}

int server_handle_join(server_t *server) {
  join_t temp_join;
  int n_bytes = read(server->join_fd, &temp_join, sizeof(join_t));
  if(n_bytes == -1) {return -1;}
  server_add_client(server, &temp_join);
  mesg_t joined;
  memset(&joined, 0, sizeof(mesg_t));
  mesg_kind_t e_kind = BL_JOINED;
  joined.kind = e_kind;
  strncpy(joined.name, temp_join.name, sizeof(mesg_t));
  server->join_ready = 0;
  server_broadcast(server, &joined);
  return 0;
}
int server_client_ready(server_t *server, int idx) {
  return server->client[idx].data_ready;
}

int server_handle_client(server_t *server, int idx) {
  mesg_t msg_temp;
  if(read(server->client[idx].to_server_fd,
    &msg_temp, sizeof(mesg_t)) == -1) {return -1;}
  if(msg_temp.kind == BL_MESG) {
    server_broadcast(server, &msg_temp);
  } else if (msg_temp.kind == BL_JOINED) {
    server_broadcast(server, &msg_temp);
  } else if (msg_temp.kind == BL_DEPARTED) {
    server_remove_client(server, idx);
    server_broadcast(server, &msg_temp);
  } else if (msg_temp.kind == BL_SHUTDOWN) {
  // Broadcast when the server starts shutting down
      server_broadcast(server, &msg_temp);
  } else {
    return -1;
  }
  server->client[idx].data_ready = 0;
  return 0;
}
void server_tick(server_t *server) {
  server->time_sec++;
}
