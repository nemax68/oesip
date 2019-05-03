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
 * This module sets up a posix queue for receive cmd and open another queue
 * for stdio.
 * The module is indented for Unix-based systems.
 */

static void handle_command(union sigval);

extern struct posix_st *posix_ui_state;

/*
 * Establish an asynchronous notification mechanism
 * for the arrival of messages in an empty POSIX message queue
 *
 *
 */

static int load_notify(mqd_t *mq)
{
    struct sigevent sev;
    mqd_t mqdes=*mq;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handle_command;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = mq;   /* Arg. to thread func. */

    if (mq_notify(mqdes, &sev) == -1)
		return ENODEV;

	return 0;
}

static int get_command(mqd_t mqdes, char *buf, int size)
{
	struct mq_attr attr;
	int ret=0;

	if (mq_getattr(mqdes, &attr) == 0) {
		debug("posixqueue: %ls max %ld maxsize %ld\n", attr.mq_curmsgs,attr.mq_maxmsg,attr.mq_msgsize);

		if(attr.mq_curmsgs){
			if(size<attr.mq_msgsize)
            	return ENOMEM;

            ret=mq_receive(mqdes, buf, attr.mq_msgsize, 0);
        }
    } else {
		return ENODATA;
    }

    return ret;
}

static void handle_command(union sigval sv)
{
    mqd_t mqdes = *((mqd_t *) sv.sival_ptr);
    mqd_t *mqpt = ((mqd_t *) sv.sival_ptr);
    struct mbuf *buf = mbuf_alloc(POSIX_CMD_BUFFER_SIZE);
    struct odict *od = NULL;
    const struct odict_entry *oe_cmd, *oe_prm, *oe_tok;
    char    cmd[POSIX_CMD_BUFFER_SIZE];
    char    *p;
    int     ret;
    int     err;

    if(buf==NULL)
        return; // gestire meglio

    p=(char *)buf->buf;

    ret = get_command(mqdes,p,POSIX_CMD_BUFFER_SIZE);

    if (ret>0) {
        err = json_decode_odict(&od, 32, p, ret, 16);
        if (err) {
            warning("posix: failed to decode JSON with %zu bytes (%m)\n",
                ret, err);
            return;
        }

        oe_cmd = odict_lookup(od, "command");
    	oe_prm = odict_lookup(od, "params");
    	oe_tok = odict_lookup(od, "token");
    	if (!oe_cmd) {
    		warning("posix: missing json entries\n");
    		goto out;
    	}
        /*
        info("posix: handle_command:  cmd='%s', token='%s' param=%s\n",
    	      oe_cmd ? oe_cmd->u.str : "",
    	      oe_tok ? oe_tok->u.str : "",
              oe_prm ? oe_prm->u.str : "");
    */
        printf("posix: handle_command:  cmd='%s'\n",oe_cmd->u.str);
        printf("posix: handle_command:  tok='%s'\n",oe_tok->u.str);
        printf("posix: handle_command:  par='%s'\n",oe_prm->u.str);

    	re_snprintf(cmd, sizeof(cmd), "%s%s%s",
    		    oe_cmd->u.str,
    		    oe_prm ? " " : "",
    		    oe_prm ? oe_prm->u.str : "");

    	/* Relay message to long commands */
    	err = cmd_process_long(baresip_commands(),
    			       cmd,
    			       str_len(cmd),
    			       NULL, NULL);
    	if (err) {
    		warning("posix: error processing command (%m)\n", err);
    	}
    }
	//ui_input_str((char *)buf->buf);

    debug("\nevent! mq=%d ret=%d",mqdes,ret);

    load_notify(mqpt);

out:
   mem_deref(buf);
   mem_deref(od);
}

int posix_command_init(struct posix_st *st)
{
    struct mq_attr attr;
    struct mbuf *buf;

 	if (!st)
 		return EINVAL;

	conf_get_str(conf_cur(), "posix_cmd_queue_name", st->cmd.qname, POSIX_QUEUE_NAME_SIZE);
	if (strlen(st->cmd.qname)==0)
		return EINVAL;

	st->cmd.size = POSIX_CMD_BUFFER_SIZE;

	/* open the mail queque */
	debug("POSIX QUEUE open!\n");
	st->cmd.mq = mq_open(st->cmd.qname, O_RDONLY|O_NONBLOCK );
	if (st->cmd.mq==(mqd_t)-1) {
		if (errno==ENOENT) {
			attr.mq_flags=O_NONBLOCK;
			attr.mq_maxmsg=10;
			attr.mq_msgsize=st->cmd.size;
			st->cmd.mq = mq_open(st->cmd.qname, O_RDONLY|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &attr );

			if (st->cmd.mq==(mqd_t)-1)
				return errno;
		} else {
			return errno;
		}
	}

    buf = mbuf_alloc(POSIX_CMD_BUFFER_SIZE);

	// flush all queued events
    if (buf) {
       while(get_command(st->cmd.mq,(char *)buf->buf,st->cmd.size)!=0);
       mem_deref(buf);
    }

    load_notify(&st->cmd.mq);

 	return 0;
}

int posix_command_close(struct posix_st *st)
{
	debug("posixque: command close\n");

	if (st) {
		if(st->cmd.mq)
			mq_close(st->cmd.mq);
		if(strlen(st->cmd.qname))
			mq_unlink(st->cmd.qname);
	}

	return 0;
}
