# Weather HTTPS trust bundle

`x509_crt_bundle.bin` contains 122 compact CA subject/public-key entries (56,149
bytes): 121 generated from a system trusted CA PEM bundle, plus GlobalSign Root CA
(R1, `globalsign-root-ca-r1.pem`, valid until 2028-01-28).

GlobalSign R1 is included because some servers (CoinGecko, via Google Trust
Services) send `GTS Root R4` cross-signed by GlobalSign R1 as the last chain
certificate. The ESP32 bundle verifier only checks the issuer of that last
certificate, so without R1 the connection fails with `TLS error -12288`
(`MBEDTLS_ERR_X509_FATAL_ERROR`) even though `GTS Root R4` is in the bundle. Certificate
material is public; it contains no private keys or user credentials.

PlatformIO embeds it into flash; WiFiClientSecure uses the framework's compact
bundle verifier. This avoids loading full PEM root certificates into RAM.
The bundle is committed so firmware builds need no local CA paths or generator.

To add a root to the existing bundle without changing any other entry:

```sh
python3 scripts/build_cert_bundle.py --base data/cert/x509_crt_bundle.bin new-root.pem data/cert/x509_crt_bundle.bin
```

To regenerate after reviewing/updating the system trust store (then add R1 again with `--base`):

```sh
python3 scripts/build_cert_bundle.py /etc/ssl/certs/ca-certificates.crt data/cert/x509_crt_bundle.bin
```

The generator requires Python cryptography. It writes the big-endian entry count,
then sorted subject-name DER/public-key DER records expected by the installed
Arduino ESP32 `esp_crt_bundle.c`. Updating the binary requires recompiling.
Certificate expiry/rotation or new provider roots may require a bundle update;
TLS failures must be diagnosed on hardware, never bypassed with insecure mode.
