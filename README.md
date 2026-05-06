# EV-Fingerprint Web Controller

Sistem kontrol akses berbasis sidik jari dengan antarmuka web untuk ESP32-C3. Mendukung enroll, hapus, dan kontrol relay.

## 💰 DONASI

Jika kode ini membantu mungkin sedikit donasi akan sangat saya hargai

[![Donate](https://cdn-icons-png.flaticon.com/256/5002/5002279.png)](https://saweria.co/yudhaisme)

## Fitur Utama

- ✅ Enroll & hapus sidik jari via web
- ✅ Kontrol relay via web atau sentuhan sensor
- ✅ Manajemen nama pengguna per ID
- ✅ WiFi dapat dihidupkan/matikan (default mati)
- ✅ Pengaturan WiFi (SSID/password) via web
- ✅ Tombol fisik untuk toggle WiFi (tekan 3 detik)
- ✅ Sleep mode hemat daya (LED fingerprint mati)
- ✅ Buzzer feedback untuk setiap aksi

## Hardware Requirements

| Komponen | Pin | Keterangan |
|----------|-----|-------------|
| ESP32-C3 | - | Microcontroller utama |
| Fingerprint Sensor (HLK ZW-101) | RX=20, TX=21, VCC=3.3V/5V | Modul sidik jari |
| Relay Module | GPIO 3 | Kontrol output |
| Buzzer | GPIO 4 | Feedback suara |
| Touch Sensor / Button | GPIO 7 | Trigger scan fingerprint |
| Toggle Button | GPIO 5 (to GND) | Toggle WiFi ON/OFF (tahan 3 detik) |

## Other Requirements

- Stepdown 90v ke 5v untuk menghidupkan ESP32
- Solid State Relay SSR DD pastikan DD (DC-DC) bukan DA (DC-AC)
- Saklar
- Kabel, Solder, Timah dll

## Software Requirements

- Arduino IDE (atau PlatformIO)
- Library yang diperlukan:
  - `Adafruit_Fingerprint`
  - `WiFi`
  - `WebServer`
  - `Preferences`
 
## Struktur File
```
📁 EV-Fingerprint-Web-Controller/
├── 📄 EV_fingerprint_web.ino    # Kode utama
├── 📄 config.h                   # Konfigurasi pin & parameter
├── 📄 webdisplay.h               # Header halaman web
└── 📄 webdisplay.cpp             # Kode HTML & CSS (PROGMEM)
```

## Wiring Diagram

| ESP32-C3 Pin | Koneksi ke Komponen | Keterangan |
|--------------|---------------------|-------------|
| GPIO 3 | Relay Module (IN) | Kontrol relay (HIGH = ON) |
| GPIO 4 | Buzzer (+) | Feedback suara, GND ke ground |
| GPIO 5 | Push Button (1 kaki ke GND) | Toggle WiFi, internal pull-up |
| GPIO 7 | Touch Sensor / Push Button (ke GND) | Trigger scan fingerprint |
| GPIO 20 (UART RX) | Fingerprint Sensor (TX) | Komunikasi data dari sensor |
| GPIO 21 (UART TX) | Fingerprint Sensor (RX) | Komunikasi data ke sensor |
| 3.3V / 5V | Fingerprint Sensor (VCC) | Sesuai spesifikasi sensor |
| GND | Fingerprint Sensor (GND) | Ground bersama |
| GND | Buzzer (-) | Ground |
| GND | Push Button (kaki lain) | Ground |

### HLK ZW101 Fingerprint Sensor - Wiring Table (6 Pin)

| Pin ZW101 | Fungsi | Koneksi ke ESP32-C3 | Keterangan |
|-----------|--------|---------------------|-------------|
| V_Touch | Kontrol power sensor | 3.3v | |
| TOUCH_OUT | Deteksi sentuhan jari | GPIO 7 | bisa untuk tombol atau untuk wake jika sleep |
| VCC | Power 3.3V | 3.3V | Jangan pakai 5V, bisa rusak |
| TX | Kirim data ke ESP32 | GPIO 20 (RX) | Pin RX dari ESP32 |
| RX | Terima data dari ESP32 | GPIO 21 (TX) | Pin TX dari ESP32 |
| GND | Ground | GND | Meskipun kabel berwarna merah fungsinya adalah untuk gnd jangan beri tegangan |



## Konfigurasi

Buka file `config.h` dan sesuaikan:

```cpp
#define DEFAULT_WIFI_SSID "EVFINGERPRINT"
#define DEFAULT_WIFI_PASS "YNTKTS123"

// Nilai tinggi = lebih akurat tapi bisa reject jari yang benar
#define CONFIDENCE_THRESHOLD 40

// true = sensor ikut sleep (hemat daya), false = sensor tetap menyala
#define ENABLE_SENSOR_SLEEP false
```

## Cara Pasang ke Kendaraan 

- Kunci Kontak pada dasarnya terhubung ke batre dan controller untuk menyalurkan +72v ke Controller bisanya berwarna kuning dan merah
- Ambil +72v pasangkan ke positif Step Down/Reducer (Kabel Warna merah ada sekring untuk membedakan)
- Untuk -72v ke reducer cari dari gnd atau jika di kendaraan Polytron Fox R/S ambil dari modul charger USB (warna kabel Hijau)
- Output +5v Pasangkan ke pin 5v dan -5v ke gnd pada esp32
- Pasangkan gnd ke input (-) SSR lalu GPIO3 ke (+) SSR
- Cabangkan +72v ke (+) output SSR
- Untuk output (-) SSR pasangkan ke kabel menuju controller (biasanya kabel kuning dari kabel stop kontak)
- Output (+) dan (-) SSR boleh dicabang ke saklar kunci kontak jika masih ingin menggunakan kunci kontak fisik

## Cara Penggunaan

### 1. Menyalakan/Mematikan WiFi

| Aksi | Hasil |
|------|-------|
| Setelah booting / restart | WiFi dalam keadaan **MATI** (default) |
| Tekan tombol GPIO 5 ke GND selama **3 detik** | WiFi akan **MENYALA** (beep panjang 1 detik) |
| Tekan tombol GPIO 5 ke GND selama **3 detik** lagi | WiFi akan **MATI** (beep panjang 1 detik) |
| Klik tombol "Matikan Wi-Fi" di halaman web | WiFi mati (tanpa restart) |

**Feedback beep saat menekan tombol 3 detik:**
- Detik ke-1 → beep pendek (80ms)
- Detik ke-2 → beep pendek (80ms)
- Detik ke-3 → beep panjang (1000ms) → WiFi toggle

### 2. Mengganti SSID/Password WiFi

1. Pastikan WiFi dalam keadaan **MENYALA**
2. Buka browser, akses alamat **192.168.4.1**
3. Masuk ke menu **"Setting WiFi"** (tombol orange)
4. Masukkan SSID dan password baru
5. Klik tombol **"Simpan dan Restart"**
6. ESP32 akan restart, WiFi akan **tetap menyala** dengan SSID baru

> **Catatan:** Password WiFi minimal 8 karakter. Hindari spasi pada SSID.

### 3. Menambahkan Sidik Jari Baru (Enroll)

1. Pastikan WiFi **MENYALA** (jika ingin akses web)
2. Buka **192.168.4.1** (jika WiFi menyala)
3. Klik tombol **"Tambah"**
4. Klik tombol **"Mulai"** (akan terdengar beep 4x)
5. Tempelkan jari ke sensor
6. Lepas jari saat diminta
7. Tempelkan ulang jari yang sama
8. Jika berhasil → beep pendek 2x

**Tanpa web (offline enroll):**
- Fitur enroll hanya tersedia via web untuk keamanan

### 4. Menghapus Sidik Jari

1. Buka **192.168.4.1**
2. Masuk ke menu **"Kelola"**
3. Cari ID yang ingin dihapus
4. Klik **"Hapus"** untuk satu ID, atau **"Hapus Semua ID"** untuk membersihkan semua
5. Data di sensor dan nama akan terhapus permanen

### 5. Mengganti Nama Sidik Jari

1. Buka menu **"Kelola"**
2. Klik tombol **"Rename"** pada ID yang diinginkan
3. Masukkan nama baru
4. Nama akan tersimpan di memori ESP32

### 6. Mengontrol Relay

**Via Web:**
- Toggle switch (ON/OFF) tersedia di **header semua halaman** (sebelah kanan judul)
- Status relay akan ditampilkan di bawah toggle

**Via Fingerprint:**
- Sentuh sensor (GPIO 7) dengan jari yang sudah terdaftar
- Jika cocok → relay akan berganti status (ON↔OFF) dan beep 150ms
- Jika tidak cocok → beep pendek 3x

### 7. Mode Sleep

- Setelah **10 detik** tidak ada aktivitas:
  - LED fingerprint mati (jika ENABLE_SENSOR_SLEEP = true)
  - Modul tetap bisa discan (sentuh sensor akan membangunkan)
- Untuk menghemat daya lebih, set `ENABLE_SENSOR_SLEEP = true` di config.h

### 8. Indikator LED Fingerprint

| Warna | Arti |
|-------|------|
| Biru (breathing) | Sensor standby / sleep |
| Biru (kedip cepat) | Sedang mengambil gambar sidik jari |
| Hijau (kedip) | Fingerprint cocok |
| Merah (kedip) | Fingerprint tidak cocok |
| Mati | Sensor dimatikan (mode sleep) |

### 9. Reset ke Pengaturan Awal

| Metode | Cara | Hasil |
|--------|------|-------|
| Reset WiFi ke default | Upload ulang firmware (kode baru) | Kembali ke SSID "EVFINGERPRINT" |
| Hapus semua fingerprint | Gunakan tombol "Hapus Semua ID" di web | Semua data fingerprint terhapus |

> **Catatan:** Tidak ada tombol fisik untuk reset WiFi demi keamanan. Jika lupa password WiFi, upload ulang firmware.

### 10. Troubleshooting Cepat

| Masalah | Solusi |
|---------|--------|
| Tidak bisa connect ke WiFi | Pastikan WiFi menyala (tekan tombol 3 detik, cek ada beep panjang) |
| Gagal enroll | Bersihkan sensor, posisikan jari dengan benar, coba lagi |
| Sensor tidak merespon | Cabut power sensor, pasang kembali, restart ESP32 |
| Web tidak terbuka | Pastikan perangkat terhubung ke WiFi ESP32 (bukan ke router lain) |
| Fingerprint selalu ditolak | Turunkan CONFIDENCE_THRESHOLD di config.h (misal ke 30) |
| Relay tidak bekerja | Periksa wiring, coba balik logika (HIGH/LOW) di kode |

### 11. Akses Web

| Metode | Alamat | Keterangan |
|--------|--------|-------------|
| WiFi menyala | `192.168.4.1` | Akses via browser setelah connect ke SSID ESP32 |
| Default SSID | `EVFINGERPRINT` | Setelah upload pertama atau reset |
| Default Password | `YNTKTS123` | Minimal 8 karakter |

### 12. Keamanan

- Fingerprint disimpan di **sensor** (bukan di ESP32)
- Nama pengguna disimpan di **Preferences ESP32** (terenkripsi secara internal)
- WiFi password tidak ditampilkan ke web (hanya bisa diganti, tidak bisa dilihat)
- Tombol reset WiFi **tidak ada** (cegah akses tidak sah via fisik)

## Lisensi

MIT License

## Kontributor

Proyek ini dikembangkan untuk keperluan otomatisasi akses berbasis sidik jari.
