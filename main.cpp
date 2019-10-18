#include <iostream>
#include <vector>
#include <fstream>
#include <time.h>
#include <queue>
#include <set>
#include <random>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

using namespace std;

// This struct provides information sharing between client and server thread
struct Pipe {
    queue<int> clientToServerMessages;  // Stores the messages from client to server thread
    queue<bool> serverToClientMessages;  // Stores the messages from server to client thread
};

// This struct is for storing information of each Client
struct Client {
    int sleepTime;
    int clientId;
    int reservedSeatNo;
    int pipeId;
};

int clientSize;  // Number of clients
ofstream outputTextFile;  // Output file
set<int> availableSeats;  // Ids of the available seats are stored in this set
vector<Pipe> pipes;  // Vector to store the pipes between client and server
vector<Client> clients;  // Vector to store clients' information
vector<sem_t> sem;  // Semaphore Vector - two semaphores for each client-server pair
pthread_mutex_t reservationMutex;  // Mutex to be used when selecting the seats
pthread_mutex_t writeMutex;  // Mutex to be used when writing to the output file

// Client thread function. This function selects a seat from available seats and informs server about it.
void *ClientFun(void *data) {
    Client *client = (Client *) data;  // Data of the Client
    usleep(client->sleepTime);  // Sleep
    Pipe *p = &pipes[client->pipeId];  // Pipe between this thread and its server thread
    srand(time(NULL));  // Seed the rand function
    pthread_mutex_lock(&reservationMutex); // lock mutex
    // Select a random seat from available seats set and remove it from the set so that no other thread can request it
    int size = availableSeats.size();
    set<int>::iterator it = availableSeats.begin();
    advance(it, rand() % size);
    int seatNo = *it;
    availableSeats.erase(it);
    pthread_mutex_unlock(&reservationMutex);  // Unlock mutex
    // Send a message to server to inform it about the reservation
    p->clientToServerMessages.push(seatNo);
    // Signal the first semaphore of this server-client pair so that server can continue processing
    sem_post(&sem[client->clientId - 1]);
    // Wait for the second semaphore of this server-client pair so that client can continue processing
    sem_wait(&sem[client->clientId - 1 + clientSize]);
    // If the reservation is successful pop the message.
    if (p->serverToClientMessages.front()) {
        p->serverToClientMessages.pop();
    }
    pthread_exit(NULL);
}

// Server thread function. This function confirms reservation and prints the output to the output file.
void *ServerFun(void *data) {
    Client *client = (Client *) data;  // Data of the Client
    Pipe *p = &pipes[client->pipeId];  // Pipe between this thread and its client thread
    //  Wait for the first semaphore of this server-client pair so that this(server) can continue processing
    sem_wait(&sem[client->clientId - 1]);
    int seatNo = p->clientToServerMessages.front(); // The seatNo that client requested
    p->clientToServerMessages.pop();
    client->reservedSeatNo = seatNo;
    pthread_mutex_lock(&writeMutex); // Lock write mutex
    outputTextFile << "Client" << client->clientId << " reserves Seat" << client->reservedSeatNo << endl;
    pthread_mutex_unlock(&writeMutex); // Unlock write mutex
    p->serverToClientMessages.push(true); // Reservation is successful
    //  Signal the second semaphore of this server-client pair so that client can continue processing
    sem_post(&sem[client->clientId - 1 + clientSize]);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Check if the arguments are given correctly
    if (argc < 2) {
        cout << "You need to give an argument" << endl;
        cout << "e.g. ./flight_simulation [seat_number]" << endl;
        return 0;
    }
    clientSize = atoi(argv[1]);
    reservationMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;

    // Initialize the semaphores - two semaphore for each client-server pair
    for (int i = 0; i < 2 * clientSize; i++) {
        sem_t s;
        sem_init(&s, 0, 0);
        sem.push_back(s);
    }
    outputTextFile.open("output.txt"); // Output text file
    outputTextFile << "Number of total seats: " << clientSize << endl;
    // Add available seats
    for (int i = 0; i < clientSize; i++) {
        availableSeats.insert(i + 1);
    }
    pthread_t clientThreads[clientSize];  // Client Threads
    pthread_t serverThreads[clientSize];  // Server Threads
    srand(time(NULL)); // Seed the rand function
    // Create client information and pipes
    for (int i = 0; i < clientSize; i++) {
        Client data;
        data.clientId = i + 1;
        data.sleepTime = (rand() % 151 + 50) * 1000;
        data.reservedSeatNo = -1;
        data.pipeId = i;
        clients.push_back(data);
        Pipe pipe;
        pipes.push_back(pipe);
    }
    // Create server and client threads. Client information is passed as parameter
    for (int i = 0; i < clientSize; i++) {
        pthread_create(&clientThreads[i], NULL, &ClientFun, (void *) &clients[i]);
        pthread_create(&serverThreads[i], NULL, &ServerFun, (void *) &clients[i]);
    }
    // Join server threads
    for (int i = 0; i < clientSize; i++) {
        pthread_join(serverThreads[i], NULL);
    }
    outputTextFile << "All seats are reserved." << endl;
    outputTextFile.close();
    return 0;
}