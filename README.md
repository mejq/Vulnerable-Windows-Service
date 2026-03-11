# Vulnerable Windows Service: EuroSky_InventorySync

This repository contains a deliberately vulnerable Windows service (`EuroSky_InventorySync.cpp`) and a Proof of Concept (PoC) malicious DLL (`inventory_helper_ext.cpp`) designed to demonstrate **DLL Hijacking (DLL Search Order Hijacking)** and subsequent **Privilege Escalation**.

This project is built for penetration testing, red team operations, and simulating privilege escalation scenarios within intentionally vulnerable laboratory environments.

## Vulnerability Details

Windows services typically run with high system privileges, often as `NT AUTHORITY\SYSTEM`. When the `EuroSky_InventorySync` service starts, it calls the `InitializeSyncPlugin` function, which attempts to load an external plugin:

```cpp
const char* pluginPath = "inventory_helper_ext.dll";
HINSTANCE hPlugin = LoadLibraryA(pluginPath);

```

The core vulnerability lies in the fact that `LoadLibraryA` is called **without an absolute path**. This omission forces Windows to rely on its standard [DLL Search Order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order). If an attacker can place a malicious `inventory_helper_ext.dll` in the directory where the service is executed from—or in any writable directory listed in the system's PATH environment variable—the service will load and execute the attacker's DLL with `SYSTEM` privileges.

## The Payload

The `inventory_helper_ext.cpp` file acts as the attacker's simulated malicious payload. Once the service loads the DLL, it searches for an exported function named `StartSync` and executes it.

When triggered, the `StartSync` function calls `FileSync()`, which leverages the `NetLocalGroupAddMembers` Windows API to add a standard user named **"bob"** to the local **"Administrators"** group. This is a classic TTP used to achieve persistence and full system compromise after a successful privilege escalation.

## Customizing the Target User
By default, the payload is hardcoded to escalate the privileges of a user named bob. If you want to use a different username in your specific lab environment, simply open inventory_helper_ext.cpp and modify the following line before compiling:

```bash
// Change "bob" to your target username
LPCWSTR targetUser = L"bob";
```

## Compilation & Usage

### 1. Compilation

You can compile these files using MSVC (`cl.exe`) or MinGW (`g++`).

**To compile the vulnerable service (MinGW):**

```bash
g++ EuroSky_InventorySync.cpp -o EuroSky_InventorySync.exe

```

**To compile the malicious DLL (MinGW):**

```bash
g++ -shared -o inventory_helper_ext.dll inventory_helper_ext.cpp -lnetapi32

```

*(Note: Because the DLL code utilizes the `NetLocalGroupAddMembers` API, you must link against `netapi32.lib` during compilation.)*

### 2. Setting Up the Lab Environment

1. Copy the compiled `EuroSky_InventorySync.exe` to a target directory (e.g., `C:\EuroSky\`).
2. Register the executable as a Windows service:
```cmd
sc create EuroSky_InventorySync binPath= "C:\EuroSky\EuroSky_InventorySync.exe" start= demand

```


3. Create a standard user named `bob` on the target system (if the user does not already exist):
```cmd
net user bob Password123! /add

```



### 3. Exploitation Steps

1. As the attacker, copy the compiled malicious `inventory_helper_ext.dll` into the service's working directory (`C:\EuroSky\`) or another vulnerable path.
2. Start the service:
```cmd
sc start EuroSky_InventorySync

```


3. Verify the successful exploitation by checking the group memberships of the user `bob`:
```cmd
net user bob

```


*You should now see `bob` listed as a member of the `Administrators` group.*

## Clean Up

To restore your lab environment to its original state, run the following commands:

```cmd
sc stop EuroSky_InventorySync
sc delete EuroSky_InventorySync
net localgroup Administrators bob /delete

```

## Disclaimer

This project is created strictly for **educational purposes and security research**. Only use these techniques on systems where you have explicit permission to test and develop defensive mechanisms.
