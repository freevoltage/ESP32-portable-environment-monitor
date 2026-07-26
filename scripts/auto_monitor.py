Import("env")

def after_upload(source, target, env):
    import subprocess
    # --no-reconnect: when ESP32 enters deep sleep, USB disconnects.
    # Without this, the monitor reconnects — opening the serial port
    # triggers the CP210x auto-reset circuit (DTR HIGH→LOW via capacitor),
    # causing a hard reset and infinite boot loop.
    subprocess.Popen(["pio", "device", "monitor", "--no-reconnect"])

env.AddPostAction("upload", after_upload)
