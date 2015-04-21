#! /bin/bash

serverFolder=/home/pi/SmartHome/server/
webFolder=/var/www/
echo \*\* Init... \*\*

mkdir -p $serverFolder

echo \*\* updating JS \*\*
#JS goes to /home/pi/SmartHome/server/
service nodeServer stop
cp ./JS/server.js $serverFolder
service nodeServer start

echo \*\* updating C code \*\*
 #compile !
cd C
gcc -g -o RF_Receiver RF_Receiver.c -lpigpio -lpthread -lrt -lsqlite3 linkedList.c
service RF_Receiver stop
cp ./RF_Receiver $serverFolder

cd ..
#C goes to /home/pi/SmartHome/server/

echo \*\* upgrading DB \*\*
#SQL should update db in /home/pi/SmartHome/server/
#DB should now its current version

echo \*\* upgrading Web \*\*
#web goes to /var/www/...

echo \*\* restart service \*\*
service RF_Receiver start

echo \*\* End \*\*
