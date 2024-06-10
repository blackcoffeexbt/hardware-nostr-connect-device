# Nostr Connect Hardware Signer

# THIS IS AN ALPHA PROJECT

Pls contribute if you have time and are able. Better tests, restuvturing and refactoring, would be nice.

The project currently supports only the Lilygo T-Display S3 board and is in alpha state.

The Lilygo T-Display S3 can be bought for around 9GBP / 11 EUR / 11 USD on AliExpress - https://www.aliexpress.com/item/1005004898948906.html

You need to be able to use platformio to be able to test this.

Please review, improve and PR.

## How to flash

1. Clone the repo
1. Copy src/config.h.example to src/config.h and update the relevant vars
1. Flash
1. Test with a client that supports NIP46. nostrudel.ninja is a great client for testing this. Press and hold button 1 to get a pairing QR code.


# Todo

- [ ] Add a stats screen (notes signed, time online etc)
- [ ] Add a menu system
- [ ] Pin code to unlock on startup
- [ ] Web installer
- [ ] Web configurator
