#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576"
vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=24/1"
audcap="audio/x-raw-int,channels=1,width=16,rate=44100"
tienc="TIVidenc1 codecName=h264enc engineName=codecServer byteStream=true genTimeStamps=false "
#tienc="TIVidenc1 codecName=mpeg4enc engineName=codecServer byteStream=true "
clisink="multiudpsink clients=192.168.0.100:1199,192.168.0.174:1199,127.0.0.1:1204,127.0.0.1:1209,192.168.0.103:1199"
#clisink="multiudpsink clients=127.0.0.1:1204"
#clisink="multiudpsink clients=192.168.0.100:1199"
tssrc="udpsrc port=1204 caps=video/mpegts"
sinkavi="filesink location=a.avi"
sinkts="filesink location=a.ts"
sink264="filesink location=a.264"
tisrc="ticapturesrc video-standard=480p"
#tisrc="ticapturesrc video-standard=pal"
#tisrc="ticapturesrc video-standard=ntsc"
n="num-buffers=10"
nn="num-buffers=300"

#export GST_DEBUG_DUMP_DOT_DIR=`pwd`/dot
export DMAI_DEBUG=0

[ "$1" = "test" ] && {
	debug="ti*:3"
	pipe="$tisrc ! $vidcap ! fakesink"
}

[ "$1" = "udp2ser" ] && {
	pipe="udpsrc port=1200 ! filesink buffer-mode=2 location=/dev/ttyS2"
}

[ "$1" = "tsudp" ] && {
	debug="ti*:5,TI*:5,mpeg*:3"
	pipe="$tisrc ! $vidcap ! $tienc ! mpegtsmux name=m ! fakesink alsasrc ! $audcap ! ffenc_mp2 ! m."
	pipe="$tisrc ! $vidcap ! $tienc ! $clisink "
	#udpsink host=127.0.0.1 port=1204"
}

[ "$1" = "264udp" ] && {
	debug="malve:9,TI*:3"
	pipe="$tisrc ! $vidcap ! $tienc ! $clisink"
}

[ "$1" = "aac" ] && {
#	pipe="alsasrc ! $audcap ! ffenc_aac bitrate=96000 ! fakesink"
	pipe="alsasrc ! $audcap ! faac shortctl=2 ! fakesink"
}

[ "$1" = "gst-rtsp" ] && {
#	p="$tssrc ! mpegtsdemux name=m "
#	p="$p m. ! video/x-h264 ! queue ! rtph264pay name=pay0 pt=96 "
#	p="$p m. ! audio/mpeg ! queue ! rtpmpapay name=pay1 pt=97 "
#	p="$tisrc ! $vidcap ! $tienc ! rtph264pay name=pay0 pt=96"
	p="udpsrc port=1204 caps=video/x-h264 ! rtph264pay name=pay0 pt=96"
	GST_DEBUG="*:3" PIPELINE="$p" ./rtsp
#	pipe="$p"
	exit
}

[ "$1" = "encdec" ] && {
	debug="ti*:2,TI*:3,mpeg*:3"
	pipe="$tisrc ! $vidcap ! $tienc ! ffdec_h264 ! fakesink"
}

echo $pipe
echo $debug
gst-launch --gst-debug="$debug" $pipe 

