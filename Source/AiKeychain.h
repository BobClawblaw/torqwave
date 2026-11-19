#pragma once

#include <optional>
#include <string>

/** Where an AI provider's key is kept: the system's own store for secrets, never a settings file.

    - Mac: the login keychain (a generic password, service "Torqwave AI").
    - Windows: Credential Manager (a generic credential, "Torqwave AI: <account>").
    - Linux: no store yet. The key comes from the environment (TORQWAVE_AI_KEY, then OPENAI_API_KEY),
      and store() refuses.

    The account names the server the key is for (its base URL), so each server keeps its own key.
    Plain C++ here, apart from the platform's own headers, so they never meet JUCE's. */
namespace AiKeychain
{
    /** True where store() can keep a key (Mac and Windows). */
    bool canStore();

    /** The key for this account, or nothing. On Linux, the environment's key for any account. */
    std::optional<std::string> load (const std::string& account);

    /** Keeps the key for this account, replacing any. False when it couldn't. */
    bool store (const std::string& account, const std::string& secret);

    /** Removes the key for this account. True when there is none afterwards. */
    bool erase (const std::string& account);

    /** Where keys are kept, for the settings page ("the login keychain"). */
    const char* whereKept();
}
