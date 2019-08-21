CFLAGS		= -Wall -g
CC				= gcc $(CFLAGS)

all  			: bl-server bl-client

bl-server : bl-server.o util.o server.o simpio.o
					$(CC) -o bl-server bl-server.o util.o server.o simpio.o
bl-server.o : bl-server.c blather.h
					$(CC) -c $<

bl-client : bl-client.o util.o server.o simpio.o
					$(CC) -o bl-client bl-client.o util.o server.o simpio.o -lpthread

bl-client.o : bl-client.c blather.h
					$(CC) -c $<

simpio.o : simpio.c
					$(CC) -c $<
server.o 	: server.c blather.h
					$(CC) -c $<
util.o   	: util.c blather.h
					$(CC) -c $<
make shell-tests : shell_tests.sh shell_tests_data.sh clean-tests
					   chmod u+rx shell_tests.sh shell_tests_data.sh normalize.awk filter-semopen-bug.awk
					   ./shell_tests.sh

clean:
				rm *.o bl-server bl-client *.fifo *.log
clean-tests :
				   rm -f test-*.{log,out,expect,diff,valgrindout}
