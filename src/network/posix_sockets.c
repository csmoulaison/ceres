#include "network/network.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct {
	i32 descriptor;
} PosixSocket;

NetworkAddress network_generate_address(char* ipv4_string, i32 port) {
	struct sockaddr_in address;
	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if(ipv4_string == NULL) {
		address.sin_addr.s_addr = INADDR_ANY;
	} else {
		assert(inet_pton(AF_INET, ipv4_string, &address.sin_addr) > 0);
	}
	return *((NetworkAddress*)(&address));
}

void network_init_server_socket(NetworkSocket* sock, StackAllocator* stack) {
	sock->type = NETWORK_SOCKET_SERVER;
	sock->backend = stack_alloc(stack, sizeof(PosixSocket));
	PosixSocket* posix = (PosixSocket*)sock->backend;

	assert((posix->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) >= 0);
	// TODO: If we didn't set the socket to reusable, we would have to wait a few
	// minutes when reconfiguring sockets. But, this also means when we poll for
	// received packets on a reused socket, we can receive packets which were meant
	// for the previous socket.
	// 
	// TLDR: We should account for packets meant for old versions of reused sockets,
	// or else we could obviously run into trouble.
	i32 reusable = 1;
	setsockopt(posix->descriptor, SOL_SOCKET, SO_REUSEADDR, &reusable, sizeof(&reusable));

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(struct sockaddr_in));
	server_address.sin_family = AF_INET;
	// TODO: User configurable server address/port.
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(8080);

	assert(bind(posix->descriptor, (struct sockaddr*)&server_address, sizeof(server_address)) >=  0);
}

void network_init_client_socket(NetworkSocket* sock, StackAllocator* stack) {
	sock->type = NETWORK_SOCKET_CLIENT;
	sock->backend = stack_alloc(stack, sizeof(PosixSocket));
	PosixSocket* posix = (PosixSocket*)sock->backend;
	assert((posix->descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) >= 0);
}

void network_close_socket(NetworkSocket* sock) {
	close(((PosixSocket*)&sock->backend)->descriptor);
}

void network_send_packet(NetworkSocket* sock, NetworkAddress* address, void* packet, u32 size) {
	// NOW: Implement packet sending
	PosixSocket* posix = (PosixSocket*)sock->backend;
	sendto(posix->descriptor, packet, size, MSG_CONFIRM, (struct sockaddr*)address, sizeof(struct sockaddr_in));
}

NetworkPacket* network_poll_packets(NetworkSocket* sock, StackAllocator* stack) {
	// NOW: Implement packet receiving
	return NULL;
}

