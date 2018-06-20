#!/bin/sh

if [ -z LIGHTSTEP_ACCESS_TOKEN ]
then
  echo "LIGHTSTEP_ACCESS_TOKEN must be set"
  exit -1
fi

rm -rf /app/html/*
wget https://raw.githubusercontent.com/lightstep/lightstep-tracer-javascript/35fa9583a5e94470f61e6b087226712ed1353da1/dist/lightstep-tracer.js -P /app/html/js
wget https://raw.githubusercontent.com/lightstep/lightstep-tracer-javascript/35fa9583a5e94470f61e6b087226712ed1353da1/examples/browser-trivial/opentracing-browser.min.js -P /app/html/js
envsubst < /init/index.html.in > /app/html/index.html
envsubst < /init/ot-chat-config.json.in > /etc/ot-chat-config.json

/app/ot-chat --config=/etc/ot-chat-config.json
