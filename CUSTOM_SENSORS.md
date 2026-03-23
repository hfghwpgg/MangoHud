# MangoHud Custom Sensor Label/Grouping System

This document describes how to use the custom sensor labels feature added to MangoHud.
This feature allows you to display individual GPU/CPU sensors on separate lines,
grouped under custom labels with configurable colors.

> **Requires `legacy_layout=0`** — custom sensors only work with the non-legacy layout.

## Quick Example

```ini
# MangoHud config file (~/.config/MangoHud/MangoHud.conf)
legacy_layout=0
table_columns=3

# Custom label color (hex RGB, default: 2e9762 = GPU green)
cust_label_color1=2e9762

# Line 1: "GPU TEMP" label with edge + junction temps
cust_label1=GPU TEMP
cust_gpu_temp
cust_gpu_junction_temp

# Line 2: "GPU CLK" label with core clock + voltage
cust_label1=GPU CLK
cust_gpu_core_clock
cust_gpu_voltage

# Line 3: "GPU PWR" label with power + fan
cust_label2=GPU PWR
cust_gpu_power
cust_gpu_fan

# You can still use standard gpu_stats/cpu_stats alongside:
gpu_stats
cpu_stats
fps
frame_timing
```

## How It Works

1. `cust_label(1-9)=<text>` starts a **new HUD row** with the given label text
2. Any `cust_*` sensor params that follow fill columns in that same row
3. When columns are full, sensors wrap to the next row automatically
4. The next `cust_label(1-9)` starts a completely new row

## Available Custom Sensor Names

### GPU Sensors

| Config Key | Description | Unit Style |
|---|---|---|
| `cust_gpu_load` | GPU utilization % | Normal text |
| `cust_gpu_temp` | GPU edge temperature | Normal °C/°F |
| `cust_gpu_junction_temp` | GPU junction temperature | Normal °C/°F + small "Jnc" |
| `cust_gpu_mem_temp` | GPU memory temperature | Normal °C/°F |
| `cust_gpu_core_clock` | GPU core clock frequency | Small "MHz" |
| `cust_gpu_mem_clock` | GPU memory clock frequency | Small "MHz" |
| `cust_gpu_power` | GPU power draw | Small "W" |
| `cust_gpu_voltage` | GPU core voltage | Small "mV" |
| `cust_gpu_fan` | GPU fan speed | Small "RPM" or "% FAN" |

### CPU Sensors

| Config Key | Description | Unit Style |
|---|---|---|
| `cust_cpu_load` | CPU utilization % | Normal text |
| `cust_cpu_temp` | CPU temperature | Normal °C/°F |
| `cust_cpu_mhz` | CPU frequency | Small "MHz" |
| `cust_cpu_power` | CPU power draw | Small "W" |

## Configuration

### Label Color

```ini
# Set color for all custom labels (hex RGB)
cust_label_color1=2e9762
```

Default is `2e9762` (GPU green). This follows the same format as other MangoHud
color options (`cpu_color`, `gpu_color`, etc.).

### Unit Rendering

- **Temperature** units (°C / °F) render in **normal text size**
- **MHz, mV, W** units render in **small text** at the bottom (subscript style)
- **Jnc** (junction) annotation renders in **small text**
- **RPM / FAN** renders in **small text**

This matches the standard `gpu_stats` / `cpu_stats` rendering behavior.

## Backwards Compatibility

- All existing config options (`gpu_stats`, `cpu_stats`, etc.) work unchanged
- The `cust_` prefix avoids any name collision with existing parameters
- If no `cust_*` options are used, MangoHud behavior is identical to before
- `gpu_load_change`, `cpu_load_change`, and `temp_fahrenheit` settings
  apply to custom sensors too
