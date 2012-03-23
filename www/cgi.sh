#!/bin/sh

notify() { 
	echo $1 | nc -u -c localhost 1653 
}

echo 'Content-Type: text/xml'
echo 

echo $REQUEST_URI | grep -q act && {
	cat > ../act.xml
	notify a
}
echo $REQUEST_URI | grep -q save && {
	notify s
}
echo $REQUEST_URI | grep -q load && {
	notify l
}
echo $REQUEST_URI | grep -q upload && {
	cat > ../config.xml
	notify l
}
echo $REQUEST_URI | grep -q cur && {
	notify c
	sleep 1
	cat ../cur.xml
}
echo $REQUEST_URI | grep -q restart && {
	notify q
}
echo $REQUEST_URI | grep -q reset && {
	notify r
}

