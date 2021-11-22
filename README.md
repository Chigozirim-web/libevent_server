# libevent_server

The server obtains phrases via the program fortune, i.e., by running fortune -s as a child
process and reading the output produced by the child process from a pipe. 

The server than selects a random word, replaces it with underscores, and sends the challenge to the client. 

## to run: 

in terminal type "make"
then,

./gwgd [-p some_port_number]

Finally,
connect to port the libevent server is running on;
if option "-p" was not included when running ./gwgd, then connect to port 8000.
