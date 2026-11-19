#include "AiKeychain.h"

#include <cstdlib>

#if defined (__APPLE__)
 #include <Security/Security.h>
#elif defined (_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <wincred.h>
 #include <vector>
 #pragma comment (lib, "Advapi32.lib")
#endif

namespace AiKeychain
{
#if defined (__APPLE__)

namespace
{
    constexpr const char* service = "Torqwave AI";

    /** A CFString of UTF-8 text, released when it goes. */
    struct CfString
    {
        explicit CfString (const std::string& s)
            : ref (CFStringCreateWithBytes (kCFAllocatorDefault, (const UInt8*) s.data(), (CFIndex) s.size(), kCFStringEncodingUTF8, false)) {}
        ~CfString()   { if (ref != nullptr) CFRelease (ref); }
        CfString (const CfString&) = delete;
        CfString& operator= (const CfString&) = delete;
        CFStringRef ref;
    };

    /** The query that finds this account's item. The caller releases it. */
    CFMutableDictionaryRef queryFor (const CfString& serviceName, const CfString& accountName)
    {
        auto q = CFDictionaryCreateMutable (kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue (q, kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue (q, kSecAttrService, serviceName.ref);
        CFDictionarySetValue (q, kSecAttrAccount, accountName.ref);
        return q;
    }
}

bool canStore()             { return true; }
const char* whereKept()     { return "the login keychain"; }

std::optional<std::string> load (const std::string& account)
{
    const CfString s (service), a (account);
    auto q = queryFor (s, a);
    CFDictionarySetValue (q, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue (q, kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef found = nullptr;
    const auto status = SecItemCopyMatching (q, &found);
    CFRelease (q);

    if (status != errSecSuccess || found == nullptr)
        return std::nullopt;

    const auto data = (CFDataRef) found;
    std::string secret ((const char*) CFDataGetBytePtr (data), (size_t) CFDataGetLength (data));
    CFRelease (found);
    return secret;
}

bool store (const std::string& account, const std::string& secret)
{
    const CfString s (service), a (account);
    auto data = CFDataCreate (kCFAllocatorDefault, (const UInt8*) secret.data(), (CFIndex) secret.size());
    auto q = queryFor (s, a);

    auto changes = CFDictionaryCreateMutable (kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue (changes, kSecValueData, data);
    auto status = SecItemUpdate (q, changes);
    CFRelease (changes);

    if (status == errSecItemNotFound)
    {
        CFDictionarySetValue (q, kSecValueData, data);
        CFDictionarySetValue (q, kSecAttrLabel, s.ref);
        status = SecItemAdd (q, nullptr);
    }

    CFRelease (q);
    CFRelease (data);
    return status == errSecSuccess;
}

bool erase (const std::string& account)
{
    const CfString s (service), a (account);
    auto q = queryFor (s, a);
    const auto status = SecItemDelete (q);
    CFRelease (q);
    return status == errSecSuccess || status == errSecItemNotFound;
}

#elif defined (_WIN32)

namespace
{
    std::wstring wide (const std::string& s)
    {
        if (s.empty())
            return {};

        const auto n = MultiByteToWideChar (CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0);
        std::wstring w ((size_t) n, L'\0');
        MultiByteToWideChar (CP_UTF8, 0, s.data(), (int) s.size(), w.data(), n);
        return w;
    }

    std::wstring targetFor (const std::string& account)   { return L"Torqwave AI: " + wide (account); }
}

bool canStore()             { return true; }
const char* whereKept()     { return "Windows Credential Manager"; }

std::optional<std::string> load (const std::string& account)
{
    const auto target = targetFor (account);
    PCREDENTIALW found = nullptr;

    if (! CredReadW (target.c_str(), CRED_TYPE_GENERIC, 0, &found) || found == nullptr)
        return std::nullopt;

    std::string secret ((const char*) found->CredentialBlob, (size_t) found->CredentialBlobSize);
    CredFree (found);
    return secret;
}

bool store (const std::string& account, const std::string& secret)
{
    auto target = targetFor (account);
    auto user = wide (account);
    std::vector<BYTE> blob (secret.begin(), secret.end());

    CREDENTIALW c {};
    c.Type = CRED_TYPE_GENERIC;
    c.TargetName = target.data();
    c.UserName = user.data();
    c.CredentialBlobSize = (DWORD) blob.size();
    c.CredentialBlob = blob.data();
    c.Persist = CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW (&c, 0) != FALSE;
}

bool erase (const std::string& account)
{
    const auto target = targetFor (account);
    return CredDeleteW (target.c_str(), CRED_TYPE_GENERIC, 0) != FALSE || GetLastError() == ERROR_NOT_FOUND;
}

#else

bool canStore()             { return false; }
const char* whereKept()     { return "the TORQWAVE_AI_KEY or OPENAI_API_KEY environment variable"; }

std::optional<std::string> load (const std::string&)
{
    for (auto* name : { "TORQWAVE_AI_KEY", "OPENAI_API_KEY" })
        if (const auto* value = std::getenv (name); value != nullptr && *value != '\0')
            return std::string (value);

    return std::nullopt;
}

bool store (const std::string&, const std::string&)   { return false; }
bool erase (const std::string&)                        { return true; }

#endif
}
