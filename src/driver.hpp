#pragma once
#include <format>
#include <vector>

#include "fs.hpp"

inline bool SCManager(SC_HANDLE* handle) {
	*handle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (handle) return 1;
	cout << "[x] Can't open SC manager. (permissions issue)\n";
	return 0;
}

inline bool OpenDriver(
	wstring serviceName,
	DWORD access,
	SC_HANDLE* manager,
	SC_HANDLE* driver,
	int ignoreLevel
) {
	auto _manager = manager;

	if (!manager || !SCManager(manager))
		return 0;

	*driver = OpenService(
		*manager,
		serviceName.c_str(),
		access
	);

	if (!*driver) {
		if (ignoreLevel < 1)
			cout << "[x] Cannot open service.\n";
		if (!_manager) 
			CloseServiceHandle(*manager);
		return 0;
	}
	return 1;
}

inline bool GetStatus(
	wstring serviceName,
	SERVICE_STATUS* status,
	SC_HANDLE* manager = 0
) {
	bool lv = 0;
	SC_HANDLE handle, service = 0;

	if (manager)
		handle = *manager;

	if (!OpenDriver(
		serviceName,
		SERVICE_QUERY_STATUS,
		&handle,
		&service,
		0
	)) return 0;

	if (!QueryServiceStatus(service, status))
		cout << "[x] Unable to get status of service.\n";
	else lv = 1;

	if (service)
		CloseServiceHandle(service);

	if (!manager)
		CloseServiceHandle(handle);

	return lv;
}

inline bool DeleteDriver(
	wstring serviceName,
	SC_HANDLE* manager = 0,
	int ignoreLevel = 0
) {
	bool lv = 0;
	SC_HANDLE handle, service = 0;

	if (manager)
		handle = *manager;

	if (!OpenDriver(
		serviceName,
		SERVICE_STOP | DELETE,
		&handle,
		&service,
		ignoreLevel
	)) return 0;

	if (!service) {
		if (ignoreLevel < 1) 
			cout << "[x] Cannot open service.\n";
		goto CLOSE;
	}
	
	SERVICE_STATUS status;
	if (
		ControlService(service, SERVICE_CONTROL_STOP, &status) ||
		status.dwCurrentState == SERVICE_STOPPED
	)
		if (!DeleteService(service) && ignoreLevel < 3)
			cout << "[x] Cannot delete service.\n";
		else lv = 1;

	else if (ignoreLevel < 2)
		cout << "[x] Service cannot be stopped (Probably the driver unload method is not defined).\n";

CLOSE:
	if (service)
		CloseServiceHandle(service);

	if (!manager) 
		CloseServiceHandle(handle);

	return lv;
}

inline bool LoadDriver(
	wstring driverPath,
	wstring serviceName,
	wstring displayName,
	SC_HANDLE* manager = 0,
	bool overwrite = 0
) {
	bool lv = 0;
	auto _manager = manager;
	SC_HANDLE handle;

	if (manager)
		handle = *manager;
	else if (!SCManager(&handle))
		return 0;
	
CREATE_SERVICE:
	SC_HANDLE service = CreateService(
		handle,
		serviceName.c_str(),
		displayName.c_str(),
		SERVICE_START | DELETE | SERVICE_STOP,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		driverPath.c_str(),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!service) {
		if (overwrite) {
			if (DeleteDriver(
				serviceName.c_str(),
				&handle
			)) goto CREATE_SERVICE;
		} else
			cout << "[x] Unable to create service. (there is already a registered service)\n";
		goto CLOSE;
	}

	if (!StartServiceA(service, NULL, NULL))
		cout << "[x] Service cannot be run.\n";
	else lv = 1;

CLOSE:
	if (service)
		CloseServiceHandle(service);
	if (!_manager)
		CloseServiceHandle(handle);

	return lv;
}

#define FLAG_STR(list, value, flag) \
	if ((value & flag) == flag)		\
	list.push_back(#flag);

inline bool PrintStatus(
	wstring serviceName,
	SC_HANDLE* manager = 0
) {
	SERVICE_STATUS status;
	
	if (!GetStatus(serviceName, &status, manager))
		return 0;

	vector<string> types, ctrlacp;
	FLAG_STR(types, status.dwServiceType, SERVICE_FILE_SYSTEM_DRIVER);
	FLAG_STR(types, status.dwServiceType, SERVICE_KERNEL_DRIVER);
	FLAG_STR(types, status.dwServiceType, SERVICE_WIN32_OWN_PROCESS);
	FLAG_STR(types, status.dwServiceType, SERVICE_WIN32_SHARE_PROCESS);
	FLAG_STR(types, status.dwServiceType, SERVICE_USER_OWN_PROCESS);
	FLAG_STR(types, status.dwServiceType, SERVICE_USER_SHARE_PROCESS);
	FLAG_STR(types, status.dwServiceType, SERVICE_INTERACTIVE_PROCESS);

	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_NETBINDCHANGE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_PARAMCHANGE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_PAUSE_CONTINUE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_PRESHUTDOWN);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_SHUTDOWN);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_STOP);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_HARDWAREPROFILECHANGE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_POWEREVENT);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_SESSIONCHANGE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_TIMECHANGE);
	FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_TRIGGEREVENT);
	// FLAG_STR(ctrlacp, status.dwControlsAccepted, SERVICE_ACCEPT_USERMODEREBOOT);

	cout << format(
		"[+] {} Service Status :\n"
		"------------------------------------------------------\n"
		"  Type                   :  {}\n"
		"  Status                 :  {}\n"
		"------------------------------------------------------\n"
		"  Wait Hint              :  {}ms\n"
		"  Checkpoint             :  {}\n"
		"  Controls Accepted      :  {}\n"
		"------------------------------------------------------\n"
		"  Exit Code (Win32)      :  {:#010x}\n"
		"  Exit Code (Service)    :  {:#010x}\n"
		"------------------------------------------------------\n"
		"\n",
		path(serviceName).string(),
		strjoin(types, " | "),
		status.dwCurrentState == SERVICE_CONTINUE_PENDING	? "PENDING" :
		status.dwCurrentState == SERVICE_PAUSE_PENDING		? "PAUSE PENDING" :
		status.dwCurrentState == SERVICE_PAUSED				? "PAUSED" :
		status.dwCurrentState == SERVICE_RUNNING			? "RUNNING" :
		status.dwCurrentState == SERVICE_START_PENDING		? "START PENDING" :
		status.dwCurrentState == SERVICE_STOP_PENDING		? "STOP PENDING" :
		status.dwCurrentState == SERVICE_STOPPED			? "STOPPED" :
			to_string(status.dwCurrentState),
		status.dwWaitHint,
		status.dwCheckPoint,
		ctrlacp.empty() ?"NONE" : strjoin(ctrlacp, " | "),
		status.dwWin32ExitCode,
		status.dwServiceSpecificExitCode
	);

	return 1;
}

inline bool HookDebugLog() {
	
}