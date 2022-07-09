Name: Eylon Naamat
ID: 315303529
Name: Michael Matveev
ID: 315918557

Brief Explanation About The Code:
the beej multi process server implemented in ex3 merge with the stack, locking, shared memory is server.cpp (multiprocess - section 1,2,3).
the client is client.cpp, the tests are tests.cpp.


How To Run The Code:
first run the command make all in the terminal (after you in the directory), then type "./server" in order to run the server.
then in another terminal (or terminals if you want multi clients) run the command "./client" and start typing commands for the stack.
if you want to check our tests you should exit everything, run the server (by the command "./server"), open another terminal, and run "./tests".

if you want to exit from the server, first close the client as we explain later on, and then press ctrl+c, we made a sig_handler for SIGINT which closes the sockets, releasing the stack (and of course releasing memory), closing the shared memory file, and releasing the shared memory file.
if you want to close a client simply type in the clients terminal EXIT.