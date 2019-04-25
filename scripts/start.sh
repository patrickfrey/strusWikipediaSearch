#!/bin/sh

ISERVER1="http://127.0.0.1:7184"
ISERVER2="http://127.0.0.1:7185"
ISERVER3="http://127.0.0.1:7186"
ISERVER4="http://127.0.0.1:7187"
VSERVER1="http://127.0.0.1:7191"

strusWebService -c config.irsv1.js -V
strusWebService -c config.irsv2.js -V
strusWebService -c config.irsv3.js -V
strusWebService -c config.irsv4.js -V
strusWebService -c config.vrsv1.js -V

echo "--- Create storages ---"
curl -d "@examples/istorage1.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER1/storage/istorage"
curl -d "@examples/istorage2.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER2/storage/istorage"
curl -d "@examples/istorage3.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER3/storage/istorage"
curl -d "@examples/istorage4.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$SERVER4/storage/istorage"

