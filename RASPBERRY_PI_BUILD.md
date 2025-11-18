# Compilation sur Raspberry Pi OS

Guide complet pour compiler et exécuter le DAW Audio Engine sur Raspberry Pi.

## Compatibilité

✅ **Le projet est compatible Raspberry Pi OS**

- **JUCE 8.0.10** : Support officiel ARM/Raspberry Pi
- **Crow WebSocket** : Compatible ARM (header-only)
- **ASIO standalone** : Compatible toutes architectures
- **nlohmann/json** : Compatible ARM (header-only)
- **Aucun code x86-specific** dans les sources

### Matériel recommandé

- **Raspberry Pi 4** (2GB+ RAM) - Recommandé
- **Raspberry Pi 3B+** - Fonctionne mais plus lent
- **Interface audio USB** - Fortement recommandé pour meilleure qualité/latence
- **Carte SD rapide** - Classe 10 ou U3 minimum

### Temps de compilation estimé

- **Raspberry Pi 4** : ~30-40 minutes (première compilation)
- **Raspberry Pi 3** : ~45-60 minutes
- Compilations incrémentales : 2-5 minutes

---

## Étape 1 : Installation des dépendances

### Mise à jour du système

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### Outils de compilation essentiels

```bash
sudo apt-get install -y \
    cmake \
    build-essential \
    git \
    pkg-config
```

### Dépendances JUCE

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

⚠️ **Important** : `libfontconfig1-dev` est indispensable pour compiler `juceaide` (outil interne JUCE).

### JACK (optionnel - détection automatique)

```bash
# Si vous voulez utiliser JACK Audio Connection Kit
sudo apt-get install -y \
    jackd2 \
    libjack-jackd2-dev
```

**Note** :
- Le support JACK est **automatiquement détecté** lors de la configuration CMake
- Si JACK est installé, il sera activé automatiquement
- Si JACK n'est pas présent, le projet utilisera ALSA uniquement
- Pour débuter, ALSA seul est suffisant et plus simple à configurer

---

## Étape 2 : Clonage du projet

```bash
# Aller dans votre répertoire de travail
cd ~

# Cloner le repository
git clone <URL_DU_REPOSITORY> daw
cd daw

# Initialiser le submodule JUCE (CRITIQUE !)
git submodule update --init --recursive
```

⚠️ **N'oubliez pas le submodule** : Sans cette commande, JUCE ne sera pas présent et la compilation échouera.

---

## Étape 3 : Configuration avec CMake

```bash
# Créer le dossier de build
cd backend
mkdir build
cd build

# Configurer le projet en mode Release
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Vérifications après CMake

La sortie devrait afficher :
- ✅ `Found ALSA`
- ✅ `JACK Audio Connection Kit found - enabling JACK support` (si JACK installé)
  - OU `JACK not found - using ALSA only` (si JACK absent)
- ✅ `Configuring juceaide` → `Building juceaide` (peut prendre 5-10 minutes)
- ✅ `Configuring done`

Si vous voyez des erreurs, consultez la section [Dépannage](#dépannage).

---

## Étape 4 : Compilation

```bash
# Compiler avec tous les cores disponibles
make -j$(nproc)
```

☕ **Patience** : La première compilation prend 30-45 minutes sur Raspberry Pi 4.

### Compilation réussie

Vous devriez voir à la fin :
```
[100%] Built target DAWAudioEngine
```

Le binaire se trouve dans :
```
build/DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

### Vérifier le binaire

```bash
# Vérifier que c'est bien un exécutable ARM
file DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

Sortie attendue :
```
ELF 32-bit LSB executable, ARM, ...
```

---

## Étape 5 : Configuration audio

### Lister les périphériques audio

```bash
aplay -l
```

Exemple de sortie :
```
card 0: vc4hdmi0 [vc4-hdmi-0] ...       # HDMI (haute latence)
card 1: vc4hdmi1 [vc4-hdmi-1] ...       # HDMI 2
card 2: Pro70795057 [Babyface Pro] ...  # Interface USB
```

### Recommandations

- **Interface USB** (carte 2 dans l'exemple) : Meilleure qualité, latence plus basse
- **Audio intégré 3.5mm** : Latence élevée (~50-100ms), qualité moyenne
- **HDMI** : Fonctionne mais latence variable

### Tester la sortie audio

```bash
# Tester avec interface USB (remplacer X par le numéro de carte)
speaker-test -D plughw:2,0 -c 2 -t wav

# Tester avec audio intégré
speaker-test -D plughw:0,0 -c 2 -t wav
```

Appuyez sur Ctrl+C pour arrêter.

---

## Étape 6 : Lancement de l'application

```bash
cd ~/daw/backend/build
./DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

### Sortie attendue

```
=== DAW Audio Engine - Starting ===
[Messages d'initialisation JUCE]
WebSocket server started on port 8080
```

### Avertissements normaux (non bloquants)

Si vous voyez :
```
Cannot connect to server socket err = No such file or directory
jack server is not running or cannot be started
```

**C'est normal !** JUCE essaie JACK d'abord, puis utilise ALSA automatiquement. Le son fonctionne via ALSA.

Pour supprimer cet avertissement, voir [Configuration JACK](#utilisation-de-jack-optionnel).

---

## Test de l'application

### Vérifier le WebSocket

Depuis un autre terminal ou une autre machine sur le réseau :

```bash
# Vérifier que le port 8080 écoute
netstat -tuln | grep 8080

# Ou depuis une autre machine
telnet <IP_RASPBERRY_PI> 8080
```

### Tester la lecture audio

Connectez votre frontend/client au WebSocket de la Raspberry Pi et testez la lecture audio.

---

## Dépannage

### Erreur : `ft2build.h: No such file or directory`

**Cause** : Headers FreeType manquants

**Solution** :
```bash
sudo apt-get install -y libfontconfig1-dev libfreetype-dev
cd ~/daw/backend
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Erreur : `jack/jack.h: No such file or directory`

**Cause** : Cette erreur ne devrait plus apparaître avec la détection automatique de JACK. Si elle persiste, le cache CMake est peut-être corrompu.

**Solution** :
```bash
# Supprimer le build et reconfigurer
cd ~/daw/backend
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Lors de la configuration, vous devriez voir :
- `JACK Audio Connection Kit found` (si installé)
- `JACK not found - using ALSA only` (si absent)

### Erreur de link : `undefined reference to __atomic_*`

**Cause** : Sur Raspberry Pi 3 et antérieurs, la bibliothèque atomique doit être liée explicitement

**Solution** :
```bash
cd ~/daw/backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-latomic"
make -j$(nproc)
```

### Pas de son / Erreur de périphérique audio

**Vérifier les permissions** :
```bash
# Vérifier que vous êtes dans le groupe audio
groups $USER

# Si "audio" n'apparaît pas :
sudo usermod -aG audio $USER
# Puis déconnectez-vous et reconnectez-vous
```

**Vérifier ALSA** :
```bash
# Lister les devices
aplay -L

# Tester la sortie
speaker-test -D default -c 2
```

### Port 8080 déjà utilisé

```bash
# Voir ce qui utilise le port
sudo netstat -tulpn | grep :8080

# Tuer le processus si nécessaire
sudo kill -9 <PID>
```

---

## Utilisation de JACK (optionnel)

JACK offre une latence plus basse et un routage audio flexible, mais nécessite une configuration supplémentaire.

### Installation de JACK

```bash
sudo apt-get install -y jackd2 libjack-jackd2-dev

# Configurer les permissions temps-réel
sudo dpkg-reconfigure -p high jackd2
# Répondre "Yes" pour activer les priorités temps-réel
```

### Configuration des permissions temps-réel

Le fichier `/etc/security/limits.d/audio.conf` devrait contenir :
```
@audio   -  rtprio     95
@audio   -  memlock    unlimited
```

**Redémarrer** la Raspberry Pi après configuration :
```bash
sudo reboot
```

### Vérification des permissions

Après reconnexion :
```bash
# Devrait afficher 95
ulimit -r

# Devrait afficher "unlimited"
ulimit -l
```

### Lancement de JACK

**Terminal 1** : Démarrer le serveur JACK
```bash
# Pour interface USB (remplacer hw:2 par votre carte)
jackd -dalsa -dhw:2 -r48000 -p512 -n3
```

**Paramètres expliqués** :
- `-dalsa` : Backend ALSA
- `-dhw:2` : Carte audio 2 (voir `aplay -l`)
- `-r48000` : Sample rate 48kHz
- `-p512` : Buffer 512 samples (~10.7ms de latence)
- `-n3` : 3 périodes (plus stable sur Raspberry Pi)

**Terminal 2** : Lancer l'application
```bash
cd ~/daw/backend/build
./DAWAudioEngine_artefacts/Release/DAWAudioEngine
```

### Connecter les ports audio

**Terminal 3** : Vérifier les ports
```bash
# Lister tous les ports JACK
jack_lsp -c
```

Si votre application utilise JACK, vous verrez :
```
system:capture_1
system:playback_1
system:playback_2
DAWAudioEngine:output_1
DAWAudioEngine:output_2
```

**Connecter manuellement** :
```bash
jack_connect DAWAudioEngine:output_1 system:playback_1
jack_connect DAWAudioEngine:output_2 system:playback_2
```

### Activer le support JACK

Le support JACK est détecté et activé automatiquement. Pour l'activer :

```bash
# 1. Installer JACK
sudo apt-get install -y libjack-jackd2-dev

# 2. Recompiler (JACK sera détecté automatiquement)
cd ~/daw/backend
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

Vous devriez voir le message :
```
-- JACK Audio Connection Kit found - enabling JACK support
```

Puis compiler :
```bash
make -j$(nproc)
```

Vérifier que JACK est bien lié :
```bash
ldd ./DAWAudioEngine_artefacts/Release/DAWAudioEngine | grep jack
```

### Optimisations JACK pour Raspberry Pi

**Pour audio intégré** (haute latence) :
```bash
jackd -dalsa -dhw:0 -r48000 -p2048 -n3
```

**Pour interface USB** (basse latence) :
```bash
jackd -dalsa -dhw:2 -r48000 -p256 -n2
```

**Pour RME Babyface Pro** (très basse latence) :
```bash
jackd -dalsa -dhw:2 -r48000 -p128 -n2
```

---

## Optimisations de compilation

### Pour Raspberry Pi 4 spécifiquement

```bash
cd ~/daw/backend/build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -mcpu=cortex-a72 -mfpu=neon-fp-armv8"
make -j4
```

### Pour Raspberry Pi 3

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -mcpu=cortex-a53 -mfpu=neon-fp-armv8"
make -j4
```

### Pour Raspberry Pi 2

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -mcpu=cortex-a7 -mfpu=neon-vfpv4"
make -j4
```

---

## Déploiement en production

### Lancer au démarrage avec systemd

Créer le fichier de service :
```bash
sudo nano /etc/systemd/system/daw-audio-engine.service
```

Contenu :
```ini
[Unit]
Description=DAW Audio Engine
After=network.target sound.target

[Service]
Type=simple
User=ugo
WorkingDirectory=/home/ugo/daw/backend/build
ExecStart=/home/ugo/daw/backend/build/DAWAudioEngine_artefacts/Release/DAWAudioEngine
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Activer et démarrer :
```bash
sudo systemctl enable daw-audio-engine
sudo systemctl start daw-audio-engine
sudo systemctl status daw-audio-engine
```

### Logs

```bash
# Voir les logs en temps réel
sudo journalctl -u daw-audio-engine -f

# Voir les derniers logs
sudo journalctl -u daw-audio-engine -n 50
```

---

## Performances attendues

### Raspberry Pi 4 (4GB)

- ✅ Compilation : 30-40 minutes
- ✅ Latence audio : 5-20ms (USB), 50-100ms (intégré)
- ✅ Utilisation CPU : 15-30% par piste
- ✅ Pistes simultanées : 8-16 tracks

### Raspberry Pi 3B+

- ✅ Compilation : 45-60 minutes
- ✅ Latence audio : 10-30ms (USB), 50-100ms (intégré)
- ⚠️ Utilisation CPU : 20-40% par piste
- ⚠️ Pistes simultanées : 4-8 tracks

### Raspberry Pi Zero/2

❌ Non recommandé pour audio temps-réel
✅ Peut fonctionner pour lecture seule (non temps-réel)

---

## Conseils et bonnes pratiques

### Audio

- 🎛️ **Privilégier une interface USB** de qualité (class-compliant)
- 🔊 **Éviter l'audio intégré** pour applications critiques
- ⏱️ **Ajuster les buffers** selon vos besoins de latence vs stabilité
- 🎚️ **Vérifier les niveaux** avec `alsamixer`

### Système

- 💾 **Utiliser une carte SD rapide** (U3 minimum)
- 🌡️ **Surveiller la température** : `vcgencmd measure_temp`
- ⚡ **Utiliser une alimentation de qualité** (3A minimum pour Pi 4)
- 🔌 **Éviter les hubs USB non alimentés**

### Développement

- 📝 **Compilations incrémentales** : Seul le code modifié est recompilé
- 🔄 **Git workflow** : Développer sur PC, compiler sur Pi
- 🧪 **Tester avant déploiement** : Mode Debug pour développement
- 🚀 **Mode Release pour production** : Optimisations activées

---

## Ressources supplémentaires

- [JUCE Documentation](https://docs.juce.com/)
- [Raspberry Pi Audio Documentation](https://www.raspberrypi.com/documentation/computers/os.html#audio)
- [JACK Audio Connection Kit](https://jackaudio.org/)
- [ALSA Project](https://www.alsa-project.org/)

---

## Support

Pour tout problème spécifique au projet, ouvrir une issue sur le repository GitHub.

Pour les problèmes généraux Raspberry Pi :
- [Forum Raspberry Pi](https://forums.raspberrypi.com/)
- [JUCE Forum](https://forum.juce.com/)

---

**Compilation réussie ? Bon développement audio sur Raspberry Pi ! 🎵🎛️**
