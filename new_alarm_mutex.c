// TOAN TRUONG
//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//           Budda Preserves, No Bug At All
//
//
//

/*
 * new_rw_mutex.c
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

<<<<<<< HEAD
//#define DEBUG
#define NUM_THREADS 10
=======
#define TRUE  1
#define FALSE 0
>>>>>>> Qiao

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
    time_t              time;            /* seconds from EPOCH */
    char                message[128];
    int                 assigned;        //assigment flag boolean
    int                 terminate;       //termination flag boolean
}alarm_t;

pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;//for terminating thread
alarm_t *alarm_list = NULL;
int read_count = 0;


void *alarm_print(alarm_t *arg)//reader
{
    int status;

    while(1)
    {
        
        status = pthread_mutex_lock (&mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        read_count++;
        if (read_count == 1)
        {
            status = pthread_mutex_lock (&rw_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
        }
        status = pthread_mutex_unlock (&mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");

        if(arg->terminate == TRUE)
        {
            free(arg);
            pthread_exit(0);
        }
        printf ("Alarm With Message Type (%d) Printed by Alarm Thread %d at %ld : Type A %s\n", arg->type, pthread_self(), time(NULL),arg->message);
        
        status = pthread_mutex_lock (&mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        read_count--;
        if (read_count == 0)
        {
            status = pthread_mutex_unlock (&rw_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
        status = pthread_mutex_unlock (&mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");



        sched_yield ();
        sleep(arg->seconds);

    }
}

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (int *arg)//writter
{
    alarm_t *alarm,*head;
    int sleep_time;
    time_t now;
    int status;
    pthread_t thread;



    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) 
    {
        status = pthread_mutex_lock (&rw_mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        alarm = alarm_list;
        while(alarm != NULL)//go through the list
        {
            
            if(alarm->type == *arg&&alarm->assigned != TRUE)//check message tye
            {
                //start a new thread to print them
                alarm -> assigned = TRUE;
                printf("Alarm Request With Message Type (%d) Assigned to Alarm Thread %d at %ld: %s\n", alarm->type, pthread_self(), time(NULL),alarm->message);
                status = pthread_create (&thread, NULL, alarm_print, alarm);
                if (status != 0)
                    err_abort (status, "Create alarm thread");

            }
            alarm = alarm->link; 
        }
        status = pthread_mutex_unlock (&rw_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
    }
}

int main (int argc, char *argv[])//writter
{
<<<<<<< HEAD
    int status, worker_status[NUM_THREADS];
    alarm_t *alarm, **last, *next;
    pthread_t thread;
    pthread_t workers[NUM_THREADS];
    pthread_attr_t attr[NUM_THREADS];
    char line[128];
    int i;
    int thread_i = 0;

=======
    int status, new_thread;
    char line[128];
    alarm_t *alarm, **last, *next, *t_alarm;
    pthread_t thread;
    
    /*
>>>>>>> Qiao
    // make a separate thread that will run the alarm_thread function
    status = pthread_create (&thread, NULL, alarm_thread, NULL);
    // error in creation of the thread if status isn't 0
    if (status != 0)
        err_abort (status, "Create alarm thread");
    */

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
        alarm->assigned = FALSE;
        alarm->terminate  = FALSE;
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
            printf("\nAlarm Request With Message Type(%d) Inserted by Main Thread %d Into\n", alarm->type, pthread_self());//Change thread to pthread_self()
            printf("Alarm List at %ld:%d\n\n", time(NULL), alarm->type);
            
            status = pthread_mutex_lock (&rw_mutex);
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
                if (next->type >= alarm->type) 
                {
                    alarm->link = next;
                    *last = alarm;
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
            }
            #ifdef DEBUG
                printf ("[list: ");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time,
                        next->time - time (NULL), next->message);
                printf ("]\n");
            #endif
            status = pthread_mutex_unlock (&rw_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
        else if (!(sscanf (line, "Create_Thread: MessageType(%d)", &alarm->type) < 1))
        {
<<<<<<< HEAD
            int this_type = alarm->type;
            
            worker_status[thread_i] = pthread_create (&workers[thread_i], NULL, alarm_thread, NULL);

            if (worker_status[thread_i] != 0)
                err_abort (worker_status[thread_i], "Create alarm thread");

            thread_i = ((thread_i + 1) % 10);
            printf("New Alarm Thread %d For Message Type (%d) " 
                "Created at %ld:%d\n", workers[thread_i-1], alarm->type, alarm->time, alarm->type);
=======
            printf("Entered Create_Thread Branch\n");
            new_thread = pthread_create (&thread, NULL, alarm_thread, &alarm->type);//pass alarm->type as parameter to the new thread
            
            printf("New Alarm Thread %d For Message Type (%d) Created at %ld:Type B\n", thread, alarm->type, time(NULL));
            //printf("%d\n",*alarm->type);
            
        }
        else if (!(sscanf (line, "Terminate_Thread: MessageType(%d)", &alarm->type) < 1))
        {
            printf("Entered Terminate_Thread Branch\n");
>>>>>>> Qiao

            status = pthread_mutex_lock (&rw_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");

            t_alarm = alarm_list;
            while(t_alarm != NULL)//go through the list
            {
<<<<<<< HEAD
                if (next->type == this_type)
                {
                    worker_status[thread_i-1] = pthread_mutex_lock (&alarm_mutex);
                    if (worker_status[thread_i-1] != 0)
                        err_abort (worker_status[thread_i-1], "Lock mutex");
=======
            
                if(t_alarm->type == alarm->type)//check message tye
                {
                    //switching termanation flag
                    t_alarm->terminate = TRUE;
>>>>>>> Qiao
                }
                t_alarm = t_alarm->link; 
            }

<<<<<<< HEAD
            worker_status[thread_i-1] = pthread_mutex_unlock (&alarm_mutex);
            if (worker_status[thread_i-1] != 0)
                err_abort (worker_status[thread_i-1], "Unlock mutex");

        }
        else if (!(sscanf (line, "Terminate_Thread: MessageType(%d)", &alarm->type) < 1))
        {
            for (i=0; i<=NUM_THREADS; i++)
            {
                pthread_cancel(workers[i]);
            }

            last = &alarm_list;
            next = *last;
            while (next != NULL) 
            {
                // need way to remove node with specified type
                if (next->type == alarm->type)
                {
                    //last->link = last->link->link;
                    //next->link = next->link->link;
                }
                last = &next->link;
                next = next->link;
            }
=======
            status = pthread_mutex_unlock (&rw_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
>>>>>>> Qiao

            printf("All Alarm Threads for Message Type(%d) Terminated And All Messages of Message Type Removed at %ld:Type C\n", alarm->type, time(NULL));
        }
    }
}