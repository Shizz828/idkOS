"""Generate the Arduino ESP32 compact CA bundle from a trusted PEM bundle.
Run with a Python environment containing cryptography. The committed binary
lets normal firmware builds run without this generator or host certificate paths.
Format matches framework WiFiClientSecure/src/esp_crt_bundle.c.
"""
import argparse
import re
import struct
from pathlib import Path
from cryptography import x509
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

parser = argparse.ArgumentParser()
parser.add_argument("pem")
parser.add_argument("output")
args = parser.parse_args()
entries = {}
for pem in re.findall(rb"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", Path(args.pem).read_bytes(), re.S):
    cert = x509.load_pem_x509_certificate(pem)
    subject = cert.subject.public_bytes()
    key = cert.public_key().public_bytes(Encoding.DER, PublicFormat.SubjectPublicKeyInfo)
    entries[subject] = key
bundle = struct.pack(">H", len(entries))
for subject, key in sorted(entries.items()):
    bundle += struct.pack(">HH", len(subject), len(key)) + subject + key
Path(args.output).write_bytes(bundle)
print(f"Wrote {len(entries)} CA entries, {len(bundle)} bytes")
