Import("env")

def after_upload(source, target, env):
    import subprocess
    # --rts false --dtr false: prevent DTR/RTS toggling on reconnect.
    # Without this, the serial monitor toggles DTR/RTS when the ESP32
    # enters deep sleep (USB disconnects) and reconnects — this triggers
    # a hard reset, causing an infinite boot loop.
    subprocess.Popen(["pio", "device", "monitor", "--rts", "false", "--dtr", "false"])

env.AddPostAction("upload", after_upload)
