/**
 * @file posixque.c Standard posix message queue UI module
 *
 * Copyright (C) 2019 open-eyes.it
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/stat.h>

#include <re.h>
#include <baresip.h>

#include "posixque.h"

/**
 * @defgroup posixque posixque
 *
 * User-Interface (UI) module for posix message queue
 *
 * This module contains functions for sending outgoing messages
 * from voip client to posix queue
 * The module is indented for Unix-based systems.
  */

extern struct posix_st *posix_ui_state;


static int print_handler(const char *p, size_t size, void *arg)
{
	struct mbuf *mb = arg;
	return mbuf_write_mem(mb, (uint8_t *)p, size);
}

static int ipc_send_event(struct posix_st *st, struct mbuf *p)
{
    mqd_t mq;
    size_t len=p->pos;

	//*(p->buf+len)=0;
	//printf("IPC: [%s]\n",p->buf);

    if(len>st->evt.size)
        return ENOMEM;

    /* open the mail queue */
    mq = mq_open(st->evt.qname, O_WRONLY|O_NONBLOCK );
    if(mq==(mqd_t)-1)
		return ENODEV;

	if(	mq_send(mq, (char *)p->buf, len, 0) )
		return ENODEV;

	mq_close(mq);

	return 0;
}

/*
 * Relay UA events as send messages to the POSIX queue
 */

static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{
	struct posix_st *st = arg;
    struct mbuf *buf = mbuf_alloc(POSIX_EVT_BUFFER_SIZE);
    struct re_printf pf = {print_handler, buf};
	struct odict *od = NULL;
	int err=0;

    if (buf==NULL) {
        goto out_no_buf;
    }

	if( (err=odict_alloc(&od, 8)) ) {
		goto out_no_odict;
    }

	if( (err=event_encode_dict(od, ua, ev, call, prm)) ) {
		goto out;
    }

    if( (err=json_encode_odict(&pf, od)) ){
        goto out;
    }

    if( (err=ipc_send_event(st,buf)) ){
        goto out;
    }

out:
	mem_deref(od);
out_no_odict:
    mem_deref(buf);
out_no_buf:
	if(err)
    	info("posix: failed to send message (%m)\n", err);
}

int posix_event_init(struct posix_st *st)
{
    int err=0;

 	if (!st)
 		return EINVAL;

    conf_get_str(conf_cur(), "posix_evt_queue_name",
                            st->evt.qname, POSIX_QUEUE_NAME_SIZE);

    if (strlen(st->evt.qname)==0)
    	return EINVAL;
    st->evt.size = POSIX_EVT_BUFFER_SIZE;

	if( (err=uag_event_register(ua_event_handler, st)) )
		return err;

	return 0;
}

int posix_event_close(struct posix_st *st)
{
	debug("posixque: event close\n");

	return 0;
}
