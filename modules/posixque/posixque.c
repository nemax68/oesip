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

/* We only allow one instance */
struct posix_st *posix_ui_state;

static void ui_destructor(void *arg)
{
	//struct ui_st *st = arg;
	(void)arg;
}

static int ui_alloc(struct posix_st **stp)
{
	struct posix_st *st;
	int err=0;

 	if (!stp)
 		return EINVAL;

	st = mem_zalloc(sizeof(*st),ui_destructor);
	if (!st)
		return ENOMEM;

	if (posix_command_init(st)!=0) {
		err = EINVAL;
		goto out;
	}

	if (posix_event_init(st)!=0) {
		err = EINVAL;
		goto out;
	}

	info("posixque: listening on queue=%d name:%s\n", st->cmd.mq, st->cmd.qname);

out:
 	if (err)
 		mem_deref(st);
 	else
 		*stp = st;

 	return err;
}


static int output_handler(const char *str)
{
	printf("[%s]",str);
	return 0;
}


static struct ui ui_posixque = {
	.name = "posixque",
	.outputh = output_handler
};

static int module_init(void)
{
	int err;

	posix_ui_state=NULL;

	err = ui_alloc(&posix_ui_state);
	if (err)
		return err;

	ui_register(baresip_uis(), &ui_posixque);

	return 0;
}


static int module_close(void)
{

	ui_unregister(&ui_posixque);

	posix_command_close(posix_ui_state);

	posix_event_close(posix_ui_state);

	posix_ui_state = mem_deref(posix_ui_state);

	debug("posixque: module close\n");

	return 0;
}


const struct mod_export DECL_EXPORTS(posixque) = {
	"posixque",
	"ui",
	module_init,
	module_close
};
