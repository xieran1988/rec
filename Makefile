
#plat := pc
plat ?= 3530
libs := gstreamer-0.10 zlib gstreamer-rtsp-0.10 gst-rtsp-server-0.10 gstreamer-app-0.10
include $(parentsdir)/top.mk

$(eval $(call single-target,r))

all: r-test

gst:
	make -C ../asys mycam
	cp ../asys/mycam .
	$(call targetsh,./gst.sh $c)

clean:
	rm -rf *.o r

