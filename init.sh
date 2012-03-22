#!/bin/sh

screen -dmS tsudp ./gst.sh tsudp 
screen -dmS rtsp ./gst.sh gst-rtsp 
screen -dmS ser ./gst.sh udp2ser
ifconfig eth0 192.168.0.37 up
