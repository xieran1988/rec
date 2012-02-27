#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=24/1"
audcap="audio/x-raw-int,channels=1,width=16,rate=8000"
tivid="TIVidenc1 codecName=h264enc engineName=codecServer byteStream=true"
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

[ "$1" = "vidudp" ] && {
#	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! $tivid ! $vidsink"
}

[ "$1" = "aududp" ] && {
	pipe="alsasrc ! $audcap ! $audsink"
}

[ "$1" = "audtest" ] && {
	debug="faac:2"
	audcap="audio/x-raw-int,channels=2,depth=16,width=16,rate=44100" # mp2 support 48000
	audsrc="alsasrc ! $audcap "
	pipe="$audsrc ! faac profile=2 bitrate=96000 ! fakesink"
}

[ "$1" = "gst-rtsp" ] && {
#	export GST_DEBUG="rtsp*:9,ff*:9,rtp*:9,ala*:9"
	rtpvid="rtph264pay name=pay0 pt=96 "
#	audsrc="alsasrc ! audio/x-raw-int,rate=8000 ! "
	rtpaud="alawenc ! rtppcmapay name=pay1 pt=97 " 
	vidsrc="$tisrc ! $vidcap ! $tivid "
	audsrc="alsasrc ! $audcap "
	export PIPELINE="( $vidsrc ! $rtpvid $audsrc ! $rtpaud )"
	./rtsp
	exit 
}

[ "$1" = "tsfile" ] && {
	debug="ffmpeg*:2,ti*:3,TI*:3,lega*:9,mpeg*:3"
	#audcap="audio/x-raw-int,channels=2,width=16,rate=96000" # mp2 support 48000
	audcap="audio/x-raw-int,channels=2,width=16,rate=44100" # mp2 support 48000
	vidsrc="$tisrc ! $vidcap ! $tivid "
	audsrc="alsasrc ! $audcap ! faac profile=2 bitrate=96000 "
	audsrc="alsasrc ! $audcap ! ffenc_mp2 bitrate=192000 "
	pipe="$vidsrc ! m. $audsrc ! m. mpegtsmux name=m ! filesink location=/hd/a.ts buffer-mode=2"
	#pipe="$tssrc ! $sinkts"
}

[ "$1" = "264file" ] && {
	debug="ffmpeg*:2,ti*:3,TI*:3,lega*:9"
	pipe="$tisrc $nn ! $vidcap ! $tivid ! $sink264"
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

