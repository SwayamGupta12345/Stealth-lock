Stealth-lock
=====================

Description
-----------------
Stealth-lock is a single-file C++ educational tool that provides simple file/image/text encryption, decryption, and a basic steganography (store a file inside an image) utility. It uses a DJB-like hash of a user password to derive a 64-bit key and applies a repeated-key XOR to data. This project is intended for learning/demos — the cryptography is intentionally simple and not secure for real-world use.

Features
--------
- User login / signup system with built-in example users:
  - admin / admin123
  - guest / guest123
  - john / doe123
  - alice / alice@123
- Passwords are stored as a deterministic DJB-like 64-bit hash and used as the encryption key.
- Image encrypt / decrypt (XOR entire binary file).
- Generic file encrypt / decrypt (XOR).
- Encrypt / decrypt short text via console (Base64-encoded encrypted output).
- Stego: store a file inside an image (appends an encrypted payload plus a small header) and retrieve it given the original image size.
- Console progress bar and simple prompts.
- Outputs are created in the same directory as input files with clear suffixes.

Important security disclaimer
-----------------------------
- The implementation uses a repeated-key XOR cipher derived from a simple DJB-like hash. This is NOT secure for protecting sensitive data.
- Password hashing is not salted and not suitable for authentication in production.
- Use this repository only for education and experimentation. For real security use vetted libraries (e.g., libsodium, OpenSSL, Argon2/Bcrypt for passwords).

Repository contents
-------------------
- shealth_lock.cpp — single C++ source file containing the entire application (UI, crypto helpers, stego, user manager).

Build
-----
Requirements:
- C++17-capable compiler (g++/clang++ supporting std::filesystem).
- Recommended: g++ with -std=c++17

Compile:
- From the repository root run:
  g++ -std=c++17 -O2 shealth_lock.cpp -o shealth_lock

Run:
- ./shealth_lock

High-level usage
----------------

1) Top-level user menu
- 1. Login — enter username and password (predefined users above available).
- 2. Signup — create a new username/password (stored in-memory for current run).
- 3. Exit — quit program.

2) After successful login
- The program asks for the password again to derive the encryption key (64-bit) used for subsequent operations.
- Main menu options:
  1. Encrypt Image
     - Provide path to an image file. Produces a file with suffix _enc (keeps extension) in same directory.
  2. Decrypt Image
     - Provide encrypted image path. Produces filename with suffix _dec and a suggested .jpg extension (if needed).
  3. Encrypt File
     - Provide any file path. Produces filename with suffix _enc and extension .enc in same directory.
  4. Decrypt File
     - Provide encrypted file path. The program will try to strip _enc or .enc when possible and restore original extension where available. Otherwise it writes <filename>_dec.
  5. Encrypt Text
     - Enter a single-line text; output is Base64-encoded encrypted text printed to console. Optionally save to file <original>_enc.txt.
  6. Decrypt Text
     - Input a Base64-encoded encrypted string to decrypt to plaintext. Optionally save to file <original>_dec.txt.
  7. Store File in Image (Stego)
     - Provide a cover image path and a file-to-hide path. The tool copies image bytes, then appends:
       - signature "STEGOSTR" (8 bytes),
       - 8-byte little-endian filename length,
       - filename bytes,
       - and the XOR-encrypted file contents.
     - Output file uses suffix _stego in same directory as the cover image.
     - The program prints the original cover image size (in bytes) — you will need that value for retrieval.
  8. Retrieve File from Image (Stego)
     - Provide image-with-file path and the original image size (in bytes) that was used when storing. The tool seeks to that byte offset, attempts to read the signature and filename metadata, decrypts the appended bytes, and writes recovered_<hiddenFileName> in same directory.
     - If signature check fails, the tool prints a warning but will continue; correct original image size is critical for success.
  9. Logout — returns to the top-level user menu.

File naming and output behavior
-------------------------------
- Image encrypt: input.jpg -> input_enc.jpg (same directory)
- Image decrypt: encrypted input -> input_dec.jpg (forced .jpg if extension ambiguous)
- File encrypt: input.pdf -> input.pdf.enc or input_enc.enc (suffix _enc then .enc)
- File decrypt: tries to reverse _enc or .enc. If it cannot infer original name/extension, it writes <original>_dec
- Text encrypt/decrypt: console Base64 output; optionally saved as <file>_enc.txt or <file>_dec.txt
- Stego store: cover.jpg -> cover_stego.jpg (appends payload). Outputs the original image size used for storage.
- Stego retrieve: writes recovered_<hiddenFileName>

Internal details (brief)
------------------------
- Key derivation: customHash(password) — a DJB-like hash seeded with 5381 and multiplies by 33 while adding each byte. Returns unsigned long long (64-bit).
- XOR operation: the 64-bit key is used cyclically over each byte (key bytes extracted by shifting key by 0..56 bits and masking).
- Text payloads are Base64-encoded for safe textual transmission.
- Uses std::filesystem for path/size operations, plus i/o streams.

Common errors & troubleshooting
-------------------------------
- "Input ... does not exist" — verify path and permissions.
- "Failed to open files" — ensure you have read/write permissions and destination is writable.
- Decryption produces garbage — ensure you used the same password (key) used during encryption and that you selected the correct file. For stego retrieval you must supply the exact original cover image size (in bytes).
- Signature missing on stego retrieval — likely wrong original image size or file not created by this tool.

Recommended improvements (if you plan to extend)
-----------------------------------------------
- Replace XOR + custom hash with authenticated encryption (e.g., AES-GCM or XChaCha20-Poly1305).
- Use a secure password hashing function (Argon2, bcrypt, scrypt) with per-user salt.
- Store users persistently (with proper secure storage and salting).
- Add block-based processing for very large files and memory efficiency.
- Add CLI flags for non-interactive usage (scriptable operations).

Example quick session
---------------------
1) Build:
   g++ -std=c++17 -O2 shealth_lock.cpp -o shealth_lock

2) Run:
   ./shealth_lock

3) Login:
   username: admin
   password: admin123

4) Choose "3. Encrypt File", enter path to file; output will be created in same folder with .enc suffix.

License & credits
-----------------
- This project is an educational demo. The repository owner is SwayamGupta12345.
- No license file is included — add a LICENSE file if you want to apply a specific license. If you want a suggestion, MIT is commonly used for small demos.

If you want, I can:
- Produce a ready-to-use README.md file text you can paste into the repository.
- Suggest and produce a small refactor (split into multiple compilation units) or add a safer crypto-backed implementation sketch.
