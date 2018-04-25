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
  && cp index.html /app/html

FROM ubuntu:18.04
RUN  apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
             gettext-base \
             wget
WORKDIR /app
COPY --from=ot-chat-build /app .

# Install vendor tracers
ADD https://363-57146219-gh.circle-artifacts.com/0/liblightstep_tracer_plugin.so /usr/local/lib
ADD https://190-95814681-gh.circle-artifacts.com/0/libzipkin_opentracing_plugin.so /usr/local/lib
ADD https://github.com/rnburn/cpp-client/raw/plugin-store/plugin/libjaegertracing_plugin.so /usr/local/lib

# Add a default init directory that starts with no tracer
ADD init .

EXPOSE 8080

ENTRYPOINT /init/start.sh
