#!/bin/sh

cmd() {
	echo $REQUEST_URI | grep -q $1
}

notify() { 
	echo $1 | nc -u -c localhost $port
}

echo 'Content-Type: text/xml'
echo 

cmd udp2ser && { dir=../udp2ser; port=1654; }
cmd algo && { dir=../; port=1653; }
[ "$dir" = "" ] && exit

cmd act && {
	echo port: $port
	cat > $dir/act.xml
	notify a
}
cmd save && {
	notify s
}
cmd load && {
	notify l
}
cmd upload && {
	cat > $dir/config.xml
	notify l
}
cmd cur && {
	notify c
	sleep 1
	cat $dir/cur.xml
}
cmd restart && {
	notify q
}
cmd reset && {
	notify r
}

