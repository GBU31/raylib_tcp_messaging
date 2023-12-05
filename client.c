#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "raylib.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8000
#define BUFFER_SIZE 1024
#define MAX_INPUT_CHARS 1024

int client_socket;
char buffer[BUFFER_SIZE] = {0};
int connected = 1;
char inputText[MAX_INPUT_CHARS + 1] = "\0";
char msg[BUFFER_SIZE] = "";

pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* arg) {
    struct sockaddr_in server_address;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (connected) {
        memset(buffer, 0, sizeof(buffer));
        if (read(client_socket, buffer, BUFFER_SIZE) <= 0) {
            perror("Server disconnected");
            connected = 0;
        }

        pthread_mutex_lock(&inputMutex);
        strcat(msg, buffer);
        pthread_mutex_unlock(&inputMutex);

        printf("Server message: %s\n", buffer);
    }

    close(client_socket);
    return NULL;
}

void* raylibThread(void* arg) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, SERVER_IP);
    int framesCounter = 0;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        framesCounter++;

        if (IsKeyPressed(KEY_BACKSPACE) && (strlen(inputText) > 0)) {
            inputText[strlen(inputText) - 1] = '\0';
        } else if (IsKeyPressed(KEY_ENTER)) {
            pthread_mutex_lock(&inputMutex);
            const char* client_message = inputText;
            if (strcmp(inputText, "clear") == 0) {
                memset(msg, 0, sizeof(msg));
            }
            strcat(inputText, "\n");
            strcat(msg, inputText);
            send(client_socket, client_message, strlen(client_message), 0);
            pthread_mutex_unlock(&inputMutex);

            printf("Entered: %s\n", inputText);
            memset(inputText, 0, sizeof(inputText));
        } else {
            int key = GetKeyPressed();
            if ((key >= 32) && (key <= 125) && (strlen(inputText) < MAX_INPUT_CHARS)) {
                pthread_mutex_lock(&inputMutex);
                if (key >= 'A' && key <= 'Z') {
                    key = tolower(key);
                }
                inputText[strlen(inputText)] = (char)key;
                pthread_mutex_unlock(&inputMutex);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        pthread_mutex_lock(&inputMutex);
        DrawText(msg, (screenWidth - MeasureText(msg, 20)) / 2, (screenHeight - 20) / 2, 20, MAROON);
        pthread_mutex_unlock(&inputMutex);
        DrawRectangle(0, screenHeight - 40, screenWidth, 40, LIGHTGRAY);
        DrawRectangleLines(0, screenHeight - 40, screenWidth, 40, GRAY);
        DrawText(inputText, 10, screenHeight - 30, 20, MAROON);

        EndDrawing();
    }

    CloseWindow();

    return NULL;
}

int main() {
    pthread_t clientThreadId, raylibThreadId;

    if (pthread_create(&clientThreadId, NULL, clientThread, NULL) != 0) {
        perror("Failed to create client thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&raylibThreadId, NULL, raylibThread, NULL) != 0) {
        perror("Failed to create Raylib thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(clientThreadId, NULL);

    return 0;
}
