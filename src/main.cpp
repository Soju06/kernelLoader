#include <iostream>
#include <string>
#include <format>
#include <conio.h>
#include <ctime>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

#include "driver.hpp"

using namespace std;

constexpr auto	WIND64_URL	= L"https://github.com/katlogic/WindowsD/releases/download/v2.2/wind64.exe";
constexpr const char* HELP_STR = R"(
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
)";

int				Argc;
const char**	Argv;
path			RunningDir;

string NowTime() {
	tm t;
	stringstream buf;
	time_t now_t = time(0);
	localtime_s(&t, &now_t);
	buf << put_time(&t, "%H:%M:%S");
	return buf.str();
}

int FindArgs(const char* flag) {
	for (int i = 0; i < Argc; i++) {
		auto arg = Argv[i];
		if (strcmp(arg, flag) == 0)
			return i;
	}
	return -1;
}

const char* GetValue(int flagIndex) {
	if (flagIndex < 0 || flagIndex >= Argc) return 0;
	return Argv[flagIndex + 1];
}

bool UseWind64(const char* cmd = 0) {
	auto windP = RunningDir / path("wind64.exe");
	wstring windW = windP.wstring();

	if (!exists(windP)) {
		wcout << format(L"[*] Download Wind64 from {}\n", WIND64_URL);


		if (
			URLDownloadToFile(
				NULL,
				WIND64_URL,
				windW.c_str(),
				BINDF_GETNEWESTVERSION,
				NULL
			) != S_OK) {
			cout << "[*] Failed to download Wind64.\n";
			return 0;
		}
	}


	if (cmd) {
		auto status = system(format("{} {}", absolute(windP).string(), cmd).c_str());

		
		if (status != 1) {
			cout << format("[*] Failed to execute Wind64. (ExitCode: 0x{:016X})\n", status);
			return 0;
		}
		
		Sleep(1000);
	}

	return 1;
}

int main(int argc, const char** argv) {
	bool acok = 0;
	int argi = 0;
	const char *invparm = 0, *misparm = 0;
    Argc = argc; Argv = argv;
	RunningDir = curdir();

    cout << "[*] Kernel Loader\n";

	#define INVPARM(parm) invparm = #parm; goto SHOW_USAGE;
	#define MISPARM(parm) misparm = #parm; goto SHOW_USAGE;

	bool ignore_signatures =
		FindArgs("-i") != -1 ||
		FindArgs("--ignore-signatures") != -1;
	if (
		argc == 0				||
		FindArgs("-?") != -1	||
		FindArgs("-h") != -1
	) {
		SHOW_USAGE:
		if (invparm) cout << format("[-] Parameter {} is invalid.\n", invparm);
		if (misparm) cout << format("[-] Parameter {} is missing.\n", misparm);
		cout << HELP_STR;
		goto EXIT;
	}

	if (ignore_signatures) { 
		// wind64.exe /i
		acok = 1;
		cout << "[*] Install services to ignore kernel driver signatures.\n";
		if (!UseWind64("/i"))
			goto EXIT;
		cout << "[+] OK\n";
	} else if (
		// wind64.exe /u
		FindArgs("-r")								!= -1 ||
		FindArgs("--uninstall-ignore-signatures")	!= -1
	) {
		acok = 1;
		cout << "[*] Uninstall services that ignore signatures of kernel drivers.\n";
		if (!UseWind64("/u"))
			goto EXIT;
	} else if (
		// status
		(argi = FindArgs("-s")) != -1 ||
		(argi = FindArgs("--status")) != -1
	) {
		acok = 1;
		auto name = GetValue(argi);

		if (!name) {
			MISPARM(name);
		}

		PrintStatus(path(name));
	}

	if (
		// load
		(argi = FindArgs("-l"))		!= -1 ||
		(argi = FindArgs("--load")) != -1
	) {
		auto
			driverPath	=	GetValue(argi),
			name		=	GetValue(argi + 1);
		auto
			overwrite	=	FindArgs("-o")			!= -1 ||
							FindArgs("--overwrite") != -1,
			watch		=	FindArgs("-w")			!= -1 ||
							FindArgs("--watch")		!= -1;

		if (!driverPath) {
			MISPARM(driverPath);
		}
		if (!name) {
			MISPARM(name);
		}

		path 
			tmpdir,
			nameP			= name,
			driverP			= absolute(driverPath),
			driver			= driverP;

		if (!exists(driver) || is_directory(driver)) {
			cout << format("[x] Driver files not found! {}\n", driver.string());
			goto EXIT;
		}

		if (watch) {
			if (!tempdir(tmpdir, "kernelLoader_")) {
				cout << "[x] Unable to create temporary directory.\n";
				goto EXIT;
			}
			driver = tmpdir / path(driver.filename());
			overwrite = 1;
		}

		SC_HANDLE manager;

		if (!SCManager(&manager))
			goto EXIT;

		bool first = 1, remv = 0;
	LOAD__:
		if (watch) {
			if (!first && !DeleteDriver(nameP, &manager, 1)) {
				cout << "[x] Cannot delete driver.\n";
				goto LOAD_EXIT;
			}
			copy(
				driverP,
				driver,
				copy_options::overwrite_existing
			);
			system("cls");
			first = 0;
		}

		if (LoadDriver(
			driver,
			nameP,
			nameP,
			0,
			overwrite
		))
			cout << format("[+] {} driver loaded successfully.\n", name);
		else
			cout << format("[x] Failed to load driver {}\n", name);

		if (watch) {
		WATCH_H:
			cout << format(
				"\n"
				"[*] Press the 'R' key to reload the driver.\n"
				"    Press the 'S' key to print the service status.\n"
				"    Press the 'Q' key to exit the loop.\n"
				"    Press the 'X' key to delete the driver and exit the loop.\n"
				"\n{}: ",
				NowTime()
			);

			int ch = toupper(_getch());
			cout << "\n";
			
			if (ch == 'R')
				goto LOAD__;
			if (ch == 'S') {
				system("cls");
				PrintStatus(nameP, &manager);
				goto WATCH_H;
			}
			if (ch == 'Q') {
				cout << "[*] Exit!\n";
				goto LOAD_EXIT;
			}
			if (ch == 'X') {
				if (remv = DeleteDriver(
					path(name)
				)) 
					cout << "[+] Driver uninstalled successfully.\n";

				goto LOAD_EXIT;
			}

			system("cls");
			cout << "[-] Wrong command key\n";
			goto WATCH_H;
		}

	LOAD_EXIT:
		CloseServiceHandle(manager);
		
		if (
			watch					&&
			remv					&&
			exists(driver)			&&
			!is_directory(driver)
		)
			remove(driver);

		if (ignore_signatures) {
			cout << "[*] Uninstall services that ignore signatures of kernel drivers.\n";
			UseWind64("/u");
		}

		remove_all(tmpdir);
	} else if (
		(argi = FindArgs("-u"))			!= -1 ||
		(argi = FindArgs("--unload"))	!= -1
	) {
		// unload
		auto name = GetValue(argi);

		if (!name) {
			MISPARM(name);
		}

		if (DeleteDriver(
			path(name)
		))
			cout << "[+] Driver uninstalled successfully.\n";
	} else if (!acok)
		goto SHOW_USAGE;

EXIT:
	return 0;
}
