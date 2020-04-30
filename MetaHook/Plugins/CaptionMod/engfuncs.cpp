#include <metahook.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "engfuncs.h"

#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_8308_SIG "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"

#define S_INIT_SIG "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_STARTDYNAMICSOUND_SIG "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"
#define S_LOADSOUND_SIG "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

cap_funcs_t gCapFuncs;

//Error when can't find sig
void Sys_ErrorEx(const char *fmt, ...)
{
	va_list argptr;
	char msg[1024];

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if(gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd("escape\n");

	MessageBox(NULL, msg, "Error", MB_ICONERROR);
	exit(0);
}

void Engine_FillAddress(void)
{
	memset(&gCapFuncs, 0, sizeof(gCapFuncs));

	if(g_dwEngineBuildnum >= 5953)
	{
		gCapFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG_NEW);
		Sig_FuncNotFound(S_Init);

		gCapFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(S_Init, S_FINDNAME_SIG_NEW);
		Sig_FuncNotFound(S_FindName);

		gCapFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_FindName, S_STARTDYNAMICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartDynamicSound);

		gCapFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartStaticSound);

		gCapFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern_From(S_StartStaticSound, S_LOADSOUND_SIG_NEW);
		if(!gCapFuncs.S_LoadSound)
			gCapFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern_From(S_StartStaticSound, S_LOADSOUND_8308_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}
	else
	{
		gCapFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG);
		Sig_FuncNotFound(S_Init);

		gCapFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(S_Init, S_FINDNAME_SIG);
		Sig_FuncNotFound(S_FindName);

		gCapFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_FindName, S_STARTDYNAMICSOUND_SIG);
		Sig_FuncNotFound(S_StartDynamicSound);

		gCapFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG);
		Sig_FuncNotFound(S_StartStaticSound);

		gCapFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern_From(S_StartStaticSound, S_LOADSOUND_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}
}

void Engine_InstallHook(void)
{
	InstallHook(S_FindName);
	InstallHook(S_StartDynamicSound);
	InstallHook(S_StartStaticSound);

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_pMetaSave->pEngineFuncs->GetClientTime, 0x20, "\xDD\x05", Sig_Length("\xDD\x05"));
	if(!addr)
		Sig_NotFound("cl_time");
	gCapFuncs.pcl_time = (double *)*(DWORD *)(addr + 2);
	gCapFuncs.pcl_oldtime = gCapFuncs.pcl_time + 1;
}
