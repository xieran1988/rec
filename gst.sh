#!/bin/sh

vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=16/1"
tivid="TIVidenc1 codecName=h264enc engineName=codecServer"
mysrc="fdsrc mysrc=1"
v4lsrc="v4l2src device=/dev/video0 always-copy=false"
tsmux="mpegtsmux"
udpsink="udpsink host=192.168.1.174 port=1199"

debug="v4l2src:9,v4l2:9,fdsrc:4,fdsink:4,udpsink:3,mpegtsmux:4"

[ "$1" = "my" ] && {
	pipe="$mysrc ! $vidcap ! $tivid ! $udpsink"
}
[ "$1" = "mytest" ] && {
	pipe="$v4lsrc ! $vidcap ! $tivid ! fdsink mysink=1"
}
[ "$1" = "vlc" ] && {
	pipe="$v4lsrc ! $vidcap ! $tivid ! $tsmux ! $udpsink"
}
[ "$1" = "myvlc" ] && {
	pipe="$mysrc ! $vidcap ! $tivid ! $tsmux ! $udpsink"
}
[ "$1" = "dump" ] && {
	pipe="$v4lsrc num-buffers=1 ! $vidcap ! filesink location=dump"
}
[ "$1" = "mydump" ] && {
	pipe="$mysrc num-buffers=1 ! $vidcap ! filesink location=dump"
}
[ "$1" = "myts" ] && {
	pipe="$mysrc num-buffers=100 ! $vidcap ! $tivid ! $tsmux ! filesink location=a.ts"
	debug="fdsrc:4,mpegtsmux:4"
	debug=
}
[ "$1" = "264" ] && {
	pipe="$v4lsrc num-buffers=190 ! $vidcap ! $tivid ! filesink location=a.264"
}

[ "$1" = "dum" ] && {
	pipe="fdsrc blocksize=829440 num-buffers=100 ! $vidcap ! $tivid ! fakesink"
	pre="./mycam 10 |"
}

echo $pipe

gst-launch --gst-debug="$debug" $pipe 

