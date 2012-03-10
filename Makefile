
plat ?= a8
test ?= telnet-simplefs-3730
libs += gstreamer-0.10
libs += zlib 
libs +=	gstreamer-rtsp-0.10 
libs +=	gst-rtsp-server-0.10
libs +=	gstreamer-app-0.10
libs +=	gstreamer-rtp-0.10 

include ${parentsdir}/top.mk

all: test-rtsp

test-rtsp: rtsp.c
	$(call sh, ./test-rtsp.sh)

$(eval $(call single-target,rtsp))
$(eval $(call my-gst-plugin,valve,malve))

vidudp aududp gst-rtsp tsudp tsfile arec tstest test vidtcp encdec:  
	make gst c=$@
 
test-my-plugin:
	$(call sh, gst-launch ddsrc ! fdsink)

vlc-ts:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "udp://@0.0.0.0:1199"

vlc-rtsp:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "rtsp://192.168.1.36:8554/test"

gst: rebuild-gst-ti rebuild-rtsp rebuild-gst-alsasrc malve.so rtsp
	$(call sh, ./gst.sh $c)

tswin:
	make gst c=tits

inspect:
	$(call sh,gst-inspect)

play-a:
	gst-launch playbin uri=file:///${PWD}/a.264

pcgst:
	./gst.sh $c

play-deca:
#	gst-launch filesrc location=a.264 ! h264parse ! ffdec_h264 ! sdlvideosink
	gst-launch udpsrc port=1199 ! h264parse ! ffdec_h264 ! xvimagesink
#	gst-launch --gst-debug="filesink:9" filesrc location=a.264 ! h264parse ! filesink location=/dev/null

clean:
	rm -rf *.o *.so rtsp

