# Custom Sensor Label/Grouping System for MangoHud

Add a new custom label + sensor grouping system that lets users display individual GPU/CPU sensors on separate lines, grouped under custom labels. Fully backwards-compatible — only adds new functionality, doesn't change existing [gpu_stats](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#284-420)/[cpu_stats](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#421-512) behavior.

## Proposed Changes

### Config System

#### [MODIFY] [overlay_params.h](file:///home/adam/Desktop/MangoHud/src/overlay_params.h)

Add new `OVERLAY_PARAM_CUSTOM` entries to the `OVERLAY_PARAMS` macro:

- `cust_label` — user-defined label text that starts a new HUD line group (e.g., `cust_label=GPU TEMP`)
- `cust_label_color` — hex color for custom labels (e.g., `cust_label_color=2e9762`)

Add to [overlay_params](file:///home/adam/Desktop/MangoHud/src/overlay_params.h#289-397) struct:
- `unsigned cust_label_color` — color value for custom labels
- `ImVec4 cust_label_color_converted` field not needed — will use [hud_colors](file:///home/adam/Desktop/MangoHud/src/hud_elements.h#124-152) struct instead

Add to [hud_colors](file:///home/adam/Desktop/MangoHud/src/hud_elements.h#124-152) struct in [hud_elements.h](file:///home/adam/Desktop/MangoHud/src/hud_elements.h):
- `ImVec4 cust_label` — converted color for custom labels

#### [MODIFY] [overlay_params.cpp](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp)

- Add `#define parse_cust_label(s) parse_str(s)` and `#define parse_cust_label_color(s) parse_color(s)` for config parsing
- Add `cust_label_color` default to [set_param_defaults()](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#844-935) (default: `0x2e9762` — same as GPU green)
- Add `cust_label_color` to the color conversion array in [parse_overlay_config()](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#959-1237)
- Handle `cust_label` params in [parse_overlay_env()](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#795-843) — they must NOT be filtered as "unknown option"

---

### New Custom Sensor Params (available as [sort_elements](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#1945-2033) keys)

These are **not** `OVERLAY_PARAM_BOOL` entries (to avoid polluting the enabled array). Instead, they are recognized **only** in [sort_elements()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#1945-2033) as display function mappings. This means they only work in non-legacy layout (`legacy_layout=0`), which is the expected usage.

New sensor rendering functions that read the same underlying GPU data as [gpu_stats()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#284-420) and [cpu_stats()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#421-512) but render each value individually:

| Config key | Renders | Unit style |
|---|---|---|
| `cust_gpu_load` | GPU utilization % | Normal text |
| `cust_gpu_temp` | GPU edge temp | Normal text (°C/°F) |
| `cust_gpu_junction_temp` | GPU junction temp | °C/°F + small "Jnc" |
| `cust_gpu_mem_temp` | GPU memory temp | Normal text (°C/°F) |
| `cust_gpu_core_clock` | GPU core clock | Small "MHz" |
| `cust_gpu_mem_clock` | GPU memory clock | Small "MHz" |
| `cust_gpu_power` | GPU power draw | Small "W" |
| `cust_gpu_voltage` | GPU voltage | Small "mV" |
| `cust_gpu_fan` | GPU fan speed | Small "RPM" or "% FAN" |
| `cust_cpu_load` | CPU utilization % | Normal text |
| `cust_cpu_temp` | CPU temperature | Normal text (°C/°F) |
| `cust_cpu_mhz` | CPU frequency | Small "MHz" |
| `cust_cpu_power` | CPU power | Small "W" |

> [!IMPORTANT]
> All new keys use `cust_` prefix to avoid name collision with existing params.

---

### Rendering

#### [MODIFY] [hud_elements.h](file:///home/adam/Desktop/MangoHud/src/hud_elements.h)

Add new static member functions:
- `static void cust_label()` — renders the label text + any sensors that follow before the next label
- One function per custom sensor listed above (e.g., `static void cust_gpu_load()`, etc.)

#### [MODIFY] [hud_elements.cpp](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp)

**`cust_label()` implementation:**
- Calls [ImguiNextColumnFirstItem()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#226-231) to start a new row
- Renders the label text from the function's stored [value](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#326-338) field using `cust_label` color
- Does NOT create a `TableNextRow()` — the overlay loop will handle that

**Individual sensor functions** (e.g., `cust_gpu_temp()`):
- Call [ImguiNextColumnOrNewRow()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#231-248) (fills columns left-to-right, wrapping to new row if needed)
- Render just the single sensor value with proper units:
  - Temperature: normal-size text for value and °C/°F
  - MHz, mV, W, Jnc: value in normal text, unit in small font (`PushFont(font_small)`)
- Uses the same data sources as existing [gpu_stats()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#284-420) (reads from `gpus->selected_gpus()`)

**Key behavior:** The overlay loop in [overlay.cpp](file:///home/adam/Desktop/MangoHud/src/overlay.cpp) calls `TableNextRow()` before each ordered function. `cust_label` creates a new first-column cell. Subsequent `cust_` sensor functions call [ImguiNextColumnOrNewRow()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#231-248) which fills the same row or wraps naturally. When the next `cust_label` appears, `TableNextRow()` creates a new line.

#### [MODIFY] [hud_elements.cpp sort_elements()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#L1945-L2032)

Add all `cust_*` keys to the `display_params` map, mapping to their respective rendering functions.

#### Color Support

#### [MODIFY] [hud_elements.cpp convert_colors()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#L144-L201)

Add `HUDElements.colors.cust_label = convert(params.cust_label_color);`

---

### Config Parsing for Unknown Keys

#### [MODIFY] [overlay_params.cpp parse_overlay_env()](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#L796-L842)

The current parsing loop rejects unknown keys with `SPDLOG_ERROR("Unknown option '{}'", key)`. The `cust_` prefixed params are **not** in the `OVERLAY_PARAMS` macro (intentionally, to avoid the enabled-array). So we need to:

1. Check if [key](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#172-187) starts with `cust_` prefix before the "Unknown option" error
2. If it does, call [add_to_options(params, key, value)](file:///home/adam/Desktop/MangoHud/src/overlay_params.cpp#1290-1294) normally
3. This ensures they appear in `HUDElements.options` and get picked up by [sort_elements()](file:///home/adam/Desktop/MangoHud/src/hud_elements.cpp#1945-2033)

---

### Documentation

#### [NEW] [CUSTOM_SENSORS.md](file:///home/adam/Desktop/MangoHud/CUSTOM_SENSORS.md)

Usage guide documenting:
- All available `cust_` sensor names and what they display
- `cust_label` usage with examples
- `cust_label_color` configuration
- Example config file showing typical usage
- Requirements (must use `legacy_layout=0`)

## Verification Plan

### Build Verification

Since tests are commented out in [meson.build](file:///home/adam/Desktop/MangoHud/meson.build) and no unit tests exist for the overlay rendering:

```bash
cd /home/adam/Desktop/MangoHud
# Reconfigure and rebuild
meson setup build --reconfigure || meson setup build
ninja -C build
```

A successful `ninja -C build` with no compilation errors confirms the changes integrate properly with the existing codebase.

### Manual Verification

Since MangoHud requires an active game or `mangoapp` to render its overlay, manual visual testing would require:

1. Creating a test config file with custom labels and sensors
2. Running a Vulkan/OpenGL application with MangoHud enabled
3. Visually confirming the layout

> [!NOTE]
> I'll leave manual visual testing to you since it requires running a graphical application. I'll provide a sample config file you can test with.
