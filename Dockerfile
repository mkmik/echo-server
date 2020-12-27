FROM mkmik/debian-base-buildpack:buster-r1 as build

RUN mkdir -p /src
WORKDIR /src

COPY demo.c Makefile /src/

RUN make

FROM bitnami/minideb

COPY --from=build /src/demo /usr/bin/demo

ENTRYPOINT [ "/usr/bin/demo" ]

EXPOSE 1234
