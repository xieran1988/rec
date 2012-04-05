
plat ?= a8
test ?= telnet-3730
libs += gstreamer-0.10
libs += zlib 
libs +=	gstreamer-rtsp-0.10 
libs +=	gst-rtsp-server-0.10
libs +=	gstreamer-app-0.10
libs +=	gstreamer-rtp-0.10 
release_files += rtsp gst.sh init.sh lighttpd.conf www

include ${parentsdir}/top.mk

all: rtsp

screen:
	$(call sh, screen -r $c)

sysinit:
	make -C ${parentsdir} poweroff-all
	$(call sh, ./init.sh)
	make screen c=algo
	#xterm -e "make screen c=algo" &
	#xterm -e "make screen c=ser" &

$(eval $(call single-target,rtsp))
$(eval $(call my-gst-plugin,valve,malve))

lighttpd:
	$(call sh, lighttpd -D -f lighttpd.conf)

post := curl -X POST -H 'Content-type: text/xml' -d @- http://192.168.0.37/cgi.sh?

test-act:
	cat test.xml | ${post}$c.act

test-load:
	echo | ${post}$c.load

test-save:
	echo | ${post}$c.save

test-cur:
	echo | ${post}$c.cur

test-upload:
	cat upload.xml | ${post}$c.upload

test-restart:
	echo | ${post}$c.restart

test-reset:
	echo | ${post}$c.reset

gst-rtsp 264udp udp2ser-asys2: 
	make gst c=$@
 
test-my-plugin:
	$(call sh, gst-launch ddsrc ! fdsink)

vlc-ts:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "udp://@0.0.0.0:1199"

vlc-rtsp:
	make -C ${parentsdir}/vlc
	${parentsdir}/vlc/vlc -vvvv "rtsp://192.168.1.36:8554/test"

gst: malve.so rtsp 
	make -C ${parentsdir} rebuild-gst-ti rebuild-rtsp rebuild-gst-alsasrc 
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
	rm -rf *.o *.so rtsp *.264 *.ts *.wav *.mp4 *.aac *.yuv act.xml default.xml cur.xml udp2ser/*

