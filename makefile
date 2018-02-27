all: new_alarm_mutex.c
	cc -o new_alarm_mutex new_alarm_mutex.c -D_POSIX_PTHREAD_SEMANTICS -lpthread
