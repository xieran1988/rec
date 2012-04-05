#!/bin/sh

grep release_zc /proc/cmdline && {
	screen -dmS algo ./gst.sh 264udp 
#	screen -dmS ser ./gst.sh udp2ser-asys2
	screen -dmS ser ./gst.sh udp2ser
	screen -dmS rtsp ./gst.sh gst-rtsp 
	lighttpd -f lighttpd.conf
}

