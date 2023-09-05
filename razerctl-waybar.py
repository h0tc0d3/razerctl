import sys
import json
import subprocess

device_id = 1  # Device ID
razerctl = 'razerctl'  # Path to razerctl binary

# FontAwesome glyphs (Private Use Area) used in the waybar output.
ICON_MOUSE = '\uf8cc'
ICON_ACTIVE_STAGE = '\uf058'
ICON_BATTERY_CHARGING = '\uf5e7'
ICON_BATTERY_LEVELS = ('\uf240', '\uf241', '\uf242', '\uf243', '\uf244')

# Safe one-line JSON for waybar when razerctl fails, times out or emits
# non-JSON (razerctl prints error text on stdout on failure).
ERROR_JSON = '{"text": "\u26a0", "tooltip": "razerctl error"}'


def battery_icon(razer):
    """Return the FontAwesome icon matching the reported battery state."""
    if razer.get('charging'):
        return ICON_BATTERY_CHARGING
    try:
        value = int(str(razer.get('battery', '')).rstrip('%'))
    except ValueError:
        value = 0
    if value >= 85:
        return ICON_BATTERY_LEVELS[0]
    if value >= 65:
        return ICON_BATTERY_LEVELS[1]
    if value >= 40:
        return ICON_BATTERY_LEVELS[2]
    if value >= 10:
        return ICON_BATTERY_LEVELS[3]
    return ICON_BATTERY_LEVELS[4]


def compact_dpi(value):
    """Collapse "NNNxNNN" into "NNN" when both axes are equal."""
    parts = str(value).split('x')
    return parts[0] if len(parts) == 2 and parts[0] == parts[1] else str(value)


def format_device(razer):
    """Build one line of waybar JSON from a razerctl --json device object."""
    dpi = compact_dpi(razer.get('dpi', '?'))

    lod = ''
    if 'lod' in razer:
        if razer.get('lod_async'):
            lod = 'Lift-Off Distance: {0}\\nLand Distance: {1}\\n'.format(razer['lod'], razer.get('ld', '?'))
        else:
            lod = 'Lift-Off Distance: {0}\\n'.format(razer['lod'])

    stages = [compact_dpi(stage) for stage in razer.get('dpi_stages') or []]
    if stages:
        active_stage = razer.get('active_stage', 0) - 1  # 1-based in JSON
        if 0 <= active_stage < len(stages):
            stages[active_stage] = ICON_ACTIVE_STAGE + stages[active_stage]

    name = razer.get('name', 'Unknown device')
    polling_rate = razer.get('polling_rate', '?')
    firmware = razer.get('firmware', '?')
    serial = razer.get('serial', '?')

    if stages and 'battery' in razer:
        return ('{{"text": "' + ICON_MOUSE + ' DPI: {0} {1} {2}", "tooltip": "{3}\\nCharging: {4}'
                '\\nBattery: {2}\\nBattery Threshold: {5}\\nIDLE Time: {6}s\\nPolling Rate: {7}hz\\n{8}'
                'DPI: {0}\\nDPI Stages: {9}\\nFirmware: {10}\\nSerial: {11}"}}').format(
                    dpi,
                    battery_icon(razer),
                    razer['battery'],
                    name,
                    razer.get('charging', False),
                    razer.get('battery_threshold', '?'),
                    razer.get('idle_time', '?'),
                    polling_rate,
                    lod,
                    ' '.join(stages),
                    firmware,
                    serial)

    if stages:
        return ('{{"text": "' + ICON_MOUSE + ' DPI: {0}", "tooltip": "{1}\\nDPI: {0}\\nDPI Stages: {2}'
                '\\nPolling Rate: {3}hz\\nFirmware: {4}\\nSerial: {5}"}}').format(
                    dpi,
                    name,
                    ' '.join(stages),
                    polling_rate,
                    firmware,
                    serial)

    return ('{{"text": "' + ICON_MOUSE + ' DPI: {0}", "tooltip": "{1}\\nDPI: {0}\\nPolling Rate: {2}hz'
            '\\nFirmware: {3}\\nSerial: {4}"}}').format(
                dpi,
                name,
                polling_rate,
                firmware,
                serial)


def main():
    try:

        process = subprocess.run(
            [razerctl, '--json', '--device', str(device_id)],
            stdout=subprocess.PIPE,
            # razerctl reports problems on stderr; keep stdout parseable.
            stderr=subprocess.DEVNULL,
            universal_newlines=True,
            timeout=30
        )

        razer = json.loads(process.stdout)
        print(format_device(razer))

    except Exception as error:
        print(error, file=sys.stderr)
        print(ERROR_JSON)
        sys.exit(1)


if __name__ == "__main__":
    main()
