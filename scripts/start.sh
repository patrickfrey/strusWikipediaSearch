#!/bin/sh

export ISERVER1="http://127.0.0.1:7184"
export ISERVER2="http://127.0.0.1:7185"
export ISERVER3="http://127.0.0.1:7186"
export ISERVER4="http://127.0.0.1:7187"
export VSERVER1="http://127.0.0.1:7191"

strusWebService -c build/config/config_isrv1.js -V
strusWebService -c build/config/config_isrv2.js -V
strusWebService -c build/config/config_isrv3.js -V
strusWebService -c build/config/config_isrv4.js -V
strusWebService -c build/config/config_vsrv1.js -V

echo "--- Link document storages ---"
curl -d "@build/config/istorage1.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$ISERVER1/storage/istorage"
curl -d "@build/config/istorage2.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$ISERVER2/storage/istorage"
curl -d "@build/config/istorage3.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$ISERVER3/storage/istorage"
curl -d "@build/config/istorage4.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$ISERVER4/storage/istorage"

echo "--- Link vector storage ---"
curl -d "@build/config/vstorage.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$VSERVER1/vstorage/vstorage"
echo "--- Define query analyzer ---"
curl -d "@build/config/qryanalyzer.json" -i -H "Accept: text/plain" -H "Content-Type: application/json; charset=UTF-8" -X PUT "$VSERVER1/qryanalyzer/qryanalyzer"

qryanalyzer.json

echo "--- Get documents ---"
curl -i -H "Accept: application/json" -H "Content-Type: application/json; charset=UTF-8" -X GET  "$ISERVER1/storage/istorage"
