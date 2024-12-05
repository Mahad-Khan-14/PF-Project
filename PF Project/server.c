#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // to Link with Winsock library for sockets

#define PORT 8081        // Port number for the chat server
#define MAX_CLIENTS 10   // Maximum number of clients allowed
#define BUFFER_SIZE 1024 // Buffer size for message handling

// Array to store connected client sockets and their names

SOCKET client_sockets[MAX_CLIENTS] = {0};
char client_names[MAX_CLIENTS][50] = {0};

// Mutex for synchronizing access to shared resources
HANDLE lock;
FILE *chat_log; // File pointer for chat log

// Function to handle communication with each client
DWORD WINAPI handle_client(LPVOID socket_desc)
{
    SOCKET sock = *(SOCKET *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;
    int client_index = -1;

    // Find an available slot for the new client
    WaitForSingleObject(lock, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] == 0)
        {
            client_sockets[i] = sock;
            client_index = i;
            break;
        }
    }
    ReleaseMutex(lock);

    // Check if server is full
    if (client_index == -1)
    {
        printf("Server full. Disconnecting client.\n");
        closesocket(sock);
        free(socket_desc);
        return 0;
    }

    // Receive and store client's name

    recv(sock, client_names[client_index], sizeof(client_names[client_index]), 0);
    printf("Client %s connected.\n", client_names[client_index]);

    // Notify other clients that a new client has joined
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, sizeof(join_msg), "%s has joined the chat.", client_names[client_index]);

    WaitForSingleObject(lock, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] != 0 && client_sockets[i] != sock)
        {
            send(client_sockets[i], join_msg, strlen(join_msg), 0);
        }
    }
    fprintf(chat_log, "%s\n", join_msg); // Log msg to file about a new member has joined
    fflush(chat_log);
    ReleaseMutex(lock);

    // Handle messages from the client
    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0'; // Null-terminate the message

        // Handle '/exit' command
        if (strcmp(buffer, "/exit") == 0)
        {
            printf("Client %s is disconnecting.\n", client_names[client_index]);
            break;
        }

        // Broadcast the message to other clients
        char message_with_name[BUFFER_SIZE];
        snprintf(message_with_name, sizeof(message_with_name), "%s: %s", client_names[client_index], buffer);

        WaitForSingleObject(lock, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_sockets[i] != 0 && client_sockets[i] != sock)
            {
                send(client_sockets[i], message_with_name, strlen(message_with_name), 0);
            }
        }
        fprintf(chat_log, "%s\n", message_with_name); // Log msgs/conversation to file
        fflush(chat_log);
        ReleaseMutex(lock);
    }

    // Notify other clients that this client has left
    snprintf(join_msg, sizeof(join_msg), "%s has left the chat.", client_names[client_index]);
    WaitForSingleObject(lock, INFINITE);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_sockets[i] != 0 && client_sockets[i] != sock)
        {
            send(client_sockets[i], join_msg, strlen(join_msg), 0);
        }
    }
    fprintf(chat_log, "%s\n", join_msg); // Log msgs to file that a client has left
    fflush(chat_log);
    ReleaseMutex(lock);

    // Clean up and free resources
    closesocket(sock);
    WaitForSingleObject(lock, INFINITE);
    client_sockets[client_index] = 0;
    memset(client_names[client_index], 0, sizeof(client_names[client_index]));
    ReleaseMutex(lock);

    free(socket_desc);
    return 0;
}

int main()
{
    WSADATA wsa;
    SOCKET server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    int addr_size = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Winsock initialization failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create a mutex for thread synchronization
    lock = CreateMutex(NULL, FALSE, NULL);

    // Open chat log file and initialize
    chat_log = fopen("chat.txt", "w");
    if (!chat_log)
    {
        printf("Error opening chat log file.\n");
        WSACleanup();
        return 1;
    }
    fprintf(chat_log, "=== New Chat Session Started ===\n");
    fflush(chat_log);

    // Create server socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        fclose(chat_log);
        WSACleanup();
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_sock);
        fclose(chat_log);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    listen(server_sock, MAX_CLIENTS);
    printf("Server listening on port %d\n", PORT);

    // Accept incoming connections
    while ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size)) != INVALID_SOCKET)
    {
        printf("New client connected.\n");
        SOCKET *new_sock_ptr = malloc(sizeof(SOCKET));
        *new_sock_ptr = new_sock;
        CreateThread(NULL, 0, handle_client, (void *)new_sock_ptr, 0, NULL);
    }

    // Cleanup resources
    closesocket(server_sock);
    fclose(chat_log);
    CloseHandle(lock);
    WSACleanup();
    return 0;
}
