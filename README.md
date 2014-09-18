powerserve
==========

Extremely fast and lightweight HTTP server and client, used to prototype a website without the effort of setting up a full Apache installation.

##Features of the client:
* Gets basic HTML webpages (obviously)
* Can handle chunked encoding
* Completely memory safe (Valgrind-approved)

##Features of the server:
* Multithreaded using POSIX-compatible pthreads
* Can serve out images
* Completely memory safe (Valgrind-approved)
* Ctrl-C is caught and causes server to shutdown cleanly
* *Most*, if not all errors are caught and will cause the program to safely close

##Building:
Both the server and client have a Makefile for building them. A standard "make all" will build the binary. However, if you want to build it with debugging flags, "make debug" will do that. Furthermore, if you have valgrind installed and in your PATH, "make run-debug" will compile it with debugging and launch it under valgrind. "make run" just builds and runs the executable.

##Running:
Build with either the "all" or "debug" make target.
The server executable is called "server", and the client is, intuitively, called "client"
