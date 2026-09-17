# HideHTTP

A minimal HTTP/HTTPS server written in C with an embedded homepage.

## Usage

Requirements: `gcc`, `make`, OpenSSL and `xxd`.

```sh
make
./bin/hidehttp
```

HTTP is enabled by default. To enable HTTPS:

```sh
./bin/hidehttp --tls --cert server.crt --key server.key
```

Useful options: `--port`, `--root`, `--https-port`, `--cert`, `--key`.
