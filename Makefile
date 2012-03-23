
plat ?= a8
test ?= telnet-simplefs-3730
libs += gstreamer-0.10
libs += zlib 
libs +=	gstreamer-rtsp-0.10 
libs +=	gst-rtsp-server-0.10
libs +=	gstreamer-app-0.10
libs +=	gstreamer-rtp-0.10 
release_objs += rtsp gst.sh init.sh

include ${parentsdir}/top.mk

all: test-rtsp

release: rtsp 

test-rtsp: rtsp
	$(call sh, ./test-rtsp.sh)

$(eval $(call single-target,rtsp))
$(eval $(call my-gst-plugin,valve,malve))

lighttpd:
	$(call sh, lighttpd -D -f lighttpd.conf)

test-act:
	cat test.xml | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?act

test-load:
	echo | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?load

test-save:
	echo | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?save

test-cur:
	echo | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?cur

test-upload:
	cat upload.xml | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?upload

test-restart:
	echo | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?restart

test-reset:
	echo | curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?reset

gst-rtsp 264udp: 
	make gst c=$@
 
test-my-plugin:
	$(call sh, gst-launch ddsrc ! fdsink)

vlc-ts:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "udp://@0.0.0.0:1199"

vlc-rtsp:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "rtsp://192.168.1.36:8554/test"

asys2:
	make -C ${parentsdir}/../asys2
	cp ${parentsdir}/../asys2/capfilter.so .
	nc.traditional -u 192.168.0.37 1653 -q 0 <<<"q"

gst: rebuild-gst-ti rebuild-rtsp rebuild-gst-alsasrc malve.so rtsp asys2
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
	rm -rf *.o *.so rtsp *.264 *.ts *.wav *.mp4 *.aac

