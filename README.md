# Chat Server

Creates a chat client that is able to communicate with a server. The client will send messages to the server, which is then able to broadcast the message to all the other clients on the system. Similarly, the client will be able to receive messages directly from the server, thus enabling the user to read messages sent by all other clients on the system. 

The client prompts for a username, parses it, then creates a TCP Socket to connect to the server and receive the welcome message. The program also uses select to check for activity (messages) from stdin as well as the client socket.

Note: This program requires two command-line arguments: server IP and port. If you are running the server locally, you specify the address as 127.0.0.1. The port must be an integer in the range [1024, 65535]. 
