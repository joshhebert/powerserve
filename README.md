Powerserve
==========

###Josh Hebert (jahebert@wpi.edu)

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

##Building:
Both the server and client have a Makefile for building them. A standard "make all" will build the binary. However, if you want to build it with debugging flags, "make debug" will do that.

##Running:
Build with either the "all" or "debug" make target.
The server executable is called "server", and the client is, intuitively, called "client". Both reside in the
bin folder of the respective directory.
