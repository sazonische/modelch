/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "subhook/subhook.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

MdlChagerExt g_MdlChagerExt;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_MdlChagerExt);


using namespace subhook;

IGameConfig *g_pGameConf;
Hook *g_HPlayerSpawn;
void *g_addr_model_normal;
void *g_addr_model_custom;
char g_model[256];
IForward *g_pSpawnForward = NULL;
void *g_addr_setstr;

inline uintptr_t GetVFT(void *obj, int offset)
{
    return *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(obj) + offset);
}

bool __cdecl PlayerSpawnHelper(CBaseEntity *pEntity, const char *&model_name, void *item, bool &arg4)
{
    //smutils->LogMessage(myself, "Model change for %d %p %i %i", gamehelpers->EntityToBCompatRef(pEntity), item, static_cast<char*>(item)[0x7C], arg4);

    cell_t result = Pl_Continue;
#ifdef WIN32
    using GetItemDefFn = void*(__thiscall*)(void*);
    bool custom = item && static_cast<char*>(item)[0x7C] && *(int*)(reinterpret_cast<uintptr_t>(reinterpret_cast<GetItemDefFn>(GetVFT(item, 48))(item)) + 616) == 38 && !arg4;
#else
    using GetItemDefFn = void*(__cdecl*)(void*);
    bool custom = item && static_cast<char*>(item)[0x7C] && *(int*)(reinterpret_cast<uintptr_t>(reinterpret_cast<GetItemDefFn>(GetVFT(item, 52))(item)) + 616) == 38 && !arg4;
#endif

    g_model[0] = '\0';
    char vo_prefix[64] = "";

    g_pSpawnForward->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
    g_pSpawnForward->PushCell(custom);
    g_pSpawnForward->PushStringEx(g_model, sizeof(g_model), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
    g_pSpawnForward->PushCell(sizeof(g_model));
    g_pSpawnForward->PushStringEx(vo_prefix, sizeof(vo_prefix), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
    g_pSpawnForward->PushCell(sizeof(vo_prefix));
    g_pSpawnForward->Execute(&result);

    //smutils->LogMessage(myself, "Fwd End: r %d, mdl \"%s\", prefix \"%s\"", result, g_model, vo_prefix);

    if (result == Pl_Changed)
    {
        model_name = g_model;
#ifdef WIN32
        reinterpret_cast<void(__thiscall*)(void*,const char*, int)>(g_addr_setstr)((void*)((uintptr_t)pEntity+15100), vo_prefix, strlen(vo_prefix)+1);
#else
        reinterpret_cast<int(*)(void*,const char*)>(g_addr_setstr)((void*)((uintptr_t)pEntity+15120), vo_prefix);
#endif
        return true;
    }

    arg4 = result == Pl_Continue ? !custom : true;
    return false;
}

__declspec(naked) void playerspawn()
{
#ifdef WIN32
    __asm lea eax, [esp+0x38-0x24]
    __asm push eax
    __asm push esi
    __asm lea eax, [esp+0x38-0x18+0x08] //we have pushed twice before
    __asm push eax
    __asm push ebx
#else
	__asm lea ecx, [ebp-0x38]
	__asm mov eax, edi
	__asm xor al, 1
	__asm or al, byte ptr [ecx]
	__asm mov byte ptr [ecx], al
    __asm push ecx
    __asm push esi
    __asm lea eax, [esp-0x04] //push address of argument itself as value of argument =)
    __asm push eax
    __asm mov eax, [ebp+0x08]
    __asm push eax
#endif
    __asm call PlayerSpawnHelper
    __asm add esp, 16

    __asm test al, al
    __asm jz UseDefaultMdl

#ifndef WIN32
	__asm mov edi, [esp-0x0C] //from value used for model_name argument getting actual address for model name
#endif
    __asm mov eax, g_addr_model_custom
    __asm jmp eax

    __asm UseDefaultMdl:
    __asm mov eax, g_addr_model_normal
#ifndef WIN32
	__asm test byte ptr [ebp-0x38], 0
#endif
    __asm jmp eax
}

bool MdlChagerExt::SDK_OnLoad(char* error, size_t maxlen, bool late)
{
    if (!gameconfs->LoadGameConfigFile("modelch.games", &g_pGameConf, error, maxlen))
        return false;

    void* addr;
    if (!g_pGameConf->GetMemSig("PlayerSpawn", &addr) || !addr)
    {
        snprintf(error, maxlen, "Failed to lookup PlayerSpawn signature.");
        return false;
    }

    if (!g_pGameConf->GetMemSig("StrAssign", &g_addr_setstr) || !g_addr_setstr)
    {
        snprintf(error, maxlen, "Failed to lookup StrAssign signature.");
        return false;
    }

    gameconfs->CloseGameConfigFile(g_pGameConf);

#ifdef WIN32
    void *addr_hook = (void*)((uintptr_t)addr + 0x7D0);
    g_addr_model_normal = (void*)((uintptr_t)addr + 0x7F6);
    g_addr_model_custom = (void*)((uintptr_t)addr + 0x879);
#else
    void *addr_hook = (void*)((uintptr_t)addr + 0xBB4);
    g_addr_model_normal = (void*)((uintptr_t)addr + 0xBF4);
    g_addr_model_custom = (void*)((uintptr_t)addr + 0xCF1);
#endif

    g_HPlayerSpawn = new Hook(addr_hook, (void*)playerspawn);
    g_HPlayerSpawn->Install();

    g_pSpawnForward = forwards->CreateForward("MdlCh_PlayerSpawn", ET_Hook, 6, NULL, Param_Cell, Param_Cell, Param_String, Param_Cell, Param_String, Param_Cell);

    return true;
}

void MdlChagerExt::SDK_OnUnload()
{
    forwards->ReleaseForward(g_pSpawnForward);
    delete g_HPlayerSpawn;
}
