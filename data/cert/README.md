# Weather HTTPS trust bundle

`x509_crt_bundle.bin` contains 121 compact CA subject/public-key entries (55,762
bytes), generated from the local system trusted CA PEM bundle. Certificate
material is public; it contains no private keys or user credentials.

PlatformIO embeds it into flash; WiFiClientSecure uses the framework's compact
bundle verifier. This avoids loading full PEM root certificates into RAM.
The bundle is committed so firmware builds need no local CA paths or generator.

To regenerate after reviewing/updating the system trust store:

```sh
python3 scripts/build_cert_bundle.py /etc/ssl/certs/ca-certificates.crt data/cert/x509_crt_bundle.bin
```

The generator requires Python cryptography. It writes the big-endian entry count,
then sorted subject-name DER/public-key DER records expected by the installed
Arduino ESP32 `esp_crt_bundle.c`. Updating the binary requires recompiling.
Certificate expiry/rotation or new provider roots may require a bundle update;
TLS failures must be diagnosed on hardware, never bypassed with insecure mode.
