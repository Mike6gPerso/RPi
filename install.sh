#! /bin/bash

serverFolder=/home/pi/SmartHome/server/
webFolder=/var/www/
echo \*\* Init... \*\*

echo \*\* updating JS \*\*
#JS goes to /home/pi/SmartHome/server/
service node stop
cp ./JS/server.js $serverFolder
service node start

echo \*\* updating C code \*\*
 #compile !
cd C
gcc -g -o vw vw.c -lpigpio -lpthread -lrt -lsqlite3
service vw stop
cp ./vw $serverFolder

cd ..
#C goes to /home/pi/SmartHome/server/

echo \*\* upgrading DB \*\*
#SQL should update db in /home/pi/SmartHome/server/
#DB should now its current version

echo \*\* upgrading Web \*\*
#web goes to /var/www/...

echo \*\* restart service \*\*
service vw start

echo \*\* End \*\*
