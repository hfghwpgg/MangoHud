#pragma once

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#include <xkbcommon/xkbcommon.h>
#include <spdlog/spdlog.h>

#include "../ipc/ipc_client.h"
#include "string_utils.h"

// Keysyms are compared lowercased, so callers can pass either level of a letter key.
class KeyState {
public:
    void update(IPCClient& ipc, xkb_keysym_t sym, bool pressed) {
        sym = xkb_keysym_to_lower(sym);
        std::erase(keys, sym);
        if (!pressed)
            return;

        keys.push_back(sym);

        std::string bind;
        {
            std::lock_guard lock(ipc.m);
            bind = ipc.toggle_hud;
        }
        if (matches(bind, sym))
            ipc.send_toggle_hud();
    }

    void clear() { keys.clear(); }

private:
    std::vector<xkb_keysym_t> keys;

    bool matches(const std::string& bind, xkb_keysym_t sym) const {
        auto names = str_tokenize(bind, "+");
        if (names.empty())
            return false;

        bool has_sym = false;
        for (const auto& name : names) {
            auto key = xkb_keysym_from_name(name.c_str(), XKB_KEYSYM_NO_FLAGS);
            if (key == XKB_KEY_NoSymbol)
                key = xkb_keysym_from_name(name.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);
            if (key == XKB_KEY_NoSymbol) {
                SPDLOG_DEBUG("toggle_hud: unknown key '{}'", name);
                return false;
            }

            key = xkb_keysym_to_lower(key);
            if (std::find(keys.begin(), keys.end(), key) == keys.end())
                return false;
            has_sym |= key == sym;
        }

        return has_sym;
    }
};
