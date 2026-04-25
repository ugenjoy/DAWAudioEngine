# Raspberry Pi Deployment Guide

Complete guide to build, configure and deploy the DAW Audio Engine on Raspberry Pi.

## Compatibility

- **JUCE 8.x**: Official ARM/Raspberry Pi support
- **Crow WebSocket**: ARM compatible (header-only)
- **ASIO standalone**: All architectures (header-only)
- **nlohmann/json**: All architectures (header-only)
- **No x86-specific code** in the sources

### Recommended hardware

- **Raspberry Pi 4** (2GB+ RAM) - Recommended
- **Raspberry Pi 3B+** - Works but slower compilation
- **USB audio interface** (class-compliant) - Required for quality audio
- **Fast SD card** - Class 10 / U3 minimum

### Estimated build time

- **Raspberry Pi 4**: ~30-40 min (first build)
- **Raspberry Pi 3**: ~45-60 min
- Incremental builds: 2-5 min

---

## Step 1: System dependencies

### System update

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### Build tools

```bash
sudo apt-get install -y \
    cmake \
    build-essential \
    git \
    pkg-config
```

### JUCE dependencies

```bash
sudo apt-get install -y \
    libasound2-dev \
    libfreetype6-dev \
    libfreetype-dev \
    libfontconfig1-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev
```

> `libfontconfig1-dev` is required to build `juceaide` (internal JUCE tool).

### JACK audio server

JACK is required for low-latency USB audio on Raspberry Pi. The built-in ALSA driver has timing issues with USB interfaces at 48kHz on the Pi's xHCI controller.

```bash
sudo apt-get install -y jackd2 libjack-jackd2-dev
```

When prompted about real-time privileges, select **Yes**.

---

## Step 2: Clone the project

```bash
cd ~
git clone <REPOSITORY_URL> daw
cd daw

# Initialize JUCE submodule (CRITICAL!)
git submodule update --init --recursive
```

> Without the submodule, JUCE will be missing and compilation will fail.

---

## Step 3: System configuration for real-time audio

### CPU governor

Set the CPU to `performance` mode to prevent frequency scaling during audio processing:

```bash
# Create a systemd service for persistent CPU governor
sudo tee /etc/systemd/system/cpu-performance.service << 'EOF'
[Unit]
Description=Set CPU governor to performance

[Service]
Type=oneshot
ExecStart=/bin/sh -c 'echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor'

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl enable cpu-performance
sudo systemctl start cpu-performance
```

### Real-time audio limits

```bash
# Add real-time limits for the audio group
sudo tee -a /etc/security/limits.conf << 'EOF'
@audio   -  rtprio     95
@audio   -  memlock    unlimited
@audio   -  nice       -20
EOF

# Add user to audio group
sudo usermod -aG audio $USER
```

> A reboot is required for limits.conf changes to take effect.

### Verify configuration

After reboot:

```bash
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor  # → performance
ulimit -r                                                    # → 95
groups $USER                                                 # → should include "audio"
```

---

## Step 4: Build

```bash
cd ~/daw/backend
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Verify build output

CMake should display:

- `Found ALSA`
- `JACK Audio Connection Kit found - enabling JACK support`
- `Configuring done`

The binary is located at:

```
build/DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

### Architecture-specific optimizations (optional)

**Raspberry Pi 4:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -mcpu=cortex-a72 -mfpu=neon-fp-armv8"
```

**Raspberry Pi 3:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -mcpu=cortex-a53 -mfpu=neon-fp-armv8"
```

---

## Step 5: Audio configuration

### Identify your USB audio interface

```bash
aplay -l
```

Example output:

```
card 0: vc4hdmi0 [vc4-hdmi-0] ...       # HDMI (high latency)
card 1: vc4hdmi1 [vc4-hdmi-1] ...       # HDMI 2
card 2: Pro70795057 [Babyface Pro] ...   # USB interface
```

### Test audio output

```bash
speaker-test -D plughw:2,0 -c 2 -r 48000 -t wav
```

> Replace `2` with your USB interface card number. Press Ctrl+C to stop.

### Start JACK

JACK manages the audio interface and provides low-latency, real-time audio routing. Configure it for your interface:

```bash
# Start JACK (adjust parameters for your interface)
jackd -d alsa -d hw:Pro70795057,0 -r 48000 -p 256 -n 3 -i 12 -o 2 &
```

Parameters:
- `-d hw:<CARD_NAME>,0`: Your USB interface (use card name from `aplay -l`)
- `-r 48000`: Sample rate in Hz
- `-p 256`: Period size in samples (lower = less latency, more CPU)
- `-n 3`: Number of periods (3 recommended for USB on Pi, absorbs USB jitter)
- `-i 12`: Number of input channels
- `-o 2`: Number of output channels

> Adjust `-p` based on your needs: 256 (~16ms latency), 512 (~32ms). If you hear crackling, increase the period size.

### Verify JACK

```bash
# List JACK ports
jack_lsp
```

You should see `system:capture_*` and `system:playback_*` ports.

---

## Step 6: Launch the application

```bash
cd ~/daw/backend/build
./DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

In the frontend audio settings, select **JACK** as the driver (not ALSA).

### Expected output

```
=== DAW Audio Engine - Starting ===
Audio initialized:
- Buffer size: 256 samples
- Sample rate: 48000 Hz
- Input channels: 12
- Output channels: 2
- Ready to play!
WebSocket server starting on port 8080
```

---

## Production deployment

### JACK service (auto-start at boot)

A systemd service template is provided in `backend/jack-audio.service`.

> **Important**: Edit `jack-audio.service` to match your audio interface card name and desired settings (sample rate, buffer size, channels) before installing.

```bash
sudo cp ~/daw/backend/jack-audio.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable jack-audio
sudo systemctl start jack-audio
sudo systemctl status jack-audio
```

### DAW Audio Engine service

The DAW service (`backend/daw-audio-engine.service`) is configured to start **after** JACK. It includes real-time scheduling priority and security hardening.

```bash
sudo cp ~/daw/backend/daw-audio-engine.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable daw-audio-engine
sudo systemctl start daw-audio-engine
sudo systemctl status daw-audio-engine
```

### nginx reverse proxy

A nginx configuration is provided in `backend/nginx-daw-audio-engine.conf`. It exposes the WebSocket (`/ws`) and health check (`/health`) on port 80.

```bash
sudo apt-get install -y nginx
sudo cp ~/daw/backend/nginx-daw-audio-engine.conf /etc/nginx/sites-available/daw-audio-engine
sudo ln -sf /etc/nginx/sites-available/daw-audio-engine /etc/nginx/sites-enabled/daw-audio-engine
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t
sudo systemctl restart nginx
sudo systemctl enable nginx
```

### Frontend static files

The frontend is served from `/var/www/daw-frontend` (nginx cannot access home directories by default).

```bash
# Install Node.js
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo bash -
sudo apt-get install -y nodejs

# Install pnpm
sudo npm install -g pnpm

# Build the frontend
cd ~/daw/frontend
pnpm install
pnpm build

# Deploy to /var/www
sudo mkdir -p /var/www/daw-frontend
sudo cp -r ~/daw/frontend/dist/* /var/www/daw-frontend/
```

> After each frontend rebuild, run `sudo cp -r` again to update the served files.

### Test the deployment

From another machine on the network:

```bash
# Health check
curl http://<RASPBERRY_PI_IP>/health

# WebSocket
npx wscat -c ws://<RASPBERRY_PI_IP>/ws

# Frontend: open http://<RASPBERRY_PI_IP> in a browser
```

### Logs

```bash
# JACK server logs
sudo journalctl -u jack-audio -f

# DAW Audio Engine logs
sudo journalctl -u daw-audio-engine -f

# nginx logs
sudo tail -f /var/log/nginx/error.log
```

---

## Troubleshooting

### `ft2build.h: No such file or directory`

Missing FreeType headers:

```bash
sudo apt-get install -y libfontconfig1-dev libfreetype-dev
rm -rf ~/daw/backend/build
cd ~/daw/backend && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### `undefined reference to __atomic_*`

On Raspberry Pi 3 and earlier, link the atomic library explicitly:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-latomic"
```

### JACK not found during CMake

```bash
sudo apt-get install -y jackd2 libjack-jackd2-dev
```

Then reconfigure: `cmake .. -DCMAKE_BUILD_TYPE=Release`

### No sound / Audio device error

```bash
# Check group membership
groups $USER  # should include "audio"

# If not:
sudo usermod -aG audio $USER
# Then log out and log back in
```

### JACK fails to start

```bash
# Check if another process uses the audio device
fuser -v /dev/snd/*

# Check JACK logs
sudo journalctl -u jack-audio --no-pager -n 20

# Test JACK manually
jackd -d alsa -d hw:Pro70795057,0 -r 48000 -p 512 -n 3 -i 12 -o 2
```

### Audio crackling with JACK

Increase the period size (`-p`) or number of periods (`-n`):

```bash
# More conservative settings
jackd -d alsa -d hw:Pro70795057,0 -r 48000 -p 512 -n 4 -i 12 -o 2
```

Also verify:
- CPU governor is `performance`: `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`
- Real-time limits are set: `ulimit -r` should return `95`
- Power supply is adequate (3A+ for Pi 4 with USB audio)

### Port 8080 already in use

```bash
sudo netstat -tulpn | grep :8080
sudo kill -9 <PID>
```

---

## Architecture notes

### Why JACK instead of ALSA directly?

USB audio interfaces on Raspberry Pi (via the VL805 xHCI controller) have timing issues when accessed directly through ALSA. Symptoms include:

- **At 44.1kHz**: Audio plays ~9% too fast (hardware clock runs at 48kHz regardless)
- **At 48kHz**: Crackling/xruns even with large buffers (>5000 samples)

JACK solves this by managing USB audio timing with its own real-time threads and triple-buffering (`-n 3`), absorbing the USB jitter that the raw ALSA driver cannot handle.

### Audio driver selection

| Driver | Use case | Latency |
|--------|----------|---------|
| **JACK** | USB audio on Raspberry Pi (recommended) | ~5-16ms |
| **ALSA** | Built-in audio, HDMI, or desktop Linux | Variable |

Sample rate and buffer size are configured at the **JACK server level** (in `jack-audio.service`), not per-client. The application inherits these settings when connecting to JACK.

---

## Resources

- [JUCE Documentation](https://docs.juce.com/)
- [JACK Audio Connection Kit](https://jackaudio.org/)
- [Raspberry Pi Audio Documentation](https://www.raspberrypi.com/documentation/computers/os.html#audio)
- [ALSA Project](https://www.alsa-project.org/)
