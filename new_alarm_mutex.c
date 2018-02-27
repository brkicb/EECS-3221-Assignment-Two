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

#define DEBUG

#define NUM_THREADS 10

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
    int                 type;   // the message type
    int                 status; // 0 for unassigned, 1 for assigned
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
    alarm_t *alarm, *prev, *next;
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

        next = alarm_list;
        prev = alarm_list;
        while (next != NULL)
        {
            if (next->status == 0)
            {
                next->status = 1;
                printf("\nAlarm Request With Message Type (%d) Assigned to Alarm Thread %u at\n", next->type, (unsigned int)(pthread_self()));
                printf("%d:Type A\n\n", next->time);
            }
            prev = next;
            next = next->link;             
        }

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
    int status, worker_status[NUM_THREADS];
    alarm_t *alarm, **last, *next, *prev;
    pthread_t thread;
    pthread_t workers[NUM_THREADS];
    pthread_attr_t attr[NUM_THREADS];
    int type[NUM_THREADS];
    int type_i = 0;
    char line[128];
    char request_type[10];
    int i;
    int thread_i = 0;

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
            strncpy(request_type, "Type A", 10);
            alarm->status = 0;
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
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
                    printf("Alarm List at %ld:%s\n\n", alarm->time, request_type);
                    break;
                }
                else if (next->time >= alarm->time) 
                {
                    alarm->link = next;
                    *last = alarm;
                    printf("\nAlarm Request With Message Type(%d) Inserted by Main Thread %d Into\n", alarm->type, thread);
                    printf("Alarm List at %ld:%s\n\n", alarm->time, request_type);
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
                printf("Alarm List at %ld:%s\n\n", alarm->time, request_type);
            }
			#ifdef DEBUG
                printf ("[list: ");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time, next->time - time (NULL), next->message);
            	printf ("]\n");
			#endif

            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
        else if (!(sscanf (line, "Create_Thread: MessageType(%d)", &alarm->type) < 1))
        {
            strncpy(request_type, "Type B", 10);
            int this_type = alarm->type;
            alarm->time = time (NULL);
            
            worker_status[thread_i] = pthread_create (&workers[thread_i], NULL, alarm_thread, NULL);

            if (worker_status[thread_i] != 0)
                err_abort (worker_status[thread_i], "Create alarm thread");

            thread_i = ((thread_i + 1) % NUM_THREADS);

            type[type_i] = this_type;
            type_i = ((type_i + 1) % NUM_THREADS);

            printf("\nNew Alarm Thread %d For Message Type (%d) " 
                "Created at %ld:%s\n\n", workers[thread_i-1], alarm->type, alarm->time, request_type);


            last = &alarm_list;
            next = *last;
            while (next != NULL)
            {
                if (next->type == this_type)
                {
                    worker_status[thread_i-1] = pthread_mutex_lock (&alarm_mutex);
                    if (worker_status[thread_i-1] != 0)
                        err_abort (worker_status[thread_i-1], "Lock mutex");
                }
                last = &next->link;
                next = next->link;
            }

            worker_status[thread_i-1] = pthread_mutex_unlock (&alarm_mutex);
            if (worker_status[thread_i-1] != 0)
                err_abort (worker_status[thread_i-1], "Unlock mutex");

        }
        else if (!(sscanf (line, "Terminate_Thread: MessageType(%d)", &alarm->type) < 1))
        {
            strncpy(request_type, "Type C", 10);
            int this_type = alarm->type;
            for (i=0; i<=NUM_THREADS; i++)
            {
                if ((type[i] == alarm->type) && (type[i] != NULL))
                {
                    type[i] = -1;
                    pthread_cancel(workers[i]);
                }
                
            }

            #ifdef DEBUG
                printf ("[list: (At start of loop)");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time, next->time - time (NULL), next->message);
                printf ("]\n");
            #endif

            next = alarm_list;
            prev = alarm_list;
            while (next != NULL)
            {
                if (next->type == this_type)
                {
                    // start of list
                    if (next == prev)
                    {
                        alarm_t *temp;
                        temp = next;
                        next = next->link;
                        temp->link = NULL;
                        prev = next;
                        alarm_list = next;
                    }
                    // middle or end of list
                    else
                    {
                        alarm_t *temp;
                        temp = next->link;
                        prev->link = next->link;
                        next->link = NULL;
                        next = temp;
                    }
                }
                else
                {
                    prev = next;
                    next = next->link;
                }              
            }

            #ifdef DEBUG
                printf ("[list: (At end of loop)");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time, next->time - time (NULL), next->message);
                printf ("]\n");
            #endif

            alarm->time = time (NULL);
            printf("\nAll Alarm Threads for Message Type(%d) Terminated And All Messages "
                "of\nMessage Type Removed at %ld:%s\n\n", this_type, alarm->time, request_type);
        }
    }
}
