
#plat := pc
plat ?= 3530
libs := gstreamer-0.10 zlib \
	gstreamer-rtsp-0.10 gst-rtsp-server-0.10 \
	gstreamer-app-0.10 \
	gstreamer-rtp-0.10 
include $(parentsdir)/top.mk

all: r-test

$(eval $(call single-target,r))
$(eval $(call my-gst-plugin,fdsrc,tiv4l))

test-my-plugin:
	$(call targetsh,gst-launch ddsrc ! fdsink)

.PHONY: mycam
mycam:
	make -C ../asys mycam
	cp ../asys/mycam .

mycam-run: mycam
	$(call targetsh,./mycam 15)

gst: tiv4l.so
	$(call targetsh,./gst.sh $c)

inspect:
	$(call targetsh,gst-inspect)

play-a:
	gst-launch playbin uri=file:///${PWD}/a.264

clean:
	rm -rf *.o r

