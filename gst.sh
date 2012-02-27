#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=24/1"
audcap="audio/x-raw-int,channels=2,width=16,rate=48000"
tivid="TIVidenc1 codecName=h264enc engineName=codecServer byteStream=true"
clisink="multiudpsink clients=192.168.0.100:1199,192.168.0.174:1199"
vidsink="multiudpsink clients=127.0.0.1:1199,127.0.0.1:1200"
audsink="multiudpsink clients=127.0.0.1:1201,127.0.0.1:1202"
vidsrc="udpsrc port=1199"
vidsrc2="udpsrc port=1200"
audsrc="udpsrc port=1201"
audsrc2="udpsrc port=1202"
sinkavi="filesink location=a.avi"
sinkts="filesink location=a.ts"
tisrc="ticapturesrc video-standard=480p"
tssrc="$vidsrc2 caps=$ticap ! m. $audsrc2 caps=$audcap ! ffenc_mp2 ! m. mpegtsmux name=m"
n="num-buffers=10"
nn="num-buffers=100"

#export GST_DEBUG_DUMP_DOT_DIR=`pwd`
export DMAI_DEBUG=0

[ "$1" = "vidudp" ] && {
#	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! $tivid ! $vidsink"
}

[ "$1" = "aududp" ] && {
	pipe="alsasrc ! $audcap ! $audsink"
}

[ "$1" = "gst-rtsp" ] && {
	export GST_DEBUG="rtsp*:9,ff*:9"
	rtpvid="rtph264pay name=pay0 pt=96 "
#	audsrc="alsasrc ! audio/x-raw-int,rate=8000 ! "
	rtpaud="ffenc_mp2 ! rtppcmapay name=pay1 pt=97 " 
	export PIPELINE="( $vidsrc caps=$ticap ! $rtpvid $audsrc caps=$audcap ! $rtpaud )"
	./rtsp
	exit 
}

[ "$1" = "tsfile" ] && {
	debug="ffmpeg*:2,ti*:3,TI*:3,lega*:9"
	pipe="$tssrc ! $sinkts"
}

[ "$1" = "avifile" ] && {
	debug="*:3,ti*:3,TI*:3,lega*:9"
	pipe="$tssrc ! $sinkts"
}

[ "$1" = "tsudp" ] && {
	debug="ti*:3,TI*:3,lega*:9"
	pipe="$tssrc ! $clisink"
}

echo $pipe
echo $debug
gst-launch --gst-debug="$debug" $pipe 

