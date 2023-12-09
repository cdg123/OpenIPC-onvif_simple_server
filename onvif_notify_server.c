/*
 * Copyright (c) 2023 roleo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>

#include "conf.h"
#include "utils.h"
#include "log.h"
#include "onvif_simple_server.h"

#define DEFAULT_CONF_FILE "/etc/onvif_simple_server.conf"
#define DEFAULT_LOG_FILE "/var/log/onvif_notify_server.log"
#define DEFAULT_PID_FILE "/var/run/onvif_notify_server.pid"
#define TEMPLATE_DIR "/etc/onvif_notify_server"
#define INOTIFY_DIR "/tmp/onvif_notify_server"

#define BD_NO_CHDIR          01
#define BD_NO_CLOSE_FILES    02
#define BD_NO_REOPEN_STD_FDS 04

#define BD_NO_UMASK0        010
#define BD_MAX_CLOSE       8192

#define PID_SIZE 32

subscriptions_t *subscriptions;
service_context_t service_ctx;

// Global variables
char *conf_file;
int debug;
FILE *fLog;
int exit_main;

int daemonize(int flags)
{
    int maxfd, fd;

    switch(fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(EXIT_SUCCESS);
    }

    if(setsid() == -1)
        return -1;

    switch(fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(EXIT_SUCCESS);
    }

    if(!(flags & BD_NO_UMASK0))
        umask(0);

    if(!(flags & BD_NO_CHDIR))
        chdir("/");

    if(!(flags & BD_NO_CLOSE_FILES)) {
        maxfd = sysconf(_SC_OPEN_MAX);
        if(maxfd == -1)
            maxfd = BD_MAX_CLOSE;
        for(fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if(!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO);

        fd = open("/dev/null", O_RDWR);
        if(fd != STDIN_FILENO)
            return -1;
        if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -2;
        if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -3;
    }

    return 0;
}

int check_pid(char *file_name)
{
    FILE *f;
    long pid;
    char pid_buffer[PID_SIZE];

    f = fopen(file_name, "r");
    if(f == NULL)
        return 0;

    if (fgets(pid_buffer, PID_SIZE, f) == NULL) {
        fclose(f);
        return 0;
    }
    fclose(f);

    if (sscanf(pid_buffer, "%ld", &pid) != 1) {
        return 0;
    }

    if (kill(pid, 0) == 0) {
        return 1;
    }

    return 0;
}

int create_pid(char *file_name)
{
    FILE *f;
    char pid_buffer[PID_SIZE];

    f = fopen(file_name, "w");
    if (f == NULL)
        return -1;

    memset(pid_buffer, '\0', PID_SIZE);
    sprintf(pid_buffer, "%ld\n", (long) getpid());
    if (fwrite(pid_buffer, strlen(pid_buffer), 1, f) != 1) {
        fclose(f);
        return -2;
    }
    fclose(f);

    return 0;
}

void signal_handler(int signal)
{
    // Exit from main loop
    exit_main = 1;
}

int send_notify(char *reference, int alarm_index, char *property, char *value)
{
    char host[1024];
    int port = 80;
    char page[1024];
    char header_fmt[] = "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/soap+xml\r\nContent-Length: %s\r\n\r\n";
    char *header;
    char *message;
    int size;
    char size_string[8];
    char template_file[	1024];
    int sockfd;
    struct sockaddr_in remote;
    char utctime[32];
    time_t now;

    // Prepare time string
    now = time(NULL);
    to_iso_date(utctime, sizeof(utctime), now);

    // Prepare IP address
    if (strncmp("https", reference, 5) == 0)
        port = 443;
    if (strchr(&reference[6], ':') == NULL) {
        sscanf(reference, "%*[^:]%*[:/]%[^/]%s", host, page);
    } else {
        sscanf(reference, "%*[^:]%*[:/]%[^:]:%d%s", host, &port, page);
    }

    // Configure socket
    memset(&remote, '\0', sizeof(remote));
    remote.sin_addr.s_addr = inet_addr(host);
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);

    log_debug("Sending notify message to %s - host %s - port %d - page %s", reference, host, port, page);
    log_debug("topic %s - UTC time %s - value %s", service_ctx.events[alarm_index].topic, utctime, value);

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_error("Error opening socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) &remote, sizeof(remote)) != 0) {
        log_error("Connection failed");
        return -2;
    }

    // Get size of message content
    log_info("Sending Notify message.");
    sprintf(template_file, "%s/Notify.xml", TEMPLATE_DIR);
    size = cat(NULL, template_file, 12,
            "%TOPIC%", service_ctx.events[alarm_index].topic,
            "%UTC_TIME%", utctime,
            "%PROPERTY%", property,
            "%SOURCE_NAME%", service_ctx.events[alarm_index].source_name,
            "%SOURCE_VALUE%", service_ctx.events[alarm_index].source_value,
            "%VALUE%", value);
    sprintf(size_string, "%d", size);

    header = (char *) malloc((strlen(header_fmt) + strlen(page) + strlen(host) + strlen(size_string) + 4) * sizeof(char));
    if (header == NULL) {
        log_error("Malloc error.\n");
        return -3;
    }
    sprintf(header, header_fmt, page, host, size_string);

    message = (char *) malloc((size + strlen(header) + 1) * sizeof(char));
    if (message == NULL) {
        log_error("Malloc error.\n");
        free(header);
        return -3;
    }

    strcpy(message, header);
    cat(&message[strlen(header)], template_file, 12,
            "%TOPIC%", service_ctx.events[alarm_index].topic,
            "%UTC_TIME%", utctime,
            "%PROPERTY%", property,
            "%SOURCE_NAME%", service_ctx.events[alarm_index].source_name,
            "%SOURCE_VALUE%", service_ctx.events[alarm_index].source_value,
            "%VALUE%", value);

    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *) &remote, sizeof(remote)) < 0) {
        log_error("Error sending Notify message.\n");
        free(header);
        free(message);
        return -4;
    }

    free(header);
    free(message);
    log_info("Sent.");

    // Close socket
    shutdown(sockfd, SHUT_RDWR);

    return 0;
}

void sync_events()
{
    int i, j;
    char value[8];
    time_t now;

    log_info("Synchronization requested");

    for (i = 0; i < service_ctx.events_num; i++) {
        if (access(service_ctx.events[i].input_file, F_OK) == 0)
            strcpy(value, "true");
        else
            strcpy(value, "false");

        for(j = 0; j < MAX_SUBSCRIPTIONS; j++) {
            if (subscriptions->items[j].used == 1) {
                // Check if subscription is expired
                now = time(NULL);
                if (now > subscriptions->items[j].expire) continue;

                send_notify(subscriptions->items[j].reference, i, "Initialized", value);
            }
        }
    }

    log_info("Synchronization done");
}

int handle_events(int fd, char *dir)
{
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[4096]__attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    int i, j;
    int sub_count = 0;
    char input_file[1024], value[8];
    time_t now;
    char *ptr;

    /* Loop while events can be read from inotify file descriptor. */
    for (;;) {
        /* Read some events. */

        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            log_error("Error reading from inotify file descriptor");
            return -1;
        }

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */
        if (len <= 0)
            break;

        /* Loop over all events in the buffer. */
        for (ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            /* Print event type. */
            if (((event->mask & IN_CREATE) || (event->mask & IN_DELETE)) && ((event->mask & IN_ISDIR) == 0) && (event->len)) {
                sprintf(input_file, "%s/%s", dir, event->name);
                if (event->mask & IN_CREATE) {
                    strcpy(value, "true");
                    log_debug("File %s created", input_file);
                } else if (event->mask & IN_DELETE) {
                    strcpy(value, "false");
                    log_debug("File %s deleted", input_file);
                }

                for (i = 0; i < service_ctx.events_num; i++) {
                    if (strcmp(service_ctx.events[i].input_file, input_file) == 0) {
                        now = time(NULL);
                        for(j = 0; j < MAX_SUBSCRIPTIONS; j++) {
                            if (subscriptions->items[j].used == 1) {
                                // Check if subscription is expired
                                if (now > subscriptions->items[j].expire) continue;
                                sub_count++;
                                send_notify(subscriptions->items[j].reference, i, "Changed", value);
                            }
                        }
                        log_debug("%d subscriptions for %s file", sub_count, service_ctx.events[i].input_file);
                    }
                }
            }
        }
    }
}

void clean_expired_subscriptions()
{
    int i;
    time_t now;

    for(i = 0; i < MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions->items[i].expire != 0) {
            // Check if subscription is expired
            now = time(NULL);
            if (now > subscriptions->items[i].expire) {
                memset(&subscriptions->items[i], '\0', sizeof(subscription_t));
            }
        }
    }
}

void *sync_events_thread(void *arg)
{
    while (!exit_main) {
        // Sync all events
        if (subscriptions->need_sync == 1) {
            subscriptions->need_sync = 0;
            sync_events();
        }
        usleep(500 * 1000);
    }
}

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [-c CONF_FILE] [-p PID_FILE] [-f] [-d LEVEL]\n\n", progname);
    fprintf(stderr, "\t-c CONF_FILE, --conf_file CONF_FILE\n");
    fprintf(stderr, "\t\tpath of the configuration file\n");
    fprintf(stderr, "\t-p PID_FILE, --pid_file PID_FILE\n");
    fprintf(stderr, "\t\tpid file\n");
    fprintf(stderr, "\t-f, --foreground\n");
    fprintf(stderr, "\t\tdon't daemonize\n");
    fprintf(stderr, "\t-d LEVEL, --debug LEVEL\n");
    fprintf(stderr, "\t\tenable debug with LEVEL = 0..5 (default 0 = log fatal errors)\n");
    fprintf(stderr, "\t-h, --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv)  {

    int errno;
    char *endptr;
    int c, i, j, ret, itmp;
    char pid_file[1024];
    int foreground;

    int fd = -1;
    int wd, poll_num;
    nfds_t nfds;
    struct pollfd fds[1];

    int acc;

    time_t now;

    conf_file = (char *) malloc((strlen(DEFAULT_CONF_FILE) + 1) * sizeof(char));
    strcpy(conf_file, DEFAULT_CONF_FILE);

    strcpy(pid_file, DEFAULT_PID_FILE);
    foreground = 0;
    debug = 5;

    while (1) {
        static struct option long_options[] =
        {
            {"conf_file",  required_argument, 0, 'c'},
            {"pid_file",  required_argument, 0, 'p'},
            {"foreground",  no_argument, 0, 'f'},
            {"debug",  required_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "c:p:fd:h",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 'c':
            /* Check for various possible errors */
            if (strlen(optarg) < MAX_LEN - 1) {
                free(conf_file);
                conf_file = (char *) malloc((strlen(optarg) + 1) * sizeof(char));
                strcpy(conf_file, optarg);
            } else {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'p':
            if (strlen(optarg) < 1024)
                strcpy(pid_file, optarg);
            break;

        case 'f':
            foreground = 1;
            break;

        case 'd':
            debug = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE && (debug == LONG_MAX || debug == LONG_MIN)) || (errno != 0 && debug == 0)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }

            if ((debug < LOG_TRACE) || (debug > LOG_FATAL)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            debug = 5 - debug;
            break;

        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }

    // Set signal handler
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    if (foreground == 0) {
        ret = daemonize(0);
        if (ret) {
            fprintf(stderr, "Error starting daemon.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Don't daemonize\n");
    }

    // Open file log
    fLog = fopen(DEFAULT_LOG_FILE, "w");
    log_add_fp(fLog, debug);
    log_set_level(debug);
    if (foreground == 0)
        log_set_quiet(1);
    log_info("Starting program.");

    log_debug("pid_file = %s", pid_file);

    // Checking pid file
    if (check_pid(pid_file) == 1) {
        log_fatal("Program is already running.\n");
        fclose(fLog);
        exit(EXIT_FAILURE);
    }
    if (create_pid(pid_file) < 0) {
        log_fatal("Error creating pid file %s\n", pid_file);
        fclose(fLog);
        exit(EXIT_FAILURE);
    }

    // Read configuration file
    log_info("Processing configuration file %s...", conf_file);
    itmp = process_conf_file(conf_file);
    if (itmp == -1) {
        log_fatal("Unable to find configuration file %s", conf_file);
        fclose(fLog);
        free(conf_file);
        exit(EXIT_FAILURE);
    } else if (itmp < -1) {
        log_fatal("Wrong syntax in configuration file %s", conf_file);
        fclose(fLog);
        free(conf_file);
        exit(EXIT_FAILURE);
    }
    log_info("Completed.");

    for (i = 0; i < service_ctx.events_num; i++) {
        log_debug("%s", service_ctx.events[i].input_file);
    }

    // Open shared memory
    subscriptions = (subscriptions_t *) create_shared_memory(1);
    if (subscriptions == NULL) {
        log_fatal("Unable to create shared memory.");
        unlink(pid_file);
        fclose(fLog);
        exit(EXIT_FAILURE);
    }
    memset(subscriptions, '\0', sizeof(subscriptions_t));

    // Create the file descriptor for accessing the inotify API
    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        if (errno == ENOSYS) {
            log_error("inotify interface not implemented, try poll strategy");
        } else {
            log_fatal("Unable to init inotify interface");
            destroy_shared_memory(subscriptions, 1);
            unlink(pid_file);
            fclose(fLog);
            exit(EXIT_FAILURE);
        }
    }

    if (fd != -1) {
        // Mark directory for events
        // - file was created
        // - file was deleted
        wd = inotify_add_watch(fd, INOTIFY_DIR, IN_CREATE | IN_DELETE);
        if (wd == -1) {
            log_fatal("Cannot watch '%s': %s\n", INOTIFY_DIR, strerror(errno));
            close(fd);
            destroy_shared_memory(subscriptions, 1);
            unlink(pid_file);
            fclose(fLog);
            exit(EXIT_FAILURE);
        }

        // Prepare for polling
        nfds = 1;
        fds[0].fd = fd; // Inotify input
        fds[0].events = POLLIN;
    }

    // Create thread to monitor subscriptions->need_sync
    pthread_t sync_events_pthread;
    pthread_create(&sync_events_pthread, NULL, sync_events_thread, NULL);
    pthread_detach(sync_events_pthread);

    // Wait for events
    log_info("Listening for events.");
    while (!exit_main) {

        // Check if new events are fired
        if (fd != -1) {
            poll_num = poll(fds, nfds, -1);
            if (poll_num == -1) {
                if (errno == EINTR)
                    continue;
                log_error("Error in poll");
                sleep(1);
            }

            if (poll_num > 0) {
                if (fds[0].revents & POLLIN) {

                    // Inotify events are available
                    handle_events(fd, INOTIFY_DIR);
                }
            }
        } else { // Inotify interface is not available
            for (i = 0; i < service_ctx.events_num; i++) {
                acc = access(service_ctx.events[i].input_file, F_OK);

                if ((service_ctx.events[i].is_on == 0) && (acc == 0)) {
                    service_ctx.events[i].is_on = 1;
                    log_info("File %s created", service_ctx.events[i].input_file);

                    now = time(NULL);
                    for(j = 0; j < MAX_SUBSCRIPTIONS; j++) {
                        if (subscriptions->items[j].used == 1) {
                            // Check if subscription is expired
                            if (now > subscriptions->items[j].expire) continue;
                            send_notify(subscriptions->items[j].reference, i, "Changed", "true");
                        }
                    }

                } else if ((service_ctx.events[i].is_on == 1) && (acc != 0)) {
                    service_ctx.events[i].is_on = 0;
                    log_info("File %s deleted", service_ctx.events[i].input_file);

                    now = time(NULL);
                    for(j = 0; j < MAX_SUBSCRIPTIONS; j++) {
                        if (subscriptions->items[j].used == 1) {
                            // Check if subscription is expired
                            if (now > subscriptions->items[j].expire) continue;
                            send_notify(subscriptions->items[j].reference, i, "Changed", "false");
                        }
                    }
                }
            }

            usleep(500 * 1000);
        }
    }

    log_info("Listening for events stopped.");

    // Close inotify file descriptor
    if (fd != -1)
        close(fd);

    destroy_shared_memory(subscriptions, 1);

    unlink(pid_file);
    log_info("Terminating program.");

    fclose(fLog);

    free_conf_file();
    free(conf_file);

    return 0;
}