# ==== CONFIG ====
MOUNT_POINT="/mnt/d-drive"
DRIVE_UUID="8A1062EB1062DDA7"
VENV_PATH="/home/dev/flask-venv"
FLASK_APP_PATH="/home/dev/Desktop/NAS/uploader.py"
FLASK_PORT=5000
LOG_FILE="/home/dev/Desktop/NAS/nas.log"

# === Drive Mount ===
if ! mountpoint -q "$MOUNT_POINT"; then
  echo "Mounting backup drive..."
  sudo mount UUID=$DRIVE_UUID "$MOUNT_POINT"
fi
echo "*Backup drive mounted."

# === Flask-Venv ===
echo "*Activating virtual environment..."
source /home/dev/flask-venv/bin/activate

# === NAS ===
echo "*Starting NAS server on http://192.168.1.8:$FLASK_PORT"
export FLASK_APP="$FLASK_APP_PATH"
export FLASK_ENV=production
nohup flask run --host=0.0.0.0 --port=$FLASK_PORT > "$LOG_FILE" 2>&1 &

