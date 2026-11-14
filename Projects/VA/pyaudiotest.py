import wave
import time
import sys
import pyaudio

if len(sys.argv) < 2:
    print(f'Plays a wave file. Usage: {sys.argv[0]} filename.wav')
    sys.exit(-1)

with wave.open(sys.argv[1], 'rb') as wf:
    bytes_per_frame = wf.getsampwidth() * wf.getnchannels()
    play_count = 1  # Track how many times we've played
    
    # Define callback for playback
    def callback(in_data, frame_count, time_info, status):
        global play_count
        data = wf.readframes(frame_count)
        
        # If data is shorter than requested, we hit the end of the file
        if len(data) < frame_count * bytes_per_frame:
            if play_count < 2:
                play_count += 1
                wf.rewind()  # Reset file pointer to the beginning
                
                # Read the missing frames to fill the buffer and prevent the stream from stopping
                missing_frames = frame_count - (len(data) // bytes_per_frame)
                data += wf.readframes(missing_frames)
            else:
                # On the second pass, let it end naturally
                return (data, pyaudio.paComplete)
                
        return (data, pyaudio.paContinue)

    # Instantiate PyAudio
    p = pyaudio.PyAudio()

    # Open stream
    stream = p.open(format=p.get_format_from_width(wf.getsampwidth()),
                    channels=wf.getnchannels(),
                    rate=wf.getframerate(),
                    output=True,
                    stream_callback=callback)

    # Wait for stream to finish
    while stream.is_active():
        time.sleep(0.1)

    # Clean up
    stream.close()
    p.terminate()