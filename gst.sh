#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=30/1"
cont="contiguousInputFrame=true"
#cont=
prep="TIPrepEncBuf numOutputBufs=3 $cont ! "
prep=
tivid="$prep TIVidenc1 $cont codecName=h264enc engineName=codecServer byteStream=true bitRate=4000000"
udpsink="udpsink host=192.168.1.174 port=1199"
udpsrc="udpsrc port=1199"
sink264file="filesink location=a.264"
tisrc="ticapturesrc video-standard=480p"
n="num-buffers=3"
nn="num-buffers=100"

export DMAI_DEBUG=0

[ "$1" = "tifakecap" ] && {
	debug="ti*:9"
	pipe="$tisrc ! $vidcap ! fakesink"
}
[ "$1" = "ticap" ] && {
	debug="ti*:9"
	pipe="$tisrc ! $vidcap ! filesink location=a.yuv"
}
[ "$1" = "tifake264" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! malve ! $tivid ! fakesink"
}
[ "$1" = "ti264file" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc $nn ! $vidcap ! malve ! $tivid ! $sink264file"
}
[ "$1" = "titsfile" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc $nn ! $vidcap ! malve ! $tivid ! mpegtsmux ! filesink location=a.ts"
#	pipe="$tisrc $nn ! $vidcap ! $tivid ! avimux ! filesink location=a.avi"
}

[ "$1" = "tits" ] && {
	debug="ti*:3,TI*:3,lega*:9"
	parse264="legacyh264parse config-interval=1"
	pipe="$tisrc ! $vidcap ! malve ! $tivid ! $parse264 ! mpegtsmux ! $udpsink"
	pipe="$tisrc ! $vidcap ! malve ! $tivid ! mpegtsmux ! $udpsink"
}

[ "$1" = "playvlcts" ] && {
	vlc -vvvv "udp://@0.0.0.0:1199"
}

[ "$1" = "playts" ] && {
	debug="mpegtsdemux:3,ffmpeg:4"
	pipe="$udpsrc caps=video/mpegts ! mpegtsdemux ! $ticap ! ffdec_h264 ! xvimagesink"
}


[ "$1" = "ti264" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! malve ! $tivid ! $udpsink"
}

[ "$1" = "fakecap" ] && {
	debug="tiv4lsrc:9"
	pipe="tiv4lsrc ! fakesink"
}

[ "$1" = "fake264" ] && {
	debug="tiv4lsrc:4,TI*:0"
	pipe="tiv4lsrc ! $vidcap ! $tivid ! fakesink"
}

[ "$1" = "264file" ] && {
	debug="udpsink:9,tiv4lsrc:3,TI*:0"
	pipe="tiv4lsrc $nn ! $vidcap ! $tivid ! $sink264file"
}
[ "$1" = "264" ] && {
	debug="udpsink:9,tiv4lsrc:9"
	pipe="tiv4lsrc ! $vidcap ! $tivid ! $udpsink"
}
[ "$1" = "play264" ] && {
	debug="ffmpeg:4"
	pipe="$udpsrc caps=$ticap ! ffdec_h264 ! xvimagesink"
}

[ "$1" = "tsfile" ] && {
	debug="mpegtsmux:4"
	pipe="tiv4lsrc num-buffers=190 ! $vidcap ! $tivid ! mpegtsmux ! filesink location=a.ts"
}
[ "$1" = "playtsfile" ] && {
	pipe="filesrc location=a.ts ! video/mpegts ! mpegtsdemux ! ffdec_h264 ! xvimagesink"
}

[ "$1" = "ts" ] && {
	debug="tiv4lsrc:9,mpegtsmux:4"
	pipe="tiv4lsrc ! $vidcap ! $tivid ! mpegtsmux ! $udpsink"
}

#	pipe="tiv4lsrc num-buffers=190 ! $vidcap ! $tivid ! rtph264pay name=pay0 pt=96"
#	pipe="tiv4lsrc num-buffers=190 ! $vidcap ! $tivid ! rtph264pay name=pay0 pt=96 ! fakesink"
echo $pipe
echo $debug
gst-launch --gst-debug="$debug" $pipe 

