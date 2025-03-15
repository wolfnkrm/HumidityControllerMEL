# HumidityControllerMEL
ESP32 code for humidity controller setup in MEL ITB

--ID--
Sebelum menggunakan kode ini untuk humidity controller lab, pastikan anda sudah memiliki prasyarat berikut:
1. Sudah mendownload dan instal Arduino IDE terbaru (arduino 2.X.X). Jika belum, anda dapat menonton video ini https://youtu.be/D0A_-cUyghA
2. Sudah menginstal library yang diperlukan (liquidcrystal I2C by Frank de  Brabander & DHT sensor library by Adafruit)
3. Setup preferensi esp32 di arduino IDE melalui **file-> Preferences-> Settings-> Pada kolom "Additional boards manager" paste-kan link berikut** https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json -> Klik OK

Cara set humidity controller:
1. Pastikan semua power dari **steker OFF**
2. Buka kode Humidity_controller_3 di arduino IDE
3. Pada menu **"Select Board"** di bagian atas IDE, pastikan terbaca **ESP32 Dev Module** dan port yang terbaca sudah benar
4. Pada line 36 atau bagian targetHumidity, ubah angka ke nilai humiditas yang diinginkan
5. Klik verify atau lambang centang pada bagian pojok kiri atas dari IDE
6. Pasang kabel usb type-C ke kontroler esp32 (bagian atas) dan ke laptop
7. Klik upload atau lambang panah kanan di sebelah tombol verify pada bagian pojok kiri atas dari IDE
8. Tunggu hingga proses selesai dan cek pada layar lcd perubahan nilai **Target**
9. **Cabut** kabel usb dari laptop
10. Nyalakan power dari steker dinding 


