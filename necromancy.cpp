#include <windows.h>
#include <sstream>
#include <fstream>
#include <iostream>

#include "memory.h"
#include "util.h"
#include "necromancy.h"

HMODULE base;
DWORD base_addr;

void dummy() {
	MessageBox(NULL, L"dummy()", L"necromancy.dll", NULL);
}

void area_op(DWORD op) {
	fprintf(stdout, "area_op: %04X \n", (WORD)op);
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
			pushfd
			pushad
			push ecx
			call area_op
			add esp, 4
			popad
			popfd
			mov edx, dword ptr ss : [esp + 4]
			mov word ptr ds : [edx] , cx
			add dword ptr ds : [eax] , 2
			mov al, 1
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
		if (TRUE == AllocConsole())
		{
			FILE* nfp[3];
			freopen_s(nfp + 0, "CONOUT$", "rb", stdin);
			freopen_s(nfp + 1, "CONOUT$", "wb", stdout);
			freopen_s(nfp + 2, "CONOUT$", "wb", stderr);
			std::ios::sync_with_stdio();
		}

		base = GetModuleHandle(L"WizardryOnline_hooked.exe");
		if (base == NULL) {
			fprintf(stderr, "Module [WizardryOnline_hooked.exe] not found \n");
			return -1;
		}
		base_addr = (DWORD)base;
		fprintf(stdout, "base: %p \n", base);
		fprintf(stdout, "base_addr: %u \n", base_addr);

		// Hooks
		hook_fn(base_addr, 0xA5B4F, hook_area_op);

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

