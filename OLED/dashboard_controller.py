import serial
import time
import spotipy
import os
from spotipy.oauth2 import SpotifyOAuth
import keyboard
import datetime

# --- CONFIGURATION ---
os.chdir(os.path.dirname(os.path.abspath(__file__)))

COM_PORT = 'COM7'
BAUD_RATE = 115200

# Spotify API Keys
CLIENT_ID = "c5c88bce0d9d4bd98bc192ffa8c8386d"
CLIENT_SECRET = "95154654e2654320aa6109658cfea19c"
REDIRECT_URI = "https://example.com/callback"

# Set up Spotipy Authentication
scope = "user-read-currently-playing"
try:
    auth_manager = SpotifyOAuth(client_id=CLIENT_ID, client_secret=CLIENT_SECRET, redirect_uri=REDIRECT_URI, scope=scope)
    sp = spotipy.Spotify(auth_manager=auth_manager)
except Exception:
    pass # Will be handled in main

# --- TAB MANAGER ---
TAB_SPOTIFY = 0
TAB_CLOCK = 1
TOTAL_TABS = 2

current_tab = TAB_CLOCK
tab_switch_time = 0.0

def next_tab():
    global current_tab, tab_switch_time
    current_tab = (current_tab + 1) % TOTAL_TABS
    tab_switch_time = time.time()
    tab_name = "CLOCK" if current_tab == 1 else "SPOTIFY"
    print(f"\n>>> SWITCHED TO TAB: {tab_name} <<<")

def prev_tab():
    global current_tab, tab_switch_time
    current_tab = (current_tab - 1) % TOTAL_TABS
    tab_switch_time = time.time()
    tab_name = "CLOCK" if current_tab == 1 else "SPOTIFY"
    print(f"\n>>> SWITCHED TO TAB: {tab_name} <<<")

# Bind Global Hotkeys!
keyboard.add_hotkey('shift+page down', next_tab)
keyboard.add_hotkey('shift+page up', prev_tab)


def get_media_info():
    """Fetches Spotify Status."""
    try:
        current_track = sp.current_user_playing_track()
        if current_track is not None and current_track.get('is_playing'):
            track_name = current_track['item']['name'].replace("|", "")
            artists = [artist['name'] for artist in current_track['item']['artists']]
            artist_name = ", ".join(artists).replace("|", "")
            progress_ms = current_track['progress_ms']
            duration_ms = current_track['item']['duration_ms']
            
            return f"{track_name}|{artist_name}|{progress_ms}|{duration_ms}|PLAYING"
        else:
            return "Spotify|Ready|0|100|PAUSED"
    except Exception as e:
        return f"API ERROR|{str(e)[:15]}|0|100|ERROR"


def main():
    print("====================================")
    print(" PC Dashboard Server v3.0 (TABS) ")
    print("====================================")
    print("HOTKEYS ENABLED: Shift + Page Up/Down")
    
    print("\nVerifying Spotify connection...")
    try:
        sp.current_user_playing_track()
        print("Spotify Authentication Successful!")
    except spotipy.oauth2.SpotifyOauthError:
        print("\n[ERROR] Failed to authenticate with Spotify.")
        return
        
    print(f"\nConnecting to ESP32 on {COM_PORT}...")
    try:
        with serial.Serial(COM_PORT, BAUD_RATE, timeout=1) as ser:
            time.sleep(2) # Give ESP32 time to reboot when opening connection
            print("Connected! Streaming Data...")
            
            # Streaming Loop
            while True:
                # If we recently switched tabs, show the App Switcher Menu
                if time.time() - tab_switch_time < 2.0:
                    tab_name = "Clock" if current_tab == TAB_CLOCK else "Spotify"
                    payload = f"M|{tab_name}\n"
                    ser.write(payload.encode('utf-8'))
                    print(f"[{tab_name} MENU]")
                    time.sleep(0.5) # Fast menu updates
                    continue
                    
                if current_tab == TAB_SPOTIFY:
                    media_data = get_media_info()
                    # Prefix with 'S'
                    payload = f"S|{media_data}\n"
                    ser.write(payload.encode('utf-8'))
                    
                elif current_tab == TAB_CLOCK:
                    now = datetime.datetime.now()
                    # Example format: 14:05 | 04 April 2026
                    time_str = now.strftime("%H:%M")
                    date_str = now.strftime("%d %B %Y")
                    # Prefix with 'C'
                    payload = f"C|{time_str}|{date_str}\n"
                    ser.write(payload.encode('utf-8'))
                    
                time.sleep(1) # We can update clock and spotify exactly once per second
                
    except serial.SerialException:
        print(f"\n[ERROR] FAILED to connect to {COM_PORT}.")
        print("CRITICAL: Make sure the Arduino IDE Serial Monitor is CLOSED!")
    except KeyboardInterrupt:
        print("\nShutting down...")

if __name__ == '__main__':
    # WARNING: To use 'keyboard' module hooks on Windows, you must run this script from an Administrator PowerShell!
    main()
