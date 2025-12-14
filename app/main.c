#include "server.h"


int main(){
    init_clients();
    start_server("unix_socket");

}