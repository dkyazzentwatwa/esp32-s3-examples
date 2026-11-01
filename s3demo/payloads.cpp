#include "payloads.h"

// Benign HID reels adapted from cypher-drive/payloads (fun/ + goodusb/):
// visual theater only — no real access, exfil, persistence, or attack.
// Blobs use the R"PL(...)PL" delimiter so embedded )" sequences don't truncate
// the literal (the plain R"(...)" form silently cut several of these off).
// Typing assumes a US keyboard layout. When opensTerminal is true the HID
// screen opens a Terminal first, so command lists start at the first command.

static const char* fun_ai_recon_dashboard[] = {
  "clear",
  R"PL(echo -e "\033[1;36m╔════════════════════════════════════╗")PL",
  R"PL(echo -e "\033[1;36m║  NEURAL RECON v2.1 - AI ANALYSIS   ║")PL",
  R"PL(echo -e "\033[1;36m╚════════════════════════════════════╝\033[0m")PL",
  R"PL(echo -e "\n\033[94m[INIT] Loading quantum-weighted neural model...")PL",
  R"PL(bash -lc 'for i in $(seq 1 5); do printf "\033[92m[PHASE %d/5] " "$i"; case $i in 1) echo "Feature extraction from network topology";; 2) echo "Anomaly detection via transformer layers";; 3) echo "Cross-referencing threat intelligence feeds";; 4) echo "Generating attack surface heatmap";; 5) echo "Finalizing risk assessment matrix";; esac; sleep 0.6; done')PL",
  R"PL(echo -e "\n\033[1;33m▼ ATTACK SURFACE HEATMAP ▼\033[0m")PL",
  R"PL(bash -lc 'printf "\033[38;5;196m"; for row in 1 2 3 4 5; do for col in $(seq 1 20); do printf "#"; sleep 0.03; done; echo ""; done; printf "\033[0m"')PL",
  R"PL(echo -e "\n\033[1;92m✓ AI ANALYSIS COMPLETE\033[0m")PL",
  R"PL(echo -e "\033[94m• Detected 3 potential entry vectors (SIMULATED)")PL",
  R"PL(echo -e "• Risk Score: 7.2/10 (Educational Estimate)")PL",
  R"PL(echo -e "• Recommended Actions: Patch, Monitor, Train\033[0m")PL",
  R"PL(echo -e "\n\033[1;31m╔════════════════════════════════════╗")PL",
  R"PL(echo -e "║   TRAINING SIMULATION ONLY         ║")PL",
  R"PL(echo -e "║   NO REAL SYSTEMS WERE ACCESSED    ║")PL",
  R"PL(echo -e "╚════════════════════════════════════╝\033[0m")PL",
  R"PL(echo -e "\n\033[96mThis demo illustrates how AI could enhance security reconnaissance.")PL",
  R"PL(echo -e "Always obtain explicit written permission before testing any system.\033[0m")PL",
};

static const char* fun_app_chaos_montage[] = {
  "echo 'SCENE 1/3: APP CASCADE'",
  R"PL(bash -lc 'apps=("Safari" "Calculator" "Chess" "Preview" "Music"); for round in 1 2 3; do for app in "${apps[@]}"; do open -a "$app"; sleep 1.3; done; done; sleep 8' # reel_apps)PL",
  "echo 'SCENE 3/3: MONTAGE ENDED'",
};

static const char* fun_ascii_chaos_console[] = {
  "clear",
  "echo 'SCENE 1/3: BOOTSTRAP ASCII GRID'",
  "echo 'module[vision] module[relay] module[crypto] module[signal] -> preflight'",
  R"PL(bash -lc 'printf "\033[1;36m"; echo ":: uplink handshake accepted"; echo ":: rendering glyph stream"; printf "\033[0;32m"; chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()[]{}<>?/|\\-_=+~"; for i in $(seq 1 200); do line=""; for j in $(seq 1 84); do idx=$((RANDOM % ${#chars})); line+="${chars:$idx:1}"; done; printf "%s\n" "$line"; if [ $((i % 20)) -eq 0 ]; then printf "\033[1;33m[STATUS] relay_%02d synced | cache=%02d%% | entropy=OK\033[0m\n" "$((RANDOM%99+1))" "$((RANDOM%100))"; fi; sleep 0.08; done; printf "\033[1;92mSIMULATION COMPLETE - VISUALS ONLY\033[0m\n"; sleep 8' # reel_ascii)PL",
  "echo 'SCENE 3/3: DROP COMPLETE'",
};

static const char* fun_autotype_drama_scene[] = {
  "echo 'SCENE 1/3: MESSAGE BUILD'",
  R"PL(bash -lc 'printf "%s\n" "[LOG ENTRY]" "I thought the machine was haunted." "Then I realized it was choreography." "No breach. No malware. Just timing, typing, and camera angles." "When the beat drops, hit record." > ~/Desktop/reel_drama_scene.txt; open -a TextEdit ~/Desktop/reel_drama_scene.txt; sleep 12' # reel_drama)PL",
};

static const char* fun_beat_sync_terminal[] = {
  "clear",
  "echo 'SCENE 1/3: 8-16-32 BAR VISUAL PULSE'",
  R"PL(bash -lc 'for bars in 8 16 32; do for i in $(seq 1 $bars); do printf "[BAR %02d/%02d] ########################\r" "$i" "$bars"; sleep 0.22; done; printf "\n"; done; echo "DROP COMPLETE"; sleep 12' # reel_beat)PL",
};

static const char* fun_cinematic_fake_breach[] = {
  "clear",
  "echo 'SCENE 1/3: ACCESS PHASE'",
  R"PL(bash -lc 'printf "\033[32m"; for i in $(seq 1 26); do printf "[%02d] injecting visual payload into relay_%02d ... OK\n" "$i" "$((RANDOM%97+1))"; sleep 0.35; done; printf "\n\033[1;92mSIMULATION COMPLETE - NO REAL ACCESS PERFORMED\033[0m\n"; sleep 10')PL",
};

static const char* fun_example_hello_world[] = {
  "echo 'Hello from an ESP32-S3 acting as a USB keyboard :)'",
  "echo 'This is a harmless demo. Nothing was installed.'",
};

static const char* fun_glitch_desktop_takeover[] = {
  "echo 'SCENE 1/3: VISUAL GLITCH WAVE'",
  R"PL(bash -lc 'for i in $(seq 1 12); do osascript -e "tell application \"System Events\" to set dark mode of appearance preferences to true" >/dev/null 2>&1; sleep 0.4; osascript -e "tell application \"System Events\" to set dark mode of appearance preferences to false" >/dev/null 2>&1; sleep 0.4; done')PL",
  "open -a 'Calculator'; sleep 1; open -a 'Chess'; sleep 1; open -a 'Music'; sleep 1; open -a 'Preview'",
  "echo 'SCENE 3/3: RESET PATH READY'",
};

static const char* fun_matrix_rain_studio[] = {
  "clear",
  "echo 'SCENE 1/3: MATRIX RAIN'",
  R"PL(bash -lc 'export LC_ALL=C; printf "\033[0;32m"; for i in $(seq 1 220); do tr -dc "A-F0-9" </dev/urandom | head -c 64; printf "\n"; sleep 0.18; done; printf "\033[0m"; sleep 8' # reel_matrix)PL",
};

static const char* fun_notification_storm_lite[] = {
  "echo 'SCENE 1/3: NOTIFICATION BURST'",
  R"PL(bash -lc 'for i in $(seq 1 18); do osascript -e "display notification \"Signal spike $i/18\" with title \"REEL MODE\" subtitle \"Visual simulation\""; sleep 0.35; done; sleep 10' # reel_notify)PL",
  "echo 'SCENE 3/3: BURST FINISHED'",
};

static const char* fun_quantum_decrypt_sim[] = {
  "clear",
  R"PL(echo -e "\033[1;35m╔════════════════════════════════════╗")PL",
  R"PL(echo -e "║   QUANTUM DECRYPTOR v3.0 - SIM     ║")PL",
  R"PL(echo -e "║   [EDUCATIONAL DEMO ONLY]          ║")PL",
  R"PL(echo -e "╚════════════════════════════════════╝\033[0m\n")PL",
  R"PL(echo -e "\033[94m[1/4] Initializing quantum register...")PL",
  R"PL(bash -lc 'printf "\033[36m"; for i in $(seq 1 12); do printf "|%d> " "$((RANDOM%2))"; sleep 0.15; done; echo -e "\n\033[0m"')PL",
  R"PL(echo -e "\033[94m[2/4] Entangling qubits via Hadamard gates...")PL",
  R"PL(bash -lc 'printf "\033[95m"; for step in 1 2 3; do printf "  Gate H x%d: " "$step"; for q in $(seq 1 4); do printf "[a|%d>+b|%d>] " "$((RANDOM%2))" "$((1-RANDOM%2))"; sleep 0.2; done; echo ""; done; printf "\033[0m"')PL",
  R"PL(echo -e "\n\033[94m[3/4] Running Shor's algorithm simulation...")PL",
  R"PL(bash -lc 'total=30; for i in $(seq 1 $total); do pct=$((i*100/total)); printf "\033[92m["; for j in $(seq 1 $pct); do printf "#"; done; for j in $(seq $((100-pct))); do printf "-"; done; printf "] %d%%\r" "$pct"; sleep 0.08; done; echo -e "\n\033[0m"')PL",
  R"PL(echo -e "\033[94m[4/4] Collapsing wavefunction...")PL",
  R"PL(echo -e "\n\033[1;93m✨ SIMULATED OUTPUT ✨")PL",
  R"PL(echo -e "  Plaintext: [REDACTED_FOR_DEMO]")PL",
  R"PL(echo -e "  Key Strength: 2048-bit RSA (SIMULATED BREAK)")PL",
  R"PL(echo -e "  Time Elapsed: 0.003s (Quantum Advantage)\033[0m")PL",
  R"PL(echo -e "\n\033[1;31m╔════════════════════════════════════╗")PL",
  R"PL(echo -e "║   THIS IS A VISUAL SIMULATION      ║")PL",
  R"PL(echo -e "║   No real encryption was broken.   ║")PL",
  R"PL(echo -e "║   For educational purposes only.   ║")PL",
  R"PL(echo -e "╚════════════════════════════════════╝\033[0m")PL",
  R"PL(echo -e "\n\033[96mQuantum computing poses future risks to current cryptography.")PL",
  R"PL(echo -e "Organizations should begin planning for post-quantum cryptography.\033[0m")PL",
};

static const char* fun_reveal_and_reset[] = {
  "pkill -f reel_ >/dev/null 2>&1 || true",
  "killall Calculator Chess Music Preview Safari >/dev/null 2>&1 || true",
  "clear",
  "echo '========================================='",
  "echo 'REVEAL: THIS WAS A CHOREOGRAPHED REEL'",
  "echo 'No persistence, no credential access, no stealth.'",
  "echo 'UNDO COMPLETE. YOU ARE BACK TO NORMAL.'",
  "echo '========================================='",
};

static const char* fun_rickroll_macos_safari[] = {
  "open -a Safari 'https://www.youtube.com/watch?v=oHg5SJYRHA0'",
  "sleep 10",
  R"PL(osascript -e 'tell application "Safari" to activate')PL",
  "sleep 6",
};

static const char* goodusb_blockchain_visualizer[] = {
  "clear",
  R"PL(echo -e "\033[1;33m╔════════════════════════════════════╗")PL",
  R"PL(echo -e "║   BLOCKCHAIN LIVE VISUALIZER v1.2  ║")PL",
  R"PL(echo -e "║   Real Data - Educational Demo     ║")PL",
  R"PL(echo -e "╚════════════════════════════════════╝\033[0m\n")PL",
  R"PL(echo -e "\033[94m[1/3] Fetching live BTC price from rate.sx...\033[0m")PL",
  "curl -s --connect-timeout 5 rate.sx/btc 2>/dev/null | head -n 1 || echo '  [Offline mode: using cached estimate]'",
  R"PL(echo -e "\n\033[94m[2/3] Visualizing mempool transaction flow...\033[0m")PL",
  R"PL(bash -lc 'printf "\033[36m"; echo "  PENDING TRANSACTIONS"; for i in $(seq 1 8); do printf "  "; for j in $(seq 1 28); do if [ $((RANDOM%3)) -eq 0 ]; then printf "#"; else printf "."; fi; done; echo ""; sleep 0.12; done; printf "\033[0m"')PL",
  R"PL(echo -e "\n\033[94m[3/3] Generating network health metrics...\033[0m")PL",
  R"PL(bash -lc 'printf "\033[92m"; echo "  Hashrate:     ~450 EH/s (simulated)"; echo "  Mempool Size: ~12,450 tx (estimated)"; echo "  Avg Fee:      ~15 sat/vB (live approx)"; printf "\033[0m\n"')PL",
  R"PL(echo -e "\033[96mThis demo uses ONLY public APIs. No keys, wallets, or personal data.\033[0m")PL",
};

static const char* goodusb_live_chart_crypto[] = {
  "curl rate.sx/btc",
};

static const char* goodusb_multi_open_apps[] = {
  "open -a Safari",
  "open -a Calculator",
  "open -a Calendar",
  "open -a Messages",
  "open -a Music",
};

#define PL(a) a, (int)(sizeof(a) / sizeof((a)[0]))

const Payload PAYLOADS[] = {
  { "AI Recon Dashboard",         "futuristic AI analysis - sim only", true, PL(fun_ai_recon_dashboard)        },
  { "App Chaos Montage",          "'possessed mac' montage",           true, PL(fun_app_chaos_montage)         },
  { "ASCII Chaos Console",        "cafe hacker theater, visual only",  true, PL(fun_ascii_chaos_console)        },
  { "Autotype Drama Scene",       "storytelling monologue",            true, PL(fun_autotype_drama_scene)       },
  { "Beat Sync Terminal",         "rhythm-synced pulses",              true, PL(fun_beat_sync_terminal)         },
  { "Cinematic Fake Breach",      "fake breach - NO real access",      true, PL(fun_cinematic_fake_breach)      },
  { "Hello World",                "harmless terminal test",            true, PL(fun_example_hello_world)        },
  { "Glitch Desktop Takeover",    "dark-mode glitch prank",            true, PL(fun_glitch_desktop_takeover)    },
  { "Matrix Rain Studio",         "green terminal rain",               true, PL(fun_matrix_rain_studio)         },
  { "Notification Storm Lite",    "meme notification burst",           true, PL(fun_notification_storm_lite)    },
  { "Quantum Decrypt Sim",        "sci-fi quantum terminal, sim only", true, PL(fun_quantum_decrypt_sim)        },
  { "Reveal and Reset",           "cleanup + reveal it was staged",    true, PL(fun_reveal_and_reset)           },
  { "Rickroll (Safari)",          "opens the classic video",           true, PL(fun_rickroll_macos_safari)      },
  { "Blockchain Visualizer",      "live public BTC data, read-only",   true, PL(goodusb_blockchain_visualizer)  },
  { "Live Crypto Ticker",         "public BTC price via rate.sx",      true, PL(goodusb_live_chart_crypto)      },
  { "Multi-Open Apps",            "opens several stock mac apps",      true, PL(goodusb_multi_open_apps)        },
};

const int PAYLOAD_COUNT = (int)(sizeof(PAYLOADS) / sizeof(PAYLOADS[0]));
