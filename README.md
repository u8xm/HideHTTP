# HideHTTP

A minimal HTTP/HTTPS server written in C with a homepage embedded directly in the binary.

## Usage

Requirements: `gcc`, `make` and OpenSSL.

```sh
make
./bin/hidehttp
```

HTTP is enabled by default. To enable HTTPS:

```sh
./bin/hidehttp --tls --cert server.crt --key server.key
```

Useful options: `--port`, `--root`, `--https-port`, `--cert`, `--key`.

The built-in homepage works even when `assets/` is not present.
