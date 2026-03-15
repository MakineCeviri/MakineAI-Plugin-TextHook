# MakineAI TextHook

> Oyun belleğinden metin çıkarma ve gömülü çeviri eklentisi

MakineAI Launcher için resmi metin hooking eklentisi. Oyunların bellek fonksiyonlarını yakalayarak metin çıkarır ve doğrudan oyun içine çeviri gömer.

## Özellikler

- **MinHook Inline Hooking** — Trampoline tabanlı fonksiyon yakalama (birincil yöntem)
- **VEH Breakpoint** — INT3 + tek adım istisna işleyici (64-bit yedek)
- **Bellek Okuma** — Bilinen adreslerden doğrudan okuma (polling)
- **Engine Handler'lar** — Unity, Unreal, RPGMaker, Ren'Py desteği
- **Gömülü Çeviri** — Çeviriyi doğrudan oyun belleğine yazar
- **Live Entegrasyonu** — TextHook metin çıkarır → Live çevirir ve gösterir

## Nasıl Çalışır

```
Oyun Süreci
    ↓ MinHook ile fonksiyon yakalama
Metin Fonksiyonu Çağrılır (TextOutW, DrawText, vb.)
    ↓ Hook tetiklenir
Orijinal Metin Yakalanır
    ↓ Live plugin'e iletilir (varsa)
Çevrilmiş Metin
    ↓ Gömülü çeviri (opsiyonel)
Oyun İçinde Türkçe Metin Görünür
```

## Desteklenen Oyun Motorları

| Motor | Yöntem | Durum |
|-------|--------|-------|
| Unity (Mono/.NET) | İnline hook | Planlanıyor |
| Unreal Engine | FText/FString hook | Planlanıyor |
| RPG Maker MV/MZ | JavaScript köprüsü | Planlanıyor |
| Ren'Py | Python string yakalama | Planlanıyor |
| Genel (Win32) | TextOutW, DrawTextW | Planlanıyor |

## Kurulum

### MakineAI Launcher Üzerinden (Önerilen)
1. MakineAI Launcher'ı açın
2. **Ayarlar → Eklentiler** sayfasına gidin
3. **MakineAI TextHook** yanındaki **"Kur"** butonuna tıklayın

### Manuel Kurulum
1. [Releases](https://github.com/MakineCeviri/MakineAI-Plugin-TextHook/releases) sayfasından `.makine` dosyasını indirin
2. `AppData/Local/MakineAI/plugins/texthook/` dizinine çıkartın
3. Launcher'ı yeniden başlatın

## Uyarılar

- **Yönetici yetkisi gerektirir** — Başka süreçlere erişim için UAC yükseltme gerekir
- **Anti-cheat uyumsuzluğu** — EAC, BattlEye, Vanguard kullanan oyunlarda çalışmaz
- **Antivirüs uyarısı** — Hooking davranışı antivirüs yazılımlarını tetikleyebilir

## Geliştirme

### Gereksinimler
- CMake 3.25+
- MinGW GCC 13.1+
- Ninja (opsiyonel)

### Derleme
```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

### Paketleme
```bash
pip install zstandard
python makine-pack.py ./build/release/
```

## Proje Yapısı

```
├── manifest.json              — Eklenti meta verileri
├── CMakeLists.txt             — Derleme yapılandırması
├── include/makineai/plugin/   — Plugin SDK başlık dosyaları
└── src/
    ├── plugin.cpp             — Giriş noktası (5 zorunlu export)
    └── hook_core.cpp          — Hook yönetimi ve koordinasyon
```

## Yol Haritası

- [x] Plugin iskeleti ve C ABI uyumluluğu
- [ ] MinHook entegrasyonu
- [ ] Genel Win32 text hook (TextOutW, DrawTextW)
- [ ] Unity Mono hook handler
- [ ] Unreal Engine hook handler
- [ ] RPG Maker MV/MZ handler
- [ ] Gömülü çeviri (embedded translation)
- [ ] Live plugin entegrasyonu
- [ ] Hook yapılandırma arayüzü

## Katkıda Bulunma

Katkılarınızı bekliyoruz! Özellikle yeni oyun motoru handler'ları için PR'lar memnuniyetle karşılanır.

## Lisans

GPL-3.0
