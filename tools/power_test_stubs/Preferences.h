#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace PowerPreferencesTest {
inline std::map<std::string, std::vector<std::uint8_t>>& storage() {
    static std::map<std::string, std::vector<std::uint8_t>> values;
    return values;
}

inline std::size_t& writeCount() {
    static std::size_t count = 0U;
    return count;
}
}

inline void powerPreferencesTestReset() {
    PowerPreferencesTest::storage().clear();
    PowerPreferencesTest::writeCount() = 0U;
}

inline std::size_t powerPreferencesTestWriteCount() {
    return PowerPreferencesTest::writeCount();
}

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false) {
        namespace_ = name != nullptr ? name : "";
        readOnly_ = readOnly;
        opened_ = !namespace_.empty();
        return opened_;
    }

    void end() {
        opened_ = false;
    }

    std::size_t getBytesLength(const char* key) const {
        const auto iterator = PowerPreferencesTest::storage().find(fullKey(key));
        return iterator == PowerPreferencesTest::storage().end()
            ? 0U
            : iterator->second.size();
    }

    std::size_t getBytes(const char* key, void* buffer, std::size_t maxLength) const {
        if (!opened_ || buffer == nullptr) {
            return 0U;
        }
        const auto iterator = PowerPreferencesTest::storage().find(fullKey(key));
        if (iterator == PowerPreferencesTest::storage().end()) {
            return 0U;
        }
        const std::size_t length = std::min(maxLength, iterator->second.size());
        std::memcpy(buffer, iterator->second.data(), length);
        return length;
    }

    std::size_t putBytes(const char* key, const void* value, std::size_t length) {
        if (!opened_ || readOnly_ || value == nullptr) {
            return 0U;
        }
        const auto* bytes = static_cast<const std::uint8_t*>(value);
        PowerPreferencesTest::storage()[fullKey(key)] =
            std::vector<std::uint8_t>(bytes, bytes + length);
        ++PowerPreferencesTest::writeCount();
        return length;
    }

private:
    std::string fullKey(const char* key) const {
        return namespace_ + "/" + (key != nullptr ? key : "");
    }

    std::string namespace_;
    bool readOnly_ = false;
    bool opened_ = false;
};
