#ifndef network_h_INCLUDED
#define network_h_INCLUDED

typedef struct { 
	char bytes[16]; 
} NetworkAddress;

typedef enum {
	NETWORK_SOCKET_CLIENT,
	NETWORK_SOCKET_SERVER
} NetworkSocketType;

typedef struct {
	NetworkSocketType type;
	void* backend;
} NetworkSocket;

typedef struct Packet {
	struct Packet* next;
	u32 size;
	void* data;
} NetworkPacket;

NetworkAddress network_generate_address(char* ipv4_string, i32 port);
void network_init_server_socket(NetworkSocket* sock, StackAllocator* stack);
void network_init_client_socket(NetworkSocket* sock, StackAllocator* stack);
void network_close_socket(NetworkSocket* sock);
void network_send_packet(NetworkSocket* sock, NetworkAddress* address, void* packet, u32 size);
NetworkPacket* network_poll_packets(NetworkSocket* sock, StackAllocator* stack);

#endif // network_h_INCLUDED
