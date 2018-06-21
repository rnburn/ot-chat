FROM ubuntu:18.04 as ot-chat-build
ADD . /src
WORKDIR /src
RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
             build-essential \
             cmake \
             curl \
             ca-certificates \
             gnupg \
             openjdk-8-jdk \
# Install bazel
  && echo "deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8" \
          | tee /etc/apt/sources.list.d/bazel.list \
  && curl https://bazel.build/bazel-release.pub.gpg | apt-key add - \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              bazel \
  && apt-get upgrade -y bazel \
# Build chat-server
  && cd /src \
  && bazel build //:ot-chat \
  && mkdir -p /app/html \
  && cp bazel-bin/ot-chat /app \
  && cp /src/html/index.html /app/html

FROM ubuntu:18.04
RUN  apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
             gettext-base \
             ca-certificates \
             wget
WORKDIR /app
COPY --from=ot-chat-build /app .

# Install vendor tracers
RUN mkdir /tracers
ADD https://github.com/lightstep/lightstep-tracer-cpp/releases/download/v0.7.1/linux-amd64-liblightstep_tracer_plugin.so.gz /tracers
ADD https://github.com/rnburn/zipkin-cpp-opentracing/releases/download/v0.4.0/linux-amd64-libzipkin_opentracing_plugin.so.gz /tracers
ADD https://github.com/jaegertracing/jaeger-client-cpp/releases/download/v0.4.1/libjaegertracing_plugin.linux_amd64.so /tracers
RUN  cat /tracers/linux-amd64-liblightstep_tracer_plugin.so.gz \
      | gunzip -c > /usr/local/lib/liblightstep_tracer_plugin.so \
  && cat /tracers/linux-amd64-libzipkin_opentracing_plugin.so.gz \
      | gunzip -c > /usr/local/lib/libzipkin_opentracing_plugin.so \
  && mv /tracers/libjaegertracing_plugin.linux_amd64.so /usr/local/lib/libjaegertracing_plugin.so \
  && rm -rf /tracers

# Add a default init directory that starts with no tracer
ADD init .

EXPOSE 8080

ENTRYPOINT /init/start.sh
