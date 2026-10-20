# Original update message service

`--updates updates.txt` enables exact-version routes. Each non-comment line is:

```text
kind "identifier" "source-version" "target-version" required "package-path" sha256
```

Example shape (replace the digest with the actual 64 lowercase hex characters):

```text
game "OUTPOST2" "1.2.0.5" "1.3.0.0" 0 "packages/IntPatch.exe" REPLACE_WITH_ACTUAL_SHA256
sigs "Outpost2_US" "3.0.2.0" "3.0.3.0" 1 "packages/SierraNW.exe" REPLACE_WITH_ACTUAL_SHA256
```

This is a text manifest for this C++ server, not the Python research server's
JSON configuration. Fields use C++ quoted-string syntax; paths containing
Windows backslashes must escape them, or use forward slashes. Required is 0
or 1. Lines beginning with `#` are ignored. Package paths are relative to the
manifest. Calculate the digest with `Get-FileHash -Algorithm SHA256`.

Startup validates every route and digest before opening service sockets. It
rejects duplicate kind/identifier/source routes, empty packages, packages over
64 MiB, more than 16 routes, and total loaded content over 128 MiB. Files are
loaded once; restart to change routes. The hash protects against a wrong local
package being configured, not against tampering with the original plaintext
client connection. No client-supplied path is opened.

Game route:

```text
616 → 901
626(identifier, source) → 907(-1, size, required)
617 → 904(size), 905(blocks), 906, close
```

Component route:

```text
616(region, source) → 903(target, size)
617 → 904(size), 905(blocks), 906, 901(lobby endpoint)
626 may then perform its game check on the same validation connection
```

617 is accepted only after a pending offer on that connection. Duplicate or
out-of-order requests close the connection. 905 uses BE32 block length plus
data, at most 512 data bytes per block. Transfer fills a bounded output queue
incrementally, allowing packages larger than that queue. Successful sending
refreshes the download timeout. Component offers use the original required
update path regardless of the manifest's required field.

No-match version 3.0.3.0 clients retain the normal 901/no-update flow. Known
OUTPOST2 game checks without a route receive 907(0,0,0); unknown games receive
902. Unconfigured other SIGS versions close instead of receiving a guessed
version-error payload.

Tests transfer and compare inert binary fixtures, including a package larger
than 1 MiB. They verify optional game and required component offers, continuation,
unsolicited-download rejection, and checksum failure. They never execute a
package. Real SNWVALID download/installation compatibility is still unverified;
staging executables and patch contents must match the original installer path.
