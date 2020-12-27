# Simple echo server

This is a simple TCP echo server.

This repo is part of a security excercise, looking at this project in isolation
might not make sense to you casual reader.

## Build

Build it with:

```
$ docker build -t demo .
```

run with:

```
$ docker run --init --rm -ti -p 1234:1234 demo
```

## Use

Connect to port 1234 and type some lines and see them come back

```
$ nc localhost 1234
foo
foo
bar
bar
```

