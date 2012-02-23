#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=30/1"
ticap="video/x-h264,width=720,height=576,framerate=24/1"
cont="contiguousInputFrame=true"
#cont=
#prep="TIPrepEncBuf numOutputBufs=3 $cont ! "
tivid="TIVidenc1 $cont codecName=h264enc engineName=codecServer byteStream=true"
timpgvid="TIVidenc1 $cont codecName=mp4venc engineName=codecServer"
imgsize=829440
ip=192.168.0.174
#ip=192.168.1.100
udpsink="udpsink host=$ip port=1199"
localudpsink="udpsink port=1199"
multiudpsink="multiudpsink clients=127.0.0.1:1199,127.0.0.1:1200"
udpsrc="udpsrc port=1199"
udpsrc2="udpsrc port=1200"
sink264file="filesink location=a.264"
sinkavi="filesink location=a.avi"
sinkmp4="filesink location=a.mp4"
tisrc="ticapturesrc video-standard=480p"
n="num-buffers=10"
nn="num-buffers=100"
audcap="audio/x-raw-int,rate=64000"

export DMAI_DEBUG=0

[ "$1" = "arec" ] && {
	arecord -f dat -d 10 -D hw:0,1 a.wav
	exit
}

[ "$1" = "ce" ] && {
	export CE_TRACE="*=01234567" 
	debug="TI*:3"
	vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=1280,height=720,framerate=30/1"
	pipe="videotestsrc ! $vidcap ! $tivid ! fakesink"
}

[ "$1" = "mp4" ] && {
	pipe="$tisrc ! $vidcap ! $timpgvid ! $sinkmp4"
}

[ "$1" = "mix" ] && {
	debug="TI*:3,ti*:3,alsa*:5,audio*:3,ff*:3"
		#alsasrc ! TIAudenc1 codecName=g711enc engineName=codecServer ! mux. 
	pipe="\
		audiotestsrc ! $audcap ! ffenc_aac ! mux. \
		$tisrc ! $vidcap ! malve ! $tivid ! mux. \
		mpegtsmux name=mux ! filesink location=a.ts
		"
	pipe="alsasrc device=default ! $audcap ! ffenc_aac ! filesink location=a.aac"
}

[ "$1" = "save" ] && {
	pipe="$udpsrc2 caps=$ticap ! avimux ! $sinkavi"
}

[ "$1" = "localudp" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! $tivid ! $multiudpsink"
}

[ "$1" = "rtsp" ] && {
	export GST_DEBUG="rtsp*:9"
	rtpvid="rtph264pay name=pay0 pt=96 "
	export PIPELINE="( $udpsrc caps=$ticap ! $rtpvid )"
	xvid="x264enc ! "
	audsrc="audiotestsrc ! audio/x-raw-int,rate=8000 ! "
	audsink="alawenc ! rtppcmapay name=pay1 pt=97 " 
	./rtsp
	exit 
}

[ "$1" = "tits" ] && {
	debug="ti*:3,TI*:3,lega*:9"
	pipe="$udpsrc2 caps=$ticap ! mpegtsmux ! $udpsink"
}
[ "$1" = "tifakecap" ] && {
	debug="ti*:9"
	pipe="$tisrc ! $vidcap ! fakesink"
}
[ "$1" = "tiyuv" ] && {
	debug="ti*:9"
	pipe="$tisrc ! $vidcap ! $udpsink"
}
[ "$1" = "playyuv" ] && {
	pipe="$udpsrc caps=$vidcap ! xvimagesink"
}
[ "$1" = "vidtest264" ] && {
	pipe="videotestsrc ! $vidcap ! malve ! $tivid ! $udpsink"
}
[ "$1" = "ti264" ] && {
	debug="ti*:3,TI*:3"
	pipe="$tisrc ! $vidcap ! $tivid ! $udpsink"
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

[ "$1" = "playvlcts" ] && {
	vlc -vvvv "udp://@0.0.0.0:1199"
}

[ "$1" = "playts" ] && {
	debug="mpegtsdemux:3,ffmpeg:4"
	pipe="$udpsrc caps=video/mpegts ! mpegtsdemux ! $ticap ! ffdec_h264 ! xvimagesink"
}

[ "$1" = "fakecap" ] && {
	debug="ti*:9"
	pipe="$tisrc ! $vidcap ! fakesink"
}

[ "$1" = "fakev4l" ] && {
	debug="v4l2*:9"
	pipe="v4l2src device=/dev/video0 ! $vidcap ! fakesink"
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

