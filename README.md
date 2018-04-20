# ot-chat-example

`ot-chat` reconstructs [sockets.io](https://socket.io/get-started/chat/)'s chat example
using [Boost.Beast](https://github.com/boostorg/beast). It instruments the application
for distributed tracing and demonstrates OpenTracing's pluggable tracing API.

With it, the same binary can be deployed for any tracer. Examples have been set up with
`docker-compose` for LightStep, Jaeger, and Zipkin. To run, follow these instructions:

```sh
# LightStep
cd lightstep
export LIGHTSTEP_ACCESS_TOKEN=<your access token>
docker-compose up

# Jaeger
cd jaeger
docker-compose up

# Zipkin
cd zipkin
docker-compose up
```

Then visit [`http://localhost:8080`](http://localhost:8080).
