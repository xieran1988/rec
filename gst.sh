#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=24/1"
audcap="audio/x-raw-int,channels=1,width=16,rate=44100"
tienc="TIVidenc1 codecName=h264enc engineName=codecServer byteStream=true"
clisink="multiudpsink clients=192.168.0.100:1199,192.168.0.174:1199"
vidsink="multiudpsink clients=127.0.0.1:1199,127.0.0.1:1200"
audsink="multiudpsink clients=127.0.0.1:1201,127.0.0.1:1202"
vidsrc="udpsrc port=1199 caps=$ticap"
vidsrc2="udpsrc port=1200 caps=$ticap"
audsrc="udpsrc port=1201 caps=$audcap"
audsrc2="udpsrc port=1202 caps=$audcap"
sinkavi="filesink location=a.avi"
sinkts="filesink location=a.ts"
sink264="filesink location=a.264"
tisrc="ticapturesrc video-standard=480p"
tssrc="$vidsrc2 ! m. $audsrc2 ! ffenc_mp2 ! m. mpegtsmux name=m"
n="num-buffers=10"
nn="num-buffers=300"

#export GST_DEBUG_DUMP_DOT_DIR=`pwd`
export DMAI_DEBUG=0

[ "$1" = "tsudp" ] && {
	debug="ti*:2,TI*:2,mpeg*:3"
	pipe="$tisrc ! $vidcap ! $tienc ! mpegtsmux name=m ! $clisink alsasrc ! $audcap ! ffenc_mp2 ! m."
}

[ "$1" = "encdec" ] && {
	debug="ti*:2,TI*:3,mpeg*:3"
	pipe="$tisrc ! $vidcap ! $tienc ! ffdec_h264 ! fakesink"
}

echo $pipe
echo $debug
gst-launch --gst-debug="$debug" $pipe 

