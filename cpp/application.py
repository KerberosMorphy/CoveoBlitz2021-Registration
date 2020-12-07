import os
import subprocess
import time
import signal

print('Playing...')

process = None
def kill(signum, frame):
    global process
    print('Killing...')
    if process is None:
        return
    process.kill()
    time.sleep(2)

signal.signal(signal.SIGINT, kill)
signal.signal(signal.SIGTERM, kill)

subprocess.Popen(["chmod", "+x", "a.out"], shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE).wait()
process = subprocess.Popen(["./a.out"], shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)

rawOut, rawErrors = process.communicate()
# process.wait()
out = rawOut.decode('utf-8')
errors = rawErrors.decode('utf-8')

print('Stdout:',  out)
print('Stderr:', errors)
