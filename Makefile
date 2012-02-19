
#plat := pc
plat ?= 3730
libs := gstreamer-0.10 zlib \
	gstreamer-rtsp-0.10 gst-rtsp-server-0.10 \
	gstreamer-app-0.10 \
	gstreamer-rtp-0.10 
include $(parentsdir)/top.mk

all: test-rtsp

test-rtsp: r
	$(call targetsh,./test-rtsp.sh)

$(eval $(call single-target,r))
$(eval $(call my-gst-plugin,fdsrc,tiv4l))
$(eval $(call my-gst-plugin,valve,malve))

test-my-plugin:
	$(call targetsh,gst-launch ddsrc ! fdsink)

.PHONY: mycam
mycam:
	make -C ../asys mycam
	cp ../asys/mycam .

vlc-ts:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "udp://@0.0.0.0:1199"

vlc-rtsp:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "rtsp://192.168.1.36:8554/test"

gst: tiv4l.so rebuild-gst-ti rebuild-rtsp malve.so r
	$(call targetsh,./gst.sh $c)

tswin:
	make gst c=tits

inspect:
	$(call targetsh,gst-inspect)


play-a:
	gst-launch playbin uri=file:///${PWD}/a.264

pcgst:
	./gst.sh $c

play-deca:
#	gst-launch filesrc location=a.264 ! h264parse ! ffdec_h264 ! sdlvideosink
	gst-launch udpsrc port=1199 ! h264parse ! ffdec_h264 ! xvimagesink
#	gst-launch --gst-debug="filesink:9" filesrc location=a.264 ! h264parse ! filesink location=/dev/null

clean:
	rm -rf *.o r

