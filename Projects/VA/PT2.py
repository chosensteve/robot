import pyaudio

p = pyaudio.PyAudio()
for i in range(p.get_device_count()):
    info = p.get_device_info_by_index(i)
    if info["maxInputChannels"] > 0:
        if 'Microphone' in {info['name']}: 
            print(f"Index {i}: {info['name']} (Default Sample Rate: {int(info['defaultSampleRate'])})")
p.terminate()