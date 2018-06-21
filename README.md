# ot-chat-example

`ot-chat` reconstructs [sockets.io](https://socket.io/get-started/chat/)'s chat example
using [Boost.Beast](https://github.com/boostorg/beast) and instruments it for
distributed tracing.

Examples have been set up with `docker-compose` for LightStep, Jaeger, and
Zipkin. To run, follow these instructions:

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

`ot-chat` demonstrates
1. Pluggable tracing using OpenTracing's dynamic loading API. See `load_tracer` in [configuration.cpp](https://github.com/rnburn/ot-chat/blob/master/src/configuration.cpp#L41).
2. Embedding of the tracer's JSON configuration with the application's configuration. See [configuration.proto](https://github.com/rnburn/ot-chat/blob/master/configuration.proto#L21).
3. Context propagation with JavaScript using a JSON text map encoding. See [message.cpp](https://github.com/rnburn/ot-chat/blob/master/src/message.cpp) and [index.html.in](https://github.com/rnburn/ot-chat/blob/master/lightstep/init/index.html.in).
