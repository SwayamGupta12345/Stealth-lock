#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <cctype>
#include <limits>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::map;
using std::ofstream;
using std::size_t;
using std::string;
using std::stringstream;
using std::uint64_t;
using std::uint8_t;
using std::vector;

static const string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
// ============================================================================
// Helper utilities (file paths, progress, prompts)
// ============================================================================

static void waitShort()
{
    // small convenience pause to let user read messages; non-blocking feel
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
}

static string trim(const string &s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace((unsigned char)s[a]))
        ++a;
    if (a == s.size())
        return "";
    size_t b = s.size() - 1;
    while (b > a && std::isspace((unsigned char)s[b]))
        --b;
    return s.substr(a, b - a + 1);
}

static string dirname_of(const string &path)
{
    // return directory part; if no directory, return current dir "."
    if (path.empty())
        return ".";
    try
    {
        fs::path p(path);
        fs::path d = p.parent_path();
        if (d.empty())
            return ".";
        return d.string();
    }
    catch (...)
    {
        // fallback naive parsing
        size_t pos1 = path.find_last_of("/\\");
        if (pos1 == string::npos)
            return ".";
        return path.substr(0, pos1);
    }
}

static string basename_of(const string &path)
{
    if (path.empty())
        return "";
    try
    {
        fs::path p(path);
        return p.filename().string();
    }
    catch (...)
    {
        size_t pos1 = path.find_last_of("/\\");
        if (pos1 == string::npos)
            return path;
        return path.substr(pos1 + 1);
    }
}

static string extension_of(const string &path)
{
    try
    {
        fs::path p(path);
        return p.extension().string();
    }
    catch (...)
    {
        string b = basename_of(path);
        size_t pos = b.find_last_of('.');
        if (pos == string::npos)
            return "";
        return b.substr(pos);
    }
}

static uint64_t filesize_bytes(const string &path)
{
    try
    {
        fs::path p(path);
        if (!fs::exists(p))
            return 0;
        return static_cast<uint64_t>(fs::file_size(p));
    }
    catch (...)
    {
        // fallback: try to open and measure
        ifstream in(path, ios::binary);
        if (!in)
            return 0;
        in.seekg(0, ios::end);
        uint64_t sz = in.tellg();
        in.close();
        return sz;
    }
}

static string make_output_same_dir(const string &inputPath, const string &suffix, const string &forcedExt = "")
{
    // Create an output filename placed in same directory as input
    string dir = dirname_of(inputPath);
    string base = basename_of(inputPath);
    string ext = extension_of(inputPath);
    // if forcedExt is provided, use it (must start with '.' if present)
    string outExt = forcedExt.empty() ? ext : forcedExt;
    if (outExt.empty())
        outExt = ""; // could be no extension

    // strip original extension from base to append suffix
    string nameNoExt = base;
    if (!ext.empty())
    {
        nameNoExt = base.substr(0, base.size() - ext.size());
    }

    string outName = nameNoExt + suffix + outExt;
    fs::path outPath = fs::path(dir) / fs::path(outName);
    return outPath.string();
}

static bool confirm_overwrite_if_exists(const string &path)
{
    if (!fs::exists(path))
        return true;
    cout << "File already exists: " << path << endl;
    cout << "Overwrite? (y/n): ";
    string ans;
    std::getline(cin, ans);
    ans = trim(ans);
    if (ans.empty())
        return false;
    char c = std::tolower(ans[0]);
    return (c == 'y');
}

static void print_progress_bar(uint64_t processed, uint64_t total)
{
    // simple console progress bar
    if (total == 0)
        return;
    const int width = 40;
    double ratio = (double)processed / (double)total;
    int filled = static_cast<int>(ratio * width);
    cout << "\r[";
    for (int i = 0; i < filled; ++i)
        cout << '=';
    for (int i = filled; i < width; ++i)
        cout << ' ';
    cout << "] " << std::setw(3) << static_cast<int>(ratio * 100.0) << "%";
    cout.flush();
    if (processed == total)
        cout << endl;
}

// ============================================================================
// UserManager with custom hash (your provided code integrated)
// ============================================================================

class UserManager
{
private:
    map<string, unsigned long long> users; // username -> password hash

    unsigned long long customHash(const string &password)
    {
        // DJB-like hash (your version). deterministic mapping of password -> integer
        unsigned long long hash = 5381ULL;
        for (unsigned char c : password)
        {
            hash = ((hash << 5) + hash) + static_cast<unsigned long long>(c); // hash * 33 + c
        }
        return hash;
    }

public:
    UserManager()
    {
        // predefined users
        users["admin"] = customHash("admin123");
        users["guest"] = customHash("guest123");
        users["john"] = customHash("doe123");
        users["alice"] = customHash("alice@123");
    }

    bool signup(const string &username, const string &password)
    {
        if (username.empty())
        {
            cout << "Username cannot be empty.\n";
            return false;
        }
        if (users.find(username) != users.end())
        {
            cout << "User already exists!\n";
            return false;
        }
        users[username] = customHash(password);
        cout << "Signup successful. Created user: " << username << "\n";
        return true;
    }

    bool login(const string &username, const string &password)
    {
        auto it = users.find(username);
        if (it != users.end() && it->second == customHash(password))
        {
            cout << "Login successful! Welcome, " << username << ".\n";
            return true;
        }
        cout << "Invalid username or password!\n";
        return false;
    }

    unsigned long long getKey(const string &password)
    {
        return customHash(password);
    }

    // Optional helper to check if user exists (used by external parts if needed)
    bool exists(const string &username)
    {
        return users.find(username) != users.end();
    }
};

// ============================================================================
// BaseCrypto: shared helper for applying key to bytes
// ============================================================================

class BaseCrypto
{
protected:
    // Apply key byte (derived from 64-bit key) to input char
    // i is the byte index so the key cycles across 8 bytes
    static unsigned char keyByteFromKey(unsigned long long key, size_t i)
    {
        size_t shift = (i % 8) * 8;
        unsigned char b = static_cast<unsigned char>((key >> shift) & 0xFFULL);
        return b;
    }

    static unsigned char applyXor(unsigned char dataByte, unsigned long long key, size_t index)
    {
        unsigned char k = keyByteFromKey(key, index);
        return static_cast<unsigned char>(dataByte ^ k);
    }
};

// ============================================================================
// ImageCrypto: separate encrypt() and decrypt() methods
// (For image files we treat them as binary and XOR the entire file)
// ============================================================================

class ImageCrypto : public BaseCrypto
{
public:
    ImageCrypto() = default;

    // Encrypt: read input image and create encrypted output (same dir)
    bool encrypt(const string &inputPath, unsigned long long key)
    {
        string in = trim(inputPath);
        if (!fs::exists(in))
        {
            cout << "Input image does not exist: " << in << "\n";
            return false;
        }

        // Create output path in same directory with suffix "_enc"
        string out = make_output_same_dir(in, "_enc", extension_of(in).empty() ? ".img" : "");
        if (!confirm_overwrite_if_exists(out))
        {
            cout << "Skipping encrypt for: " << in << "\n";
            return false;
        }

        // Open files and XOR
        ifstream fin(in, ios::binary);
        ofstream fout(out, ios::binary);
        if (!fin || !fout)
        {
            cout << "Failed to open files for image encrypt.\n";
            return false;
        }

        uint64_t total = filesize_bytes(in);
        uint64_t processed = 0;
        char buffer;
        size_t idx = 0;
        while (fin.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char transformed = applyXor(byte, key, idx);
            fout.put(static_cast<char>(transformed));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == total)
            { // update occasionally
                print_progress_bar(processed, total);
            }
        }

        fin.close();
        fout.close();
        cout << "\nImage encrypted to: " << out << "\n";
        return true;
    }

    // Decrypt: read an encrypted image and create decrypted output (same dir)
    bool decrypt(const string &inputPath, unsigned long long key)
    {
        string in = trim(inputPath);
        if (!fs::exists(in))
        {
            cout << "Input encrypted image does not exist: " << in << "\n";
            return false;
        }

        // Suggest restored extension .jpg if none present - keep original ext if possible
        string out = make_output_same_dir(in, "_dec", ".jpg");
        if (!confirm_overwrite_if_exists(out))
        {
            cout << "Skipping decrypt for: " << in << "\n";
            return false;
        }

        ifstream fin(in, ios::binary);
        ofstream fout(out, ios::binary);
        if (!fin || !fout)
        {
            cout << "Failed to open files for image decrypt.\n";
            return false;
        }

        uint64_t total = filesize_bytes(in);
        uint64_t processed = 0;
        char buffer;
        size_t idx = 0;
        while (fin.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char transformed = applyXor(byte, key, idx);
            fout.put(static_cast<char>(transformed));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == total)
            {
                print_progress_bar(processed, total);
            }
        }

        fin.close();
        fout.close();
        cout << "\nImage decrypted to: " << out << "\n";
        return true;
    }
};

class FileCrypto : public BaseCrypto
{
public:
    FileCrypto() = default;

    bool encrypt(const string &inputPath, unsigned long long key)
    {
        string in = trim(inputPath);
        if (!fs::exists(in))
        {
            cout << "Input file does not exist: " << in << "\n";
            return false;
        }

        string ext = extension_of(in);
        string out = make_output_same_dir(in, "_enc", ext.empty() ? ".enc" : ".enc");
        if (!confirm_overwrite_if_exists(out))
        {
            cout << "Skipping encrypt for: " << in << "\n";
            return false;
        }

        ifstream fin(in, ios::binary);
        ofstream fout(out, ios::binary);
        if (!fin || !fout)
        {
            cout << "Failed to open files for file encrypt.\n";
            return false;
        }

        uint64_t total = filesize_bytes(in);
        uint64_t processed = 0;
        char buffer;
        size_t idx = 0;
        while (fin.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char transformed = applyXor(byte, key, idx);
            fout.put(static_cast<char>(transformed));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == total)
            {
                print_progress_bar(processed, total);
            }
        }

        fin.close();
        fout.close();
        cout << "\nFile encrypted to: " << out << "\n";
        return true;
    }

    bool decrypt(const string &inputPath, unsigned long long key)
    {
        string in = trim(inputPath);
        if (!fs::exists(in))
        {
            cout << "Encrypted file does not exist: " << in << "\n";
            return false;
        }

        string dir = dirname_of(in);
        string base = basename_of(in);
        string outName;

        // Case 1: filename has "_enc" before extension -> strip it
        size_t posEnc = base.rfind("_enc");
        if (posEnc != string::npos)
        {
            // Example: "Contents_pop_enc.enc"
            // → remove "_enc" before extension
            string withoutEnc = base.substr(0, posEnc);       // "Contents_pop"
            string ext = fs::path(base).extension().string(); // ".enc"

            // If extension is ".enc", try to restore original extension
            if (ext == ".enc")
            {
                // Look for second extension (e.g., ".pdf" inside original)
                string stem = fs::path(withoutEnc).stem().string();         // "Contents_pop"
                string origExt = fs::path(withoutEnc).extension().string(); // ".pdf" if it was kept
                if (!origExt.empty())
                {
                    outName = stem + origExt; // Contents_pop.pdf
                }
                else
                {
                    outName = withoutEnc; // fallback
                }
            }
            else
            {
                outName = withoutEnc + ext; // fallback
            }

            // Case 2: ends with ".enc" only -> strip it
        }
        else if (base.size() > 4 && base.substr(base.size() - 4) == ".enc")
        {
            outName = base.substr(0, base.size() - 4); // file.pdf.enc -> file.pdf

            // Case 3: no marker, just append _dec
        }
        else
        {
            outName = base + "_dec";
        }

        string outPath = (fs::path(dir) / fs::path(outName)).string();
        if (!confirm_overwrite_if_exists(outPath))
        {
            cout << "Skipping decrypt for: " << in << "\n";
            return false;
        }

        ifstream fin(in, ios::binary);
        ofstream fout(outPath, ios::binary);
        if (!fin || !fout)
        {
            cout << "Failed to open files for file decrypt.\n";
            return false;
        }

        uint64_t total = filesize_bytes(in);
        uint64_t processed = 0;
        char buffer;
        size_t idx = 0;

        while (fin.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char transformed = applyXor(byte, key, idx);
            fout.put(static_cast<char>(transformed));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == total)
            {
                print_progress_bar(processed, total);
            }
        }

        fin.close();
        fout.close();
        cout << "\nFile decrypted to: " << outPath << "\n";
        return true;
    }
};

// ---------------- Base64 Helpers ----------------
// Convert vector<unsigned char> → Base64 string
string base64Encode(const vector<unsigned char> &data)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    string encoded;
    int val = 0, valb = -6;
    for (unsigned char c : data)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        encoded.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (encoded.size() % 4)
        encoded.push_back('=');
    return encoded;
}

// Convert Base64 string → vector<unsigned char>
vector<unsigned char> base64Decode(const string &s)
{
    static const int T[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 64, -1, -1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

    vector<unsigned char> out;
    int val = 0, valb = -8;
    for (unsigned char c : s)
    {
        if (T[c] == -1)
            break;
        if (T[c] == 64)
            break; // '=' padding
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return out;
}

// ============================================================================
// TextCrypto: separate encrypt() and decrypt() for strings (console I/O)
// ============================================================================
class TextCrypto : public BaseCrypto
{
public:
    TextCrypto() = default;

    // Encrypt raw text or file
    bool encryptInput(const string &input, unsigned long long key, bool isFile)
    {
        string text;
        string filePath;

        if (isFile)
        {
            filePath = trim(input);
            if (!fs::exists(filePath))
            {
                cout << "Text file does not exist: " << filePath << "\n";
                return false;
            }
            ifstream fin(filePath);
            if (!fin)
            {
                cout << "Failed to open text file.\n";
                return false;
            }
            stringstream buffer;
            buffer << fin.rdbuf();
            text = buffer.str();
            fin.close();
        }
        else
        {
            text = input;
        }

        // Encrypt
        vector<unsigned char> encrypted;
        encrypted.reserve(text.size());
        size_t idx = 0;
        for (unsigned char ch : text)
        {
            encrypted.push_back(applyXor(ch, key, idx));
            ++idx;
        }

        // Convert to Base64 for safe printing
        string encoded = base64Encode(encrypted);

        cout << "Encrypted text (Base64): " << encoded << "\n";

        if (isFile)
        {
            char choice;
            cout << "Do you want to save encrypted text to a file? (y/n): ";
            cin >> choice;
            cin.ignore();
            if (choice == 'y' || choice == 'Y')
            {
                string outPath = filePath + "_enc.txt";
                ofstream fout(outPath);
                fout << encoded;
                fout.close();
                cout << "Encrypted text saved to: " << outPath << "\n";
            }
        }
        return true;
    }

    // Decrypt raw base64 text or from file
    bool decryptInput(const string &input, unsigned long long key, bool isFile)
    {
        string encBase64;
        string filePath;

        if (isFile)
        {
            filePath = trim(input);
            if (!fs::exists(filePath))
            {
                cout << "Encrypted text file does not exist: " << filePath << "\n";
                return false;
            }
            ifstream fin(filePath);
            if (!fin)
            {
                cout << "Failed to open encrypted text file.\n";
                return false;
            }
            stringstream buffer;
            buffer << fin.rdbuf();
            encBase64 = buffer.str();
            fin.close();
        }
        else
        {
            encBase64 = input;
        }

        // Decode Base64 back to bytes
        vector<unsigned char> encrypted = base64Decode(encBase64);

        string decrypted;
        decrypted.reserve(encrypted.size());
        size_t idx = 0;
        for (unsigned char ch : encrypted)
        {
            decrypted.push_back(static_cast<char>(applyXor(ch, key, idx)));
            ++idx;
        }

        cout << "Decrypted text: " << decrypted << "\n";

        if (isFile)
        {
            char choice;
            cout << "Do you want to save decrypted text to a file? (y/n): ";
            cin >> choice;
            cin.ignore();
            if (choice == 'y' || choice == 'Y')
            {
                string outPath = filePath + "_dec.txt";
                ofstream fout(outPath);
                fout << decrypted;
                fout.close();
                cout << "Decrypted text saved to: " << outPath << "\n";
            }
        }
        return true;
    }
};
// ============================================================================
// Stego: store file inside image and retrieve it
// Implementation:
//   - storeFileInImage: copy image bytes then append encrypted file bytes (XORed by key)
//   - retrieveFileFromImage: skip original image bytes (user must provide image size) and decrypt appended bytes
// ============================================================================

class Stego : public BaseCrypto
{
public:
    Stego() = default;

    bool storeFileInImage(const string &imagePath, const string &filePath, unsigned long long key)
    {
        string img = trim(imagePath);
        string file = trim(filePath);

        if (!fs::exists(img))
        {
            cout << "Image does not exist: " << img << "\n";
            return false;
        }
        if (!fs::exists(file))
        {
            cout << "File to hide does not exist: " << file << "\n";
            return false;
        }

        // output image path in same dir as image
        string out = make_output_same_dir(img, "_stego", extension_of(img).empty() ? ".img" : "");
        if (!confirm_overwrite_if_exists(out))
        {
            cout << "Skipping store in image.\n";
            return false;
        }

        ifstream finImg(img, ios::binary);
        ifstream finFile(file, ios::binary);
        ofstream fout(out, ios::binary);
        if (!finImg || !finFile || !fout)
        {
            cout << "Failed to open files for stego store.\n";
            return false;
        }

        // Copy image bytes
        fout << finImg.rdbuf();

        // Append delimiter metadata: we append a short header indicating the original image size and file name length,
        // but because you requested the retrieve to use the original image size as input, we'll keep header minimal.
        // For safety we still append a small signature to indicate start of appended content.
        // signature: 8 bytes "STEGOSTR" then 8 bytes uint64 little-endian for filename length, then filename bytes, then file content encrypted.
        const string signature = "STEGOSTR"; // 8 chars
        fout.write(signature.c_str(), static_cast<std::streamsize>(signature.size()));

        string hiddenFileName = basename_of(file);
        uint64_t nameLen = static_cast<uint64_t>(hiddenFileName.size());
        fout.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen)); // write 8-byte length
        fout.write(hiddenFileName.c_str(), static_cast<std::streamsize>(hiddenFileName.size()));

        // Now append encrypted file content
        uint64_t total = filesize_bytes(file);
        uint64_t processed = 0;
        char buffer;
        size_t idx = 0;
        while (finFile.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char enc = applyXor(byte, key, idx);
            fout.put(static_cast<char>(enc));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == total)
            {
                print_progress_bar(processed, total);
            }
        }

        finImg.close();
        finFile.close();
        fout.close();

        cout << "\nStored file '" << hiddenFileName << "' inside image: " << out << "\n";
        cout << "Original image size (bytes) which you may need for retrieval: " << filesize_bytes(img) << "\n";
        return true;
    }

    bool retrieveFileFromImage(const string &imageWithFile, uint64_t originalImageSize, unsigned long long key)
    {
        string img = trim(imageWithFile);
        if (!fs::exists(img))
        {
            cout << "Image-with-file does not exist: " << img << "\n";
            return false;
        }

        // Open file and seek to originalImageSize
        ifstream fin(img, ios::binary);
        if (!fin)
        {
            cout << "Failed to open image-with-file for reading.\n";
            return false;
        }

        uint64_t fileLen = filesize_bytes(img);
        if (originalImageSize >= fileLen)
        {
            cout << "Given original image size is equal or larger than the file; nothing to retrieve.\n";
            fin.close();
            return false;
        }

        // Seek to the boundary where appended data begins
        fin.seekg(static_cast<std::streamoff>(originalImageSize), ios::beg);

        // Read signature
        char sigBuf[9] = {0};
        fin.read(sigBuf, 8);
        string signature(sigBuf, 8);
        if (signature != "STEGOSTR")
        {
            cout << "Warning: signature not found at expected position. Retrieval may fail.\n";
            // we will attempt to continue, but user must ensure correct image size
        }

        // Read filename length
        uint64_t nameLen = 0;
        fin.read(reinterpret_cast<char *>(&nameLen), sizeof(nameLen));
        if (!fin)
        {
            cout << "Failed to read filename length; retrieval aborted.\n";
            fin.close();
            return false;
        }

        // Read filename
        string hiddenFileName;
        if (nameLen > 0)
        {
            hiddenFileName.resize(static_cast<size_t>(nameLen));
            fin.read(&hiddenFileName[0], static_cast<std::streamsize>(nameLen));
            if (!fin)
            {
                cout << "Failed to read hidden filename; aborting.\n";
                fin.close();
                return false;
            }
        }
        else
        {
            // default name if nameLen invalid
            hiddenFileName = "recovered_file.bin";
        }

        // Prepare output path in same directory as image-with-file
        string dir = dirname_of(img);
        string outPath = (fs::path(dir) / fs::path(string("recovered_") + hiddenFileName)).string();
        if (!confirm_overwrite_if_exists(outPath))
        {
            cout << "Skipping retrieval.\n";
            fin.close();
            return false;
        }

        ofstream fout(outPath, ios::binary);
        if (!fout)
        {
            cout << "Failed to open output file for writing retrieved content.\n";
            fin.close();
            return false;
        }

        // Now read remainder of file and decrypt
        char buffer;
        size_t idx = 0;
        uint64_t processed = 0;
        uint64_t remaining = fileLen > static_cast<uint64_t>(fin.tellg()) ? (fileLen - static_cast<uint64_t>(fin.tellg())) : 0;

        while (fin.get(buffer))
        {
            unsigned char byte = static_cast<unsigned char>(buffer);
            unsigned char dec = applyXor(byte, key, idx);
            fout.put(static_cast<char>(dec));
            ++idx;
            ++processed;
            if ((processed & 0x1FFF) == 0 || processed == remaining)
            {
                print_progress_bar(processed, remaining);
            }
        }

        fin.close();
        fout.close();

        cout << "\nRetrieved hidden file to: " << outPath << "\n";
        return true;
    }
};

static void printMainMenuOptions()
{
    cout << "\n====== MAIN MENU ======\n";
    cout << "1. Encrypt Image\n";
    cout << "2. Decrypt Image\n";
    cout << "3. Encrypt File\n";
    cout << "4. Decrypt File\n";
    cout << "5. Encrypt Text\n";
    cout << "6. Decrypt Text\n";
    cout << "7. Store File in Image (Stego)\n";
    cout << "8. Retrieve File from Image (Stego)\n";
    cout << "9. Logout\n";
    cout << "Enter choice: ";
}

static void mainMenuFlow(UserManager &userManager)
{
    ImageCrypto imageCrypto;
    FileCrypto fileCrypto;
    TextCrypto textCrypto;
    Stego stego;

    cout << "Enter the password for making the encryption key: ";
    string password;
    std::getline(cin, password);
    unsigned long long key = userManager.getKey(password);

    bool keepRunning = true;
    while (keepRunning)
    {
        printMainMenuOptions();
        string choiceLine;
        std::getline(cin, choiceLine);
        int choice = 0;
        try
        {
            choice = std::stoi(trim(choiceLine));
        }
        catch (...)
        {
            choice = 0;
        }

        switch (choice)
        {
        case 1:
        { // Encrypt Image
            cout << "Enter image path: ";
            string in;
            std::getline(cin, in);
            in = trim(in);
            if (in.empty())
            {
                cout << "No image path provided.\n";
                break;
            }
            imageCrypto.encrypt(in, key);
            break;
        }
        case 2:
        { // Decrypt Image
            cout << "Enter encrypted image path: ";
            string in;
            std::getline(cin, in);
            in = trim(in);
            if (in.empty())
            {
                cout << "No path provided.\n";
                break;
            }
            imageCrypto.decrypt(in, key);
            break;
        }
        case 3:
        { // Encrypt File
            cout << "Enter file path to encrypt: ";
            string in;
            std::getline(cin, in);
            in = trim(in);
            if (in.empty())
            {
                cout << "No file path provided.\n";
                break;
            }
            fileCrypto.encrypt(in, key);
            break;
        }
        case 4:
        { // Decrypt File
            cout << "Enter encrypted file path to decrypt: ";
            string in;
            std::getline(cin, in);
            in = trim(in);
            if (in.empty())
            {
                cout << "No file path provided.\n";
                break;
            }
            fileCrypto.decrypt(in, key);
            break;
        }
        case 5:
        {
            cout << "Enter text to encrypt (single line): ";
            string txt;
            std::getline(cin, txt);
            txt = trim(txt);
            if (txt.empty())
            {
                cout << "Empty text. Nothing to encrypt.\n";
                break;
            }

            // Use TextCrypto's encryptInput
            if (!textCrypto.encryptInput(txt, key, false))
            {
                cout << "Encryption failed.\n";
            }
            break;
        }

        case 6:
        {
            cout << "Enter encrypted text to decrypt (Base64 string): ";
            string enc;
            std::getline(cin, enc);
            enc = trim(enc);
            if (enc.empty())
            {
                cout << "Empty. Nothing to decrypt.\n";
                break;
            }

            // Use TextCrypto's decryptInput
            if (!textCrypto.decryptInput(enc, key, false))
            {
                cout << "Decryption failed.\n";
            }
            break;
        }

        case 7:
        { // Store File in Image
            cout << "Enter image path (cover image): ";
            string img;
            std::getline(cin, img);
            img = trim(img);
            cout << "Enter file path to hide: ";
            string file;
            std::getline(cin, file);
            file = trim(file);
            if (img.empty() || file.empty())
            {
                cout << "Missing image or file path.\n";
                break;
            }
            stego.storeFileInImage(img, file, key);
            break;
        }
        case 8:
        { // Retrieve File from Image
            cout << "Enter image-with-file path: ";
            string img;
            std::getline(cin, img);
            img = trim(img);
            if (img.empty())
            {
                cout << "Missing path.\n";
                break;
            }
            cout << "Enter original image size (in bytes) used when storing (you can use file properties): ";
            string sizeStr;
            std::getline(cin, sizeStr);
            uint64_t origSize = 0;
            try
            {
                origSize = std::stoull(trim(sizeStr));
            }
            catch (...)
            {
                cout << "Invalid number. Aborting retrieve.\n";
                break;
            }
            stego.retrieveFileFromImage(img, origSize, key);
            break;
        }
        case 9:
        {
            cout << "Logging out...\n";
            keepRunning = false;
            break;
        }
        default:
            cout << "Invalid choice. Enter number 1-9.\n";
        }
        waitShort();
    }
}

int main()
{
    UserManager userManager;
    cout << "====== USER MENU ======\n";
    bool programRunning = true;

    while (programRunning)
    {
        cout << "\n1. Login\n2. Signup\n3. Exit\n";
        cout << "Enter choice: ";
        string choiceLine;
        std::getline(cin, choiceLine);
        int choice = 0;
        try
        {
            choice = std::stoi(trim(choiceLine));
        }
        catch (...)
        {
            choice = 0;
        }

        switch (choice)
        {
        case 1:
        {
            cout << "Enter username: ";
            string username;
            std::getline(cin, username);
            username = trim(username);
            cout << "Enter password: ";
            string password;
            std::getline(cin, password);
            password = trim(password);
            if (userManager.login(username, password))
            {
                // user logged in -> go to main menu flow
                mainMenuFlow(userManager);
            }
            break;
        }
        case 2:
        {
            cout << "Enter new username: ";
            string username;
            std::getline(cin, username);
            username = trim(username);
            cout << "Enter new password: ";
            string password;
            std::getline(cin, password);
            password = trim(password);
            userManager.signup(username, password);
            break;
        }
        case 3:
        {
            cout << "Exiting program. Goodbye.\n";
            programRunning = false;
            break;
        }
        default:
            cout << "Invalid choice. Enter 1, 2, or 3.\n";
        }

        waitShort();
    }

    return 0;
}
