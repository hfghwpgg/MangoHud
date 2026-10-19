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
        refresh_bind(ipc);
        if (matches(sym))
            ipc.toggle_hud_visible();
    }

    void clear() { keys.clear(); }

private:
    std::vector<xkb_keysym_t> keys;
    std::vector<xkb_keysym_t> bind;
    uint64_t bind_generation = 0;

    void refresh_bind(IPCClient& ipc) {
        if (ipc.toggle_hud_generation.load(std::memory_order_acquire) == bind_generation)
            return;

        std::string str;
        {
            std::lock_guard lock(ipc.m);
            str = ipc.toggle_hud;
            bind_generation = ipc.toggle_hud_generation.load(std::memory_order_acquire);
        }

        bind.clear();
        for (const auto& name : str_tokenize(str, "+")) {
            auto key = xkb_keysym_from_name(name.c_str(), XKB_KEYSYM_NO_FLAGS);
            if (key == XKB_KEY_NoSymbol)
                key = xkb_keysym_from_name(name.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);
            if (key == XKB_KEY_NoSymbol) {
                SPDLOG_WARN("toggle_hud: unknown key '{}'", name);
                bind.clear();
                return;
            }
            bind.push_back(xkb_keysym_to_lower(key));
        }
    }

    bool matches(xkb_keysym_t sym) const {
        if (bind.empty())
            return false;

        bool has_sym = false;
        for (auto key : bind) {
            if (std::find(keys.begin(), keys.end(), key) == keys.end())
                return false;
            has_sym |= key == sym;
        }

        return has_sym;
    }
};
