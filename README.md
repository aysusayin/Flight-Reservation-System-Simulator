# Flight Reservation System Simulator
##### CMPE 322 - FALL 2018

The project is to implement a flight reservation system simulator using POSIX threads.  


## Design
We need to create server and client threads. Basically, the client threads select a seat from available seats and server confirms the client’s reservation request and prints the output to the output file. Simulation with multithreading can raise critical section problems since many threads are reading and writing the same data.  There are some structures that I used in my code:
 - Client: This structure mainly used for sharing the data of the client with the server and client threads. It includes clientId, sleepTime for the client (selected randomly in main), pipeId (The id of their pipe in the pipes vector) and  reservedSeatNo.
 - Pipe: Pipe provides data sharing between server and client threads. There are two queues in the struct. One of them is for passing messages from client to server and the other is for server to client.
 - Available Seats set: This set is consist of the seat id’s of the available seats.
 - Mutexes: There are two different mutex. One is for making the reservation. Since client threads read and modify availableSeats set, this is required. The other is called writeMutex which provides mutual exclusion to the servers when they want to write to the output  file.
 - Semaphores: Since some parts of the client and server threads’ function need to execute sequentially , semaphore is needed. There are two semaphores for each pair. Both of them is initialized to zero at first. When client selects a seat signals the first semaphore. Then the server continues execution. Meanwhile client waits for the second semaphore. This is signaled after server confirms the reservation and writes to the output file.  


Let’s walk through the code. Main function starts execution. First the required structures and data are initialized(Mutexes, semaphore vector, available seats set, pipe and client vector). Then client and server threads are created and client data is passed as a parameter so that each thread will know which clientId they have and use the common resources. Then, join the server threads since we want to write to output file "All seats are reserved." by the main after each server completes writing.

Client threads run ClientFun. At first, thread sleeps for a random time. Then it acquires mutex to select a seat from available seats. Since availableSeats set is shared between threads and can be modified by other threads, this is a critical section. That is why we lock mutex before entering this section. In this section, client gets the size of the set and selects one number between 0 and size-1. Then, advance function is used to get the element with the selected number id. Since the client selects that seat, client removes it from the set. The critical section ends here. The selected seatNo is sent to server and first semaphore is signaled so that server can continue. Client waits for second semaphore to be released. After it is released it checks if the reservation is successful and exits.

Server threads run ServerFun. Server function waits until client selects a seat and first semaphore is used to achieve that. Then it gets the requested seat number from pipe. Now it needs to write to output file but this is a critical section since many server threads may want to write to the output file simultaneously. So, it needs to acquire writeMutex before writing. After writing is finished, writeMutex is released. It puts a message to pipe to indicate that reservation is successful and signals the second semaphore. Then, server thread exits. 


## Implementation Environment
 I implemented this program using C++ in the Linux platform with GCC/G++ Compiler. 
 
Linux Version: 18.04.1 LTS (Bionic Beaver)

GCC Version: gcc (Ubuntu 7.3.0-27ubuntu1~18.04) 7.3.0

## How to run
1.  Open terminal in Linux
2.  Go to the project directory
3.  Run the following command 

`$ make`

or you can directly run 

`$ g++ main.cpp -o flight_simulation -pthread -std=c++11`

4.  Run the program with a single argument which is the seat number - an integer value. 

`$ ./flight_simulation [seat_no]`
	

