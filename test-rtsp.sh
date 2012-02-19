#!/bin/sh

export GST_DEBUG="rtsp*:9"
vidcap="video/x-raw-yuv,format=(fourcc)UYVY,width=720,height=576,framerate=16/1"
tivid="TIVidenc1 codecName=h264enc engineName=codecServer"
rtpvid="rtph264pay name=pay0 pt=96 "
export PIPELINE="( tiv4lsrc ! $vidcap ! $tivid ! $rtpvid )"
xvid="x264enc ! "
audsrc="audiotestsrc ! audio/x-raw-int,rate=8000 ! "
audsink="alawenc ! rtppcmapay name=pay1 pt=97 " 
./r
exit 

