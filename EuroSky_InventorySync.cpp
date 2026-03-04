#include <windows.h>
#include <tchar.h>
#include <stdio.h>

// Service name definition
TCHAR SERVICE_NAME[] = _T("EuroSky_InventorySync");

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;


// Func Prototypes
VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler (DWORD);
DWORD WINAPI ServiceWorkerThread (LPVOID lpParam);

// func signature for the function we will look for in the DLL
typedef void (*StartSyncFunc)();

// Vulnerable function that attempts to load a DLL without specifying a full path.
void InitializeSyncPlugin() {

    const char* pluginPath = "inventory_helper_ext.dll";
    HINSTANCE hPlugin = LoadLibraryA(pluginPath);

    if (hPlugin != NULL) {
        // DLL loaded successfully, now we search "StartSync" func in it.
        StartSyncFunc pInitPlugin = (StartSyncFunc) GetProcAddress(hPlugin, "StartSync");
        if (pInitPlugin != NULL) {
            pInitPlugin(); // Call the function to start the sync process.
        } else {
            OutputDebugString(_T("EuroSky_InventorySync: StartSync function not found in DLL."));
        }
        FreeLibrary(hPlugin); // Unload the DLL after use.
    } 
    
    else {
        OutputDebugString(_T("EuroSky_InventorySync: Failed to load plugin: inventory_helper_ext.dll. Proceeding with default sync."));
        // If the DLL fails to load, we can proceed with a default sync process or log the error.
        // For demonstration, we will just log the error and not perform any sync.
    }
}


int _tmain (int argc, TCHAR *argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher (ServiceTable) == FALSE) {
        return GetLastError ();
    }
    return 0;
}


VOID WINAPI ServiceMain (DWORD argc, LPTSTR *argv) {
    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandler (SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL) {
        return;
    }

    ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("EuroSky_InventorySync: SetServiceStatus returned error"));
    }


    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;
        SetServiceStatus (g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("EuroSky_InventorySync: SetServiceStatus returned error"));
    }

    HANDLE hThread = CreateThread (NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    WaitForSingleObject (g_ServiceStopEvent, INFINITE);

    CloseHandle (g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(_T("EuroSky_InventorySync: SetServiceStatus returned error"));
    }
}

VOID WINAPI ServiceCtrlHandler (DWORD CtrlCode) {
    switch (CtrlCode) {
        case SERVICE_CONTROL_STOP:
            if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
                break;

            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwWin32ExitCode = 0;
            g_ServiceStatus.dwCheckPoint = 4;

            if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) {
                OutputDebugString(_T("EuroSky_InventorySync: SetServiceStatus returned error"));
            }

            SetEvent (g_ServiceStopEvent);
            break;

        default:
            break;
    }
}

DWORD WINAPI ServiceWorkerThread (LPVOID lpParam) {
    InitializeSyncPlugin();

    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        Sleep(3600000); // Sleep for 1 hour (3600000 milliseconds)
    }
    return ERROR_SUCCESS;
}