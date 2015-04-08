/**********************************************************
*
*   PURPOSE: Test for detecting carriage lost
*
**********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>

#define MAX_MSG_SIZE 4096

int  open_netlink(struct sockaddr_nl* snl);
void process_event(int sock_fd, struct sockaddr_nl* snl);
void print_event(struct nlmsghdr* h);

int main(int argc, char** argv) {
    int sock_fd;
    struct sockaddr_nl snl; 
    sock_fd = open_netlink(&snl);
    while(1) {
        process_event(sock_fd, &snl);
    } 
    return 0;
}

int open_netlink(struct sockaddr_nl* snl) {
    int sock_fd;
    memset(snl, 0, sizeof(*snl));
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    if (sock_fd < 0) {
        perror("ERROR: Could not create socket\n");
        return -1;
    }

    snl->nl_family = AF_NETLINK;
    snl->nl_pid = getpid();
    snl->nl_groups = RTMGRP_LINK;

    if (bind(sock_fd, (struct sockaddr *)snl, sizeof(*snl)) < 0) 
    {
        perror("ERROR: Could not bind socket\n");
        return -2;
    }
    return sock_fd;
} 

void process_event(int sock_fd, struct sockaddr_nl* snl) {
    char               msg_buffer[MAX_MSG_SIZE];
    int                len;
    struct nlmsghdr*   h;
    struct iovec       iov = {msg_buffer, sizeof(msg_buffer)};
    struct msghdr      msg = {snl, sizeof(*snl), &iov, 1, NULL, 0, 0};
    len = recvmsg(sock_fd, &msg, 0);

    if (len < 0) {
        perror("ERROR: Could not recv, error code\n");
    }

    for (h = (struct nlmsghdr *)msg_buffer; NLMSG_OK(h, len);
         h = NLMSG_NEXT(h, len)) {
        if (h->nlmsg_type == NLMSG_DONE) {
            perror("ERROR: MSG type is NLMSG_DONE\n");
            return;
        }

        if (h->nlmsg_type == NLMSG_ERROR) {
            perror("ERROR: MSG type is NLMSG_ERROR\n");
            exit(-1);
        }
        print_event(h);
    } 
} 

void print_event(struct nlmsghdr* h) {
    struct ifinfomsg* ifi = NLMSG_DATA(h);
    struct ifaddrmsg* ifa = NLMSG_DATA(h);
    char if_name[1024];

    switch (h->nlmsg_type) {
        case RTM_NEWLINK:
            if_indextoname(ifa->ifa_index, if_name);
            printf("RTM_NEWLINK: %s\n", if_name);
            printf("Link %s : %s\n", if_name,
                  (ifi->ifi_flags & IFF_RUNNING) ? "UP" : "DOWN");
            break;

        case RTM_DELLINK:
            if_indextoname(ifa->ifa_index, if_name);
            printf("RTM_DELLINK: %s\n", if_name);
            break;

        case RTM_NEWADDR:
            if_indextoname(ifi->ifi_index, if_name);
            printf("RTM_NEWADDR: %s\n", if_name);
            break;

        case RTM_DELADDR:
            if_indextoname(ifi->ifi_index, if_name);
            printf("RTM_DELADDR: %s\n", if_name);
            break;

        default:
            printf("ERROR: unknown message\n");
    }
}
