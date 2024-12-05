#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib") // Link with Winsock library

#define PORT 8081        // Port number for connecting to the chat server
#define BUFFER_SIZE 1024 // Size of the message buffer

// Function to handle receiving messages from the server
DWORD WINAPI receive_messages(LPVOID socket_desc)
{
    SOCKET sock = *(SOCKET *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Continuously receive messages from the server
    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0'; // Null-terminate the received message

        // Display system notifications (e.g., user join/leave messages) in yellow
        if (strstr(buffer, "has joined the chat") || strstr(buffer, "has left the chat"))
        {
            printf("\r\033[1;33m%s\033[0m\n", buffer); // Yellow color
        }
        else // Display regular chat messages in green
        {
            printf("\r\033[1;32m%s\033[0m\n", buffer); // Green color
        }
    }

    // Handle connection closure
    if (read_size == 0)
    {
        printf("\nServer closed the connection. Exiting...\n");
    }
    else if (read_size == SOCKET_ERROR)
    {
        printf("\nConnection lost. Error Code: %d\n", WSAGetLastError());
    }

    closesocket(sock); // Close the socket
    ExitThread(0);     // End the thread
}

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    HANDLE recv_thread;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Winsock initialization failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create a socket for connecting to the server
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost (change to server IP if needed)

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    // Prompt the user for their name and send it to the server
    printf("Enter your name: ");
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = '\0'; // Remove newline character
    send(sock, message, strlen(message), 0);

    printf("Connected to server. Start chatting! Type '/exit' to quit.\n");

    // Create a separate thread to handle incoming messages
    recv_thread = CreateThread(NULL, 0, receive_messages, (void *)&sock, 0, NULL);

    // Loop for sending messages to the server
    while (1)
    {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0'; // Remove newline character

        // Handle exit command
        if (strcmp(message, "/exit") == 0)
        {
            send(sock, message, strlen(message), 0);
            break;
        }

        send(sock, message, strlen(message), 0); // Send the message to the server
    }

    // Cleanup and close the socket
    closesocket(sock);
    WSACleanup(); // Clean up Winsock resources
    return 0;
}
