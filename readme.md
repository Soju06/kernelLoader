# KernelLoader

KernelLoader is a simple tool to load and debug unsigned kernel.

## Usage

```bash
Usage:
  -l, --load [driver path] [driver name]
	Load the driver.
		
	Options:
		-o, --overwrite
			If the driver already exists, exit and overwrite it.
		-w, --watch
			Quickly reload drivers from a prompt when needed.

  -u, --unload [driver name]
	Delete the driver.

Global Options:
  -s, --status [driver name]
	Print the drivers service status.
  -i, --ignore-signatures
  	Install services to ignore kernel driver signatures. (powered by Wind64)

  -r, --uninstall-ignore-signatures
	Remove services that ignore signatures of kernel drivers. (powered by Wind64)
```

## Examples

#### Watch Mode

Here is an example of loading `../EasyShield.sys` with Watch mode and signature ignore.

If signature ignore is enabled and an anti-cheat is running during Watch mode, a flag may occur. Disable the anti-cheat to debug the driver.

**To use this feature, the driver must support the `SERVICE_ACCEPT_STOP` control.**

To support the `SERVICE_ACCEPT_STOP` control, refer to [here](#how-to-support-service_accept_stop-control).


```bash
kernelLoader.exe -l ../EasyShield.sys EasyShield2 -w -i
```

```bash
[+] EasyShield2 driver loaded successfully.

[*] Press the 'R' key to reload the driver.
    Press the 'S' key to print the service status.
    Press the 'Q' key to exit the loop.
    Press the 'X' key to delete the driver and exit the loop.

21:59:07:
```

If you press the S key, you can check the service status as follows.

```bash
[+] EasyShield2 Service Status :
------------------------------------------------------
  Type                   :  SERVICE_KERNEL_DRIVER
  Status                 :  RUNNING
------------------------------------------------------
  Wait Hint              :  0ms
  Checkpoint             :  0
  Controls Accepted      :  SERVICE_ACCEPT_STOP
------------------------------------------------------
  Exit Code (Win32)      :  0x00000000
  Exit Code (Service)    :  0x00000000
------------------------------------------------------


[*] Press the 'R' key to reload the driver.
    Press the 'S' key to print the service status.
    Press the 'Q' key to exit the loop.
    Press the 'X' key to delete the driver and exit the loop.

21:59:45:
```

## How to support `SERVICE_ACCEPT_STOP` control

You can specify a callback function in `PDRIVER_OBJECT->DriverUnload`.

```c
VOID OnUnload(PDRIVER_OBJECT driverObject) {
	UNREFERENCED_PARAMETER(driverObject);
    DbgPrintEx(0, 0, "[*] Device unloaded.\n");
}

NTSTATUS DriverEntry(
	PDRIVER_OBJECT		driverObject,
	PUNICODE_STRING		registryPath
) {
	UNREFERENCED_PARAMETER(registryPath);
    DbgPrintEx(0, 0, "[*] Device loaded.\n");

    driverObject->DriverUnload = OnUnload;

	return STATUS_SUCCESS;
}
```