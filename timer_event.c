#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "timer-event.h"
#include "rsvp_msg.h"
#include "rsvp_db.h"

extern int sock;
struct in_addr sender_ip, receiver_ip;

extern struct session* sess;
extern struct session* path_head;
extern struct session* resv_head;
extern path_node* path_tree;
extern resv_node* resv_tree;

#define TIMEOUT 90
#define INTERVAL 30

// Timer event handler for sending PATH message
// will be checking RESV msgs for every session and 30sc
// No RESV msg for 90 sec for the specific session         
// send PATH tear message if RESV msg not received

void path_timer_handler(union sigval sv) {

    time_t now = time(NULL);
    struct session* temp = NULL;
    temp = resv_head;

    printf("path timer handler \n");
    while(temp != NULL) {
        if((now - temp->last_path_time) > TIMEOUT) {
            printf("RSVP session expired: %s->%s\n",temp->sender, temp->receiver);
            resv_head = delete_session(temp, temp->sender, temp->receiver);
            resv_tree = delete_resv_node(resv_tree, temp->tunnel_id);
        } else if((now - temp->last_path_time) < INTERVAL) {
            printf(" less than 30 sec\n");
            temp = temp->next;
            continue;
        } else {
            //printf("--------No resv msg, sending path message\n");

            inet_pton(AF_INET, temp->sender, &sender_ip);
            inet_pton(AF_INET, temp->receiver, &receiver_ip);

            // Send RSVP-TE PATH Message
            send_path_message(sock, sender_ip, receiver_ip);
        }
        temp = temp->next;
    }
}

// Timer event handler for sending RESV message
// will be checking path msgs for every session and 30sc
// No path message for 90 sec for the specific session 
// send resv tear message if path msg not received

void resv_timer_handler(union sigval sv) {
    time_t now = time(NULL);
    struct session* temp = NULL;
    temp = path_head;

    printf("resv timer handler \n");
    while(temp != NULL) {
        if((now - temp->last_path_time) > TIMEOUT) {
            printf("RSVP session expired: %s->%s\n",temp->sender, temp->receiver);
            path_head = delete_session(temp, temp->sender, temp->receiver);
            path_tree = delete_path_node(path_tree, temp->tunnel_id);
        } else if((now - temp->last_path_time) < INTERVAL) {
            printf(" less than 30 sec\n");
            temp = temp->next;
            continue;
        } else {
            //printf("-------No path msg, sending resv message\n");

            inet_pton(AF_INET, temp->sender, &sender_ip);
            inet_pton(AF_INET, temp->receiver, &receiver_ip);
            send_resv_message(sock, sender_ip, receiver_ip);
        }
        temp = temp->next;
    }
}

// Function to create a timer that triggers every 30 seconds
timer_t create_timer(void (*handler)(union sigval)) {
    struct sigevent sev;
    timer_t timerid;

    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handler;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) < 0) {
        perror("Timer creation failed");
        exit(EXIT_FAILURE);
    }
    return timerid;
}


// Function to start a timer with a 30-second interval
void start_timer(timer_t timerid) {
    struct itimerspec its;
    its.it_value.tv_sec = INTERVAL;   // Initial delay
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = INTERVAL; // Repeating interval
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) < 0) {
        perror("Timer start failed");
        exit(EXIT_FAILURE);
    }
}

void path_event_handler() {
    timer_t path_timer = create_timer(path_timer_handler);
    start_timer(path_timer);
}

void resv_event_handler() {
    timer_t resv_timer = create_timer(resv_timer_handler);
    start_timer(resv_timer);
}

