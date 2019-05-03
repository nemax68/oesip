/**
 * @file posixque.h POSIX Message Queue
 *
 * Copyright (C) 2019 open-eyes.it
 */

#define POSIX_QUEUE_NAME_SIZE	    64
#define POSIX_CMD_BUFFER_SIZE       256
#define POSIX_EVT_BUFFER_SIZE       1024

struct posix_queue {
 	size_t size;
  	char qname[POSIX_QUEUE_NAME_SIZE];
 	//char buf[POSIX_QUEUE_BUFFER_SIZE];
 	mqd_t mq;
};

struct posix_st {
    struct posix_queue	cmd;
 	struct posix_queue	evt;
};

int posix_command_init(struct posix_st *);
int posix_command_close(struct posix_st *);

int posix_event_init(struct posix_st *);
int posix_event_close(struct posix_st *);
