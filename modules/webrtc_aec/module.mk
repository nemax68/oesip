#
# module.mk
#
# Copyright (C) 2010 Creytiv.com
#


WEBRTC_PATH	:= ../webrtc_sdk


MOD		:= webrtc_aec

$(MOD)_SRCS	+= aec.cpp
$(MOD)_SRCS	+= encode.cpp
$(MOD)_SRCS	+= decode.cpp

CPPFLAGS	+= -I$(WEBRTC_PATH)/include

LIBS	+= \
	-L$(WEBRTC_PATH)/lib/x64/Debug \
	-lwebrtc_full \
	-lstdc++


include mk/mod.mk
