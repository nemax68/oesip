#
# module.mk
#
# Copyright (C) 2010 Creytiv.com
#

MOD		:= posixque
$(MOD)_SRCS	+= posixque.c
$(MOD)_SRCS	+= cmd.c
$(MOD)_SRCS	+= event.c
$(MOD)_LFLAGS	+= -lrt

include mk/mod.mk
