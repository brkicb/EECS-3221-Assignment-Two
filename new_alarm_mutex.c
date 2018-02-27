/*
 * new_alarm_mutex.c
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "errors.h"

//#define DEBUG

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag 
{
    struct alarm_tag    *link;
    int                 seconds;
    int                 type;
    time_t              time;   /* seconds from EPOCH */
    char                message[128];
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;

    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) 
    {
        // start critical section
        status = pthread_mutex_lock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        alarm = alarm_list;

        /*
         * If the alarm list is empty, wait for one second. This
         * allows the main thread to run, and read another
         * command. If the list is not empty, remove the first
         * item. Compute the number of seconds to wait -- if the
         * result is less than 0 (the time has passed), then set
         * the sleep_time to 0.
         */
        if (alarm == NULL)
            sleep_time = 1;
        else 
        {
            alarm_list = alarm->link;
            now = time (NULL);
            if (alarm->time <= now)
                sleep_time = 0;
            else
                sleep_time = alarm->time - now;
			#ifdef DEBUG
            	printf ("[waiting: %d(%d)\"%s\"]\n", alarm->time,
                	sleep_time, alarm->message);
			#endif
        }

        /*
         * Unlock the mutex before waiting, so that the main
         * thread can lock it to insert a new alarm request. If
         * the sleep_time is 0, then call sched_yield, giving
         * the main thread a chance to run if it has been
         * readied by user input, without delaying the message
         * if there's no input.
         */

        // end critical section
        status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
        if (sleep_time > 0)
            sleep (sleep_time);
        else
            sched_yield ();

        /*
         * If a timer expired, print the message and free the
         * structure.
         */
        if (alarm != NULL) 
        {
            printf ("(%d) %s\n", alarm->seconds, alarm->message);
            free (alarm);
        }
    }
}

int main (int argc, char *argv[])
{
    int status, second_status;
    char line[128];
    alarm_t *alarm, **last, *next;
    pthread_t thread;
    pthread_t second_thread;
    int type;
    

    // make a separate thread that will run the alarm_thread function
    status = pthread_create (&thread, NULL, alarm_thread, NULL);
    // error in creation of the thread if status isn't 0
    if (status != 0)
        err_abort (status, "Create alarm thread");
    while (1) 
    {
    	// the prompt
        printf ("alarm> ");
        // fget==NULL when EOF detected -- cntr d
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        // if you didn't write anything, go back to the beginning of loop
        if (strlen (line) <= 1) continue;

        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */
        if ((sscanf (line, "%d MessageType(%d) %128[^\n]", &alarm->seconds, &alarm->type, alarm->message) < 2)
        		&& (sscanf (line, "Create_Thread: MessageType(%d)", &alarm->type) < 1)
        		&& (sscanf (line, "Terminate_Thread: MessageType(%d)", &alarm->type) < 1)) 
        {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        } 
        else if (!(sscanf(line, "%d MessageType(%d) %128[^\n]", &alarm->seconds, &alarm->type, alarm->message) < 2))
        {
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
            // last holds value of address of alarm_list which holds another address
            // &last = 0x8080 
            // last = &alarm_list = 0x8092 
            // *last = alarm_list = 0x8194 
            // **last = *alarm_list = (alarm_t value)
            last = &alarm_list;
            // next holds value of the adress held in alarm_list
            // next = *last = alarm_list = 0x8194
            next = *last;
            while (next != NULL) 
            {
                if (alarm->type < next->type)
                {
                    alarm->link = next;
                    *last = alarm;
                    printf("\nAlarm Request With Message Type(%d) Inserted by Main Thread %d Into\n", alarm->type, thread);
                    printf("Alarm List at %ld:%d\n\n", alarm->time, alarm->type);
                    break;
                }
                else if (next->time >= alarm->time) 
                {
                    alarm->link = next;
                    *last = alarm;
                    printf("\nAlarm Request With Message Type(%d) Inserted by Main Thread %d Into\n", alarm->type, thread);
                    printf("Alarm List at %ld:%d\n\n", alarm->time, alarm->type);
                    break;
                }
                last = &next->link;
                next = next->link;
            }
            /*
             * If we reached the end of the list, insert the new
             * alarm there. ("next" is NULL, and "last" points
             * to the link field of the last item, or to the
             * list header).
             */
            if (next == NULL) 
            {
                *last = alarm;
                alarm->link = NULL;
                printf("\nAlarm Request With Message Type(%d) Inserted by Main Thread %d Into\n", alarm->type, thread);
                printf("Alarm List at %ld:%d\n\n", alarm->time, alarm->type);
            }
			      #ifdef DEBUG
                printf ("[list: ");
            	  for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time,
                        next->time - time (NULL), next->message);
            	  printf ("]\n");
			      #endif

            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
        else if (!(sscanf (line, "Create_Thread: MessageType(%d)", &type) < 1))
        {
            int this_type = type;
            second_status = pthread_create (&second_thread, NULL, alarm_thread, NULL);

            if (second_status != 0)
                err_abort (second_status, "Create alarm thread");


            printf("New Alarm Thread %d For Message Type (%d) " 
                "Created at %ld:%d\n", thread, alarm->type, alarm->time, alarm->type);


            last = &alarm_list;
            next = *last;
            while (next != NULL)
            {
                if (alarm->type == this_type)
                {
                    second_status = pthread_mutex_lock (&alarm_mutex);
                    if (second_status != 0)
                        err_abort (second_status, "Lock mutex");
                }
                last = &next->link;
                next = next->link;
            }

            second_status = pthread_mutex_unlock (&alarm_mutex);
            if (second_status != 0)
                err_abort (second_status, "Unlock mutex");

        }
        else if (!(sscanf (line, "Terminate_Thread: MessageType(%d)", &alarm->type) < 1))
        {
            

            printf("All Alarm Threads for Message Type(%d) Terminated And All Messages "
                "of\nMessage Type Removed at %ld:%d\n", alarm->type, alarm->time, alarm->type);
        }
    }
}
