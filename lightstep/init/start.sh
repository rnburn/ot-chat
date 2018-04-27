#!/bin/sh

if [ -z LIGHTSTEP_ACCESS_TOKEN ]
then
  echo "LIGHTSTEP_ACCESS_TOKEN must be set"
  exit -1
fi

rm -rf /app/html/*
wget https://raw.githubusercontent.com/lightstep/lightstep-tracer-javascript/258bbe2c8086030a5eb805a593def97658128246/dist/lightstep-tracer.js -P /app/html/js
wget https://raw.githubusercontent.com/lightstep/lightstep-tracer-javascript/7790a87b43b8062b795644c8428918ff9ad6f7cb/examples/browser-trivial/opentracing-browser.js -P /app/html/js
envsubst < /init/index.html.in > /app/html/index.html
envsubst < /init/ot-chat-config.json.in > /etc/ot-chat-config.json

/app/ot-chat --config=/etc/ot-chat-config.json
