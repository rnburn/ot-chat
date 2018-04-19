#!/bin/sh

if [ -z LIGHTSTEP_ACCESS_TOKEN ]
then
  echo "LIGHTSTEP_ACCESS_TOKEN must be set"
  exit -1
fi

envsubst < /init/ot-chat-config.json.in > /etc/ot-chat-config.json

/app/ot-chat --config=/etc/ot-chat-config.json
