# libevent_server

The server obtains phrases via the program fortune, i.e., by running fortune -s as a child
process and reading the output produced by the child process from a pipe. 

The server than selects a random word, replaces it with underscores, and sends the challenge to the client. 

to run: 
make
