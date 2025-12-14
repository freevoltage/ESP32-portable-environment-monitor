Import("env")

def after_upload(source, target, env):
    import subprocess
    subprocess.Popen(["pio", "device", "monitor"])

env.AddPostAction("upload", after_upload)
