#include "x11.h"
#include "ipc_client.h"

#include <utility>
#include <cstdlib>
#include <spdlog/spdlog.h>

X11::X11(std::shared_ptr<IPCClient> ipc_) : ipc(std::move(ipc_))
{
}

X11::~X11()
{
    if (connection)
        xcb_disconnect(connection);
}

void X11::set_window(xcb_window_t next_window, const char* next_display)
{
    if (!next_window)
        return;
    if (!next_display)
        next_display = std::getenv("DISPLAY");
    if (!next_display)
        return;

    std::lock_guard lock(m);
    if (window == next_window && display_name == next_display)
        return;

    ipc->x11_focused.store(false);
    if (connection)
        xcb_disconnect(connection);
    connection = xcb_connect(next_display, nullptr);
    display_name = next_display;
    window = next_window;
    if (!connection || xcb_connection_has_error(connection)) {
        SPDLOG_ERROR("x11: failed to connect display={}", display_name);
        return;
    }

    const uint32_t mask = XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW |
                          XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_KEY_PRESS |
                          XCB_EVENT_MASK_KEY_RELEASE;
    auto cookie = xcb_change_window_attributes_checked(connection, window, XCB_CW_EVENT_MASK, &mask);
    auto* error = xcb_request_check(connection, cookie);
    if (error) {
        SPDLOG_ERROR("x11: failed to watch window={} error={}", window, error->error_code);
        std::free(error);
        return;
    }
    xcb_flush(connection);
    keys.clear();
    load_keymap();
    auto* focus = xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), nullptr);
    if (focus) {
        ipc->x11_focused.store(focus->focus == window);
        std::free(focus);
    }
    SPDLOG_DEBUG("x11: watching display={} window={}", display_name, window);
}

void X11::dispatch_events()
{
    std::lock_guard lock(m);
    if (!connection || xcb_connection_has_error(connection)) {
        ipc->x11_focused.store(false);
        return;
    }

    xcb_generic_event_t* next = nullptr;
    while (auto* event = next ? next : xcb_poll_for_event(connection)) {
        next = nullptr;
        switch (event->response_type & 0x7f) {
            case XCB_FOCUS_IN:
            case XCB_FOCUS_OUT: {
                auto* focus = reinterpret_cast<xcb_focus_in_event_t*>(event);
                keys.clear();
                // Grabs are temporary routing changes, not application focus changes.
                if (focus->detail != XCB_NOTIFY_DETAIL_INFERIOR &&
                    (focus->mode == XCB_NOTIFY_MODE_NORMAL || focus->mode == XCB_NOTIFY_MODE_WHILE_GRABBED))
                    ipc->x11_focused.store((event->response_type & 0x7f) == XCB_FOCUS_IN);
                SPDLOG_DEBUG("x11: keyboard entered={} window={} mode={} detail={}",
                             (event->response_type & 0x7f) == XCB_FOCUS_IN,
                             focus->event, focus->mode, focus->detail);
                break;
            }
            case XCB_ENTER_NOTIFY:
            case XCB_LEAVE_NOTIFY: {
                auto* crossing = reinterpret_cast<xcb_enter_notify_event_t*>(event);
                SPDLOG_DEBUG("x11: pointer entered={} window={} mode={} detail={}",
                             (event->response_type & 0x7f) == XCB_ENTER_NOTIFY,
                             crossing->event, crossing->mode, crossing->detail);
                break;
            }
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE: {
                auto* key = reinterpret_cast<xcb_key_press_event_t*>(event);
                bool pressed = (event->response_type & 0x7f) == XCB_KEY_PRESS;
                if (!pressed) {
                    // Auto-repeat sends a release immediately followed by a press with the same timestamp.
                    next = xcb_poll_for_queued_event(connection);
                    auto* repeat = reinterpret_cast<xcb_key_press_event_t*>(next);
                    if (next && (next->response_type & 0x7f) == XCB_KEY_PRESS &&
                        repeat->detail == key->detail && repeat->time == key->time) {
                        std::free(next);
                        next = nullptr;
                        break;
                    }
                }
                keys.update(*ipc, keysym(key->detail), pressed);
                break;
            }
            case XCB_MAPPING_NOTIFY:
                load_keymap();
                break;
        }
        std::free(event);
    }
}

void X11::load_keymap()
{
    keysyms.clear();
    const auto* setup = xcb_get_setup(connection);
    min_keycode = setup->min_keycode;
    uint8_t count = setup->max_keycode - setup->min_keycode + 1;
    auto* reply = xcb_get_keyboard_mapping_reply(
        connection, xcb_get_keyboard_mapping(connection, min_keycode, count), nullptr);
    if (!reply) {
        SPDLOG_ERROR("x11: failed to get keyboard mapping");
        return;
    }

    // The first keysym of each keycode is its unmodified symbol.
    auto* syms = xcb_get_keyboard_mapping_keysyms(reply);
    int per_keycode = reply->keysyms_per_keycode;
    for (int i = 0; i < count; i++)
        keysyms.push_back(per_keycode > 0 ? syms[i * per_keycode] : XKB_KEY_NoSymbol);
    std::free(reply);
}

xkb_keysym_t X11::keysym(xcb_keycode_t keycode) const
{
    if (keycode < min_keycode || keycode - min_keycode >= static_cast<int>(keysyms.size()))
        return XKB_KEY_NoSymbol;
    return keysyms[keycode - min_keycode];
}
