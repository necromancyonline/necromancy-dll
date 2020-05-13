#include <windows.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>
#include <codecvt>

#include "memory.h"
#include "util.h"
#include "necromancy.h"

#include "ini.h"

HMODULE base;
DWORD base_addr;

bool debug;
bool recv_area_op_code;

void dummy() {
	MessageBox(NULL, L"dummy()", L"necromancy.dll", NULL);
}

void write_file_debug(BYTE* bytes, DWORD len) {
	bytes -= 2;
	std::wstring w_str = std::wstring((wchar_t*)bytes, len);
	std::string str = ws_2_s(w_str);
	//  \n - already provided in log
	fprintf(stdout, "debug: %s", str.c_str());
}

void area_op(DWORD op) {
	fprintf(stdout, "recv_area_op: %04X \n", (WORD)op);
}

__declspec(naked) void hook_area_op() {
	__asm {
		mov eax, dword ptr ss : [esp + 8]
		mov ecx, dword ptr ds : [eax]
		lea edx, dword ptr ds : [ecx + 2]
		cmp edx, dword ptr ds : [eax + 4]
		jbe _jump_location
		xor al, al
		ret
		_jump_location :
		mov cx, word ptr ds : [ecx]
			// hook ecx = cx = area_recv_op_code
			pushfd
			pushad
			push ecx
			call area_op
			add esp, 4
			popad
			popfd
			// end hook
			mov edx, dword ptr ss : [esp + 4]
			mov word ptr ds : [edx] , cx
			add dword ptr ds : [eax] , 2
			mov al, 1
			ret
	}
}

__declspec(naked) void hook_write_file_debug() {
	__asm {
		mov eax, dword ptr ss : [esp + 4]
		test eax, eax
		jne _jump_location_a
		ret
		_jump_location_a :
		mov ecx, dword ptr ss : [esp + 8]
			test ecx, ecx
			jne _jump_location_b
			lea edx, dword ptr ds : [eax + 2]
			_jump_location_c :
			mov cx, word ptr ds : [eax]
			add eax, 2
			test cx, cx
			jne _jump_location_c
			sub eax, edx
			sar eax, 1
			// hook edx = byte* message eax = int length
			pushfd
			pushad
			push eax
			push edx
			call write_file_debug
			pop eax
			add esp, 4
			popad
			popfd
			// end hook
			ret
			_jump_location_b :
		cmp eax, ecx
			mov edx, eax
			jae _jump_location_d
			lea ebx, dword ptr ds : [ebx]
			_jump_location_e :
			cmp word ptr ds : [eax] , 0
			je _jump_location_d
			add eax, 2
			cmp eax, ecx
			jb _jump_location_e
			_jump_location_d :
		sub eax, edx
			sar eax, 1
			ret
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		// find image base offsets
		base = GetModuleHandle(L"WizardryOnline_hooked.exe");
		if (base == NULL) {
			fprintf(stderr, "Module [WizardryOnline_hooked.exe] not found \n");
			return -1;
		}
		base_addr = (DWORD)base;
		fprintf(stdout, "base: %p \n", base);
		fprintf(stdout, "base_addr: %u \n", base_addr);

		// set default values
		debug = true;
		recv_area_op_code = true;

		// load ini
		CSimpleIniA ini;
		ini.SetUnicode();
		if (ini.LoadFile("necromancy.ini") < 0) {
			// create default ini
			ini.SetBoolValue("necromancy", "debug", debug);
			ini.SetBoolValue("necromancy", "recv_area_op_code", recv_area_op_code);
			if (ini.SaveFile("necromancy.ini") < 0) {
				// ini create error
			}
		}

		debug = ini.GetBoolValue("necromancy", "debug", debug);
		recv_area_op_code = ini.GetBoolValue("necromancy", "recv_area_op_code", recv_area_op_code);

		// open console
		if (TRUE == AllocConsole())
		{
			FILE* nfp[3];
			freopen_s(nfp + 0, "CONOUT$", "rb", stdin);
			freopen_s(nfp + 1, "CONOUT$", "wb", stdout);
			freopen_s(nfp + 2, "CONOUT$", "wb", stderr);
			std::ios::sync_with_stdio();
		}
		
		// print current settings
		fprintf(stdout, "debug: %s \n", debug ? "true" : "false");
		fprintf(stdout, "recv_area_op_code: %s \n", recv_area_op_code ? "true" : "false");
		
		if (debug) {
			// Enable Debug (Shows FPS + Prints Debug String)
			WriteMemory((LPVOID)(base_addr + 0x14D42), "\x3C\x01\x90\x90\x90\x90", 6);
			hook_fn(base_addr, 0x8E606, hook_write_file_debug);
		}
		if (recv_area_op_code) {
			hook_fn(base_addr, 0xA5B4F, hook_area_op);
		}
		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

