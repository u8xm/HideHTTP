# HideHTTP

HideHTTP est un serveur HTTP/HTTPS minimal en C, sans framework et avec une homepage de secours integree au binaire.

## Compiler

Prerequis: `gcc`, `make`, OpenSSL et `xxd`.

```sh
make
```

Le build transforme `assets/web/index.html` et `assets/web/style.css` en ressources C. Le binaire continue donc a afficher la homepage meme si le repertoire `assets/web/` est absent au lancement.

## Demarrer

```sh
./bin/hidehttp
```

Options principales:

- `--port 8080` definit le port HTTP.
- `--root ./assets/web` definit le repertoire des fichiers du site.
- `--https-port 8443` definit le port HTTPS.
- `--cert server.crt --key server.key` configure les certificats TLS.
- `--tls` active HTTPS avec les certificats configures.

HTTP est active par defaut. Les chemins `/` et `/style.css` utilisent les ressources embarquees si leurs fichiers ne sont pas disponibles sur disque.

## Structure

- `src/`: serveur, reseau, HTTP et TLS.
- `include/`: interfaces publiques des modules.
- `assets/web/`: homepage editable et fichiers web servis.
- `scripts/embed-assets.sh`: generation des ressources C pendant le build.
- `build/`: fichiers generes et objets de compilation, ignores par Git.
- `bin/`: executables construits, ignores par Git.
