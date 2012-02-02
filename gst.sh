#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=60/1"
ticap="video/x-h264,width=720,height=576,framerate=60/1"
cont="contiguousInputFrame=true"
cont=
prep="TIPrepEncBuf numOutputBufs=3 $cont ! "
prep=
tivid="$prep TIVidenc1 $cont codecName=h264enc engineName=codecServer "
udpsink="udpsink host=192.168.1.174 port=1199"
udpsrc="udpsrc port=1199"
n="num-buffers=3"

[ "$1" = "fakecap" ] && {
	debug="tiv4lsrc:9"
	pipe="tiv4lsrc ! fakesink"
}

[ "$1" = "fake264" ] && {
	debug="tiv4lsrc:4,TI*:0"
	pipe="tiv4lsrc ! $vidcap ! $tivid ! fakesink"
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
[ "$1" = "playts" ] && {
	debug="mpegtsdemux:3,ffmpeg:4"
	pipe="$udpsrc caps=video/mpegts ! mpegtsdemux ! $ticap ! ffdec_h264 ! xvimagesink"
}

[ "$1" = "ti" ] && {
	debug="ti*:9"
	pipe="tiv4lsrc buffers=190 ! $vidcap ! $tivid ! filesink location=a.264"
	pipe="tiv4lsrc ! $vidcap ! $tivid ! fakesink"
#	pipe="tiv4lsrc num-buffers=190 ! $vidcap ! $tivid ! rtph264pay name=pay0 pt=96"
#	pipe="tiv4lsrc num-buffers=190 ! $vidcap ! $tivid ! rtph264pay name=pay0 pt=96 ! fakesink"
}
echo $pipe
echo $debug
gst-launch --gst-debug="$debug" $pipe 

