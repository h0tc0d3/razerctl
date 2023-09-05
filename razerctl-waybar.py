import sys
import json
import subprocess

device_id = 1  # Device ID
razerctl = 'razerctl'  # Path to razerctl binary


def main():
    try:

        process = subprocess.run(
            [razerctl, '--json', '--device', str(device_id)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )

        if process.stdout:

            razer = json.loads(process.stdout)

            [dpi_x, dpi_y] = razer["dpi"].split("x")
            if dpi_x == dpi_y:
                razer["dpi"] = dpi_x

            if "dpi_stages" in razer:

                dpi_stages = len(razer["dpi_stages"])
                for i in range(0, dpi_stages):
                    [dpi_x, dpi_y] = razer["dpi_stages"][i].split("x")
                    if dpi_x == dpi_y:
                        razer["dpi_stages"][i] = dpi_x

                active_stage = razer["active_stage"] - 1
                razer["dpi_stages"][active_stage] = "" + razer["dpi_stages"][active_stage]

                lod = ""
                if "lod" in razer:
                    if razer["lod_async"]:
                        lod = "Lift-Off Distance: {0}\\nLand Distance: {1}\\n".format(razer["lod"], razer["ld"])
                    else:
                        lod = "Lift-Off Distance: {0}\\n".format(razer["lod"])

                if "battery" in razer:

                    if razer["charging"]:
                        battery_icon = ''
                    else:
                        battery_value = int(razer["battery"][:-1])
                        if battery_value >= 85:
                            battery_icon = ''
                        elif 65 <= battery_value < 85:
                            battery_icon = ''
                        elif 40 <= battery_value < 65:
                            battery_icon = ''
                        elif 10 <= battery_value < 40:
                            battery_icon = ''
                        else:
                            battery_icon = ''

                    print(
                        '{{"text": " DPI: {0} {1} {2}", "tooltip": "{3}\\nCharging: {4}\\nBattery: {2}\\nBattery Threshold: {5}\\nIDLE Time: {6}s\\nPolling Rate: {7}hz\\n{8}DPI: {0}\\nDPI Stages: {9}\\nFirmware: {10}\\nSerial: {11}"}}'.format(
                            razer["dpi"],
                            battery_icon,
                            razer["battery"],
                            razer["name"],
                            razer["charging"],
                            razer["battery_threshold"],
                            razer["idle_time"],
                            razer["polling_rate"],
                            lod,
                            " ".join(razer["dpi_stages"]),
                            razer["firmware"],
                            razer["serial"]
                        ))

                else:

                    print(
                        '{{"text": " DPI: {0}", "tooltip": "{1}\\nDPI: {0}\\nDPI Stages: {2}\\nPolling Rate: {3}hz\\nFirmware: {4}\\nSerial: {5}"}}'.format(
                            razer["dpi"],
                            razer["name"],
                            " ".join(razer["dpi_stages"]),
                            razer["polling_rate"],
                            razer["firmware"],
                            razer["serial"],
                        ))

            else:

                print(
                    '{{"text": " DPI: {0}", "tooltip": "{1}\\nDPI: {0}\\nPolling Rate: {2}hz\\nFirmware: {3}\\nSerial: {4}"}}'.format(
                        razer["dpi"],
                        razer["name"],
                        razer["polling_rate"],
                        razer["firmware"],
                        razer["serial"]
                    ))


    except Exception as error:
        print(error, file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
