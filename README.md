# Nostr Connect Hardware Signer

### ⚠️ **Alpha Version** ⚠️  
This project is in its early stages, and contributions are welcome! We’d love help with testing, restructuring, and refactoring to make it better.

---

### Overview

Currently, this project supports the **Lilygo T-Display S3** board, which can be purchased on [AliExpress](https://www.aliexpress.com/item/1005004898948906.html). Please note that it's still in alpha, so expect ongoing improvements.

### Requirements

- **PlatformIO**: You’ll need PlatformIO to build, configure, and flash the board.
  
Feel free to **use**, **review**, and **improve** the project. Pull requests are always appreciated!

---

### Flashing Instructions

1. **Clone the repository.**
2. **Configure your settings**:  
   Copy `src/config.h.example` to `src/config.h` and update the necessary variables.
3. **Flash the firmware** onto the Lilygo T-Display S3.
4. **Test your setup**:  
   Use a client that supports NIP46 (like [nostrudel.ninja](https://nostrudel.ninja/)) to test. Press and hold Button 1 to display a pairing QR code.

---

### Roadmap & To-Do List

- [ ] Support for multiple `nsecs` per device
- [ ] Add a stats screen (e.g., notes signed, time online)
- [ ] Implement a menu system
- [ ] Add a PIN code unlock on startup
- [ ] Create a web-based installer
- [ ] Develop a web configurator
- [ ] Build a notification service for remote ACK of signing requests
