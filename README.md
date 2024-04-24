# Nostr Connect Hardware Signer

# THIS IS AN ALPHA PROJECT

The code quality is mediocre and needs a massive refactor. Pls help if you have time and can. Better tests, refactoring, would be nice.

The project currently supports only the Lilygo T-Display S3 board and is in alpha state.

You need to be able to use platformio to be able to test this.

Please review, improve and PR.

## How to flash

1. Clone the repo
1. Copy src/config.h.example to src/config.h and update the relevant vars
1. Flash
1. Test with a client that supports NIP46. nostrudel.ninja is a great client for testing this. Press and hold button 1 to get a pairing QR code.
