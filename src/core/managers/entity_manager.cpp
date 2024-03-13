/*
 *  This file is part of CounterStrikeSharp.
 *  CounterStrikeSharp is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CounterStrikeSharp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CounterStrikeSharp.  If not, see <https://www.gnu.org/licenses/>. *
 */

#include "core/managers/entity_manager.h"
#include "core/gameconfig.h"
#include "core/log.h"

#include <funchook.h>
#include <vector>
#include <iterator>

#include "core/managers/entity_manager.h"

#include "public/playerslot.h"
#include <public/eiface.h>
#include "scripting/callback_manager.h"

SH_DECL_HOOK6_void(ISource2GameEntities, CheckTransmit, SH_NOATTRIB, false, CCheckTransmitInfo**,
                   int, CBitVec<16384>&, const Entity2Networkable_t**, const uint16*, int);

namespace counterstrikesharp {
EntityManager::EntityManager() {}

EntityManager::~EntityManager() {}

void EntityManager::OnAllInitialized()
{
    SH_ADD_HOOK(ISource2GameEntities, CheckTransmit, g_pSource2GameEntities,
                SH_MEMBER(this, &EntityManager::OnEntityCheckTransmit), true);
    on_entity_spawned_callback = globals::callbackManager.CreateCallback("OnEntitySpawned");
    on_entity_created_callback = globals::callbackManager.CreateCallback("OnEntityCreated");
    on_entity_deleted_callback = globals::callbackManager.CreateCallback("OnEntityDeleted");
    on_entity_check_transmit = globals::callbackManager.CreateCallback("OnEntityCheckTransmit");
    on_entity_parent_changed_callback =
        globals::callbackManager.CreateCallback("OnEntityParentChanged");

    m_pFireOutputInternal = reinterpret_cast<FireOutputInternal>(modules::server->FindSignature(
        globals::gameConfig->GetSignature("CEntityIOOutput_FireOutputInternal")));

    if (m_pFireOutputInternal == nullptr) {
        CSSHARP_CORE_CRITICAL(
            "Failed to find signature for \'CEntityIOOutput_FireOutputInternal\'");
        return;
    }

    auto m_hook = funchook_create();
    funchook_prepare(m_hook, (void**)&m_pFireOutputInternal, (void*)&DetourFireOutputInternal);
    funchook_install(m_hook, 0);

    // Listener is added in ServerStartup as entity system is not initialised at this stage.
}

void EntityManager::OnShutdown()
{
    SH_REMOVE_HOOK(ISource2GameEntities, CheckTransmit, g_pSource2GameEntities,
                   SH_MEMBER(this, &EntityManager::OnEntityCheckTransmit), true);
    globals::callbackManager.ReleaseCallback(on_entity_spawned_callback);
    globals::callbackManager.ReleaseCallback(on_entity_created_callback);
    globals::callbackManager.ReleaseCallback(on_entity_deleted_callback);
    globals::callbackManager.ReleaseCallback(on_entity_parent_changed_callback);
    globals::entitySystem->RemoveListenerEntity(&entityListener);
    globals::callbackManager.ReleaseCallback(on_entity_check_transmit);
}

void CEntityListener::OnEntitySpawned(CEntityInstance* pEntity)
{
    auto callback = globals::entityManager.on_entity_spawned_callback;

    if (callback && callback->GetFunctionCount()) {
        callback->ScriptContext().Reset();
        callback->ScriptContext().Push(pEntity);
        callback->Execute();
    }
}

void EntityManager::OnEntityCheckTransmit(CCheckTransmitInfo** pInfo, int infoCount,
                                          CBitVec<16384>&,
                                          const Entity2Networkable_t** pNetworkables,
                                          const uint16* pEntityIndicies, int nEntities)
{
    CCheckTransmitInfo* pInfoo = *pInfo;
    CBitVec<16384> ffs = *pInfoo->m_pTransmitEntity;
    for (int i = 0; i < nEntities; ++i) {
        // GetEntityIdentity
        CEntityIdentity* identity =
            globals::entitySystem->GetEntityIdentity((CEntityIndex)pEntityIndicies[i]);
        // return m_pGameInfo->GetEntityInfo(pEntityIndicies[i])->flags;
        // int client = pEntityIndicies[i].Get();
        // CPlayer* pPlayer = &m_players[client];
        // auto baseEntity = globals::entitySystem->GetBaseEntity(pEntityIndicies[i]);
        std::cout << "Class name of the entity: " << identity->GetClassname() << std::endl;

        // Now 'entity' points to the entity data, which might allow you to access its name or other
        // properties
    }
    // auto callback = globals::entityManager.on_entity_check_transmit;
    // if (pInfo == nullptr) {
    //     std::cout << "CCheckTransmitInfo array pointer is null." << std::endl;
    //     return;
    // }

    // TransmitInfo* pTransmitInfo = reinterpret_cast<TransmitInfo*>(pInfo[i]);
    // CPlayerSlot playerId = pTransmitInfo->m_nClientEntityIndex.Get();
    // std::cout << "Transmit to playerindex: " << std::endl;
    int sum = 0;
    bool flag = false;

    // for (int i = 0; i < 16384; ++i) {
    //     bool isSet = ffs.IsBitSet(i);
    //     if (isSet == true) {
    //         std::cout << "This bit is set and it's " << i << " in sequence." << std::endl;
    //     }

    //     if (ffs[i].isSet()) {
    //         flag = true;
    //         sum += 1;
    //     }
    //     if (flag == true) {
    //         std::cout << "value for this bit is" << ffs[i] << "  " << i << "th in sequence"
    //                   << std::endl;
    //         if (ffs[i] == 0) {
    //             flag = false;
    //         }
    //     }
    // }
    // std::cout << "In total " << sum << "set to transmission in this iteration, infocount is "
    //           << infoCount << std::endl;
    // for (int i = 0; i < 10; ++i) {
    //     std::cout << "Info for CCheckTransmitInfo object " << i << ":" << std::endl;
    //     *pInfo[i];
    // }
};

void CEntityListener::Hook_SetTransmit(CCheckTransmitInfo* pInfo, bool b)
{
    RETURN_META(MRES_HANDLED);
};
void CEntityListener::OnEntityCreated(CEntityInstance* pEntity)
{
    auto callback = globals::entityManager.on_entity_created_callback;

    auto pEnt = globals::entitySystem->GetBaseEntity(pEntity->GetEntityIndex());
    int offset = 0;
    offset = counterstrikesharp::globals::gameConfig->GetOffset("SetTransmit");

    if (callback && callback->GetFunctionCount()) {
        callback->ScriptContext().Reset();
        callback->ScriptContext().Push(pEntity);
        callback->Execute();
    }
}
void CEntityListener::OnEntityDeleted(CEntityInstance* pEntity)
{
    auto callback = globals::entityManager.on_entity_deleted_callback;

    if (callback && callback->GetFunctionCount()) {
        callback->ScriptContext().Reset();
        callback->ScriptContext().Push(pEntity);
        callback->Execute();
    }
}
void CEntityListener::OnEntityParentChanged(CEntityInstance* pEntity, CEntityInstance* pNewParent)
{
    auto callback = globals::entityManager.on_entity_parent_changed_callback;

    if (callback && callback->GetFunctionCount()) {
        callback->ScriptContext().Reset();
        callback->ScriptContext().Push(pEntity);
        callback->ScriptContext().Push(pNewParent);
        callback->Execute();
    }
}

void EntityManager::HookEntityOutput(const char* szClassname, const char* szOutput,
                                     CallbackT fnCallback, HookMode mode)
{
    auto outputKey = OutputKey_t(szClassname, szOutput);
    CallbackPair* pCallbackPair;

    auto search = m_pHookMap.find(outputKey);
    if (search == m_pHookMap.end()) {
        m_pHookMap[outputKey] = new CallbackPair();
        pCallbackPair = m_pHookMap[outputKey];
    } else
        pCallbackPair = search->second;

    auto* pCallback = mode == HookMode::Pre ? pCallbackPair->pre : pCallbackPair->post;
    pCallback->AddListener(fnCallback);
}

void EntityManager::UnhookEntityOutput(const char* szClassname, const char* szOutput,
                                       CallbackT fnCallback, HookMode mode)
{
    auto outputKey = OutputKey_t(szClassname, szOutput);

    auto search = m_pHookMap.find(outputKey);
    if (search != m_pHookMap.end()) {
        auto* pCallbackPair = search->second;

        auto* pCallback = mode == Pre ? pCallbackPair->pre : pCallbackPair->post;

        pCallback->RemoveListener(fnCallback);

        if (!pCallbackPair->HasCallbacks()) {
            m_pHookMap.erase(outputKey);
        }
    }
}

void DetourFireOutputInternal(CEntityIOOutput* const pThis, CEntityInstance* pActivator,
                              CEntityInstance* pCaller, const CVariant* const value, float flDelay)
{
    std::vector vecSearchKeys{OutputKey_t("*", pThis->m_pDesc->m_pName), OutputKey_t("*", "*")};

    if (pCaller) {
        vecSearchKeys.push_back(OutputKey_t(pCaller->GetClassname(), pThis->m_pDesc->m_pName));
        OutputKey_t(pCaller->GetClassname(), "*");
    }

    std::vector<CallbackPair*> vecCallbackPairs;

    if (pCaller) {
        CSSHARP_CORE_TRACE("[EntityManager][FireOutputHook] - {}, {}", pThis->m_pDesc->m_pName,
                           pCaller->GetClassname());

        auto& hookMap = globals::entityManager.m_pHookMap;

        for (auto& searchKey : vecSearchKeys) {
            auto search = hookMap.find(searchKey);
            if (search != hookMap.end()) {
                vecCallbackPairs.push_back(search->second);
            }
        }
    } else
        CSSHARP_CORE_TRACE("[EntityManager][FireOutputHook] - {}, unknown caller",
                           pThis->m_pDesc->m_pName);

    HookResult result = HookResult::Continue;

    for (auto pCallbackPair : vecCallbackPairs) {
        if (pCallbackPair->pre->GetFunctionCount()) {
            pCallbackPair->pre->ScriptContext().Reset();
            pCallbackPair->pre->ScriptContext().Push(pThis);
            pCallbackPair->pre->ScriptContext().Push(pThis->m_pDesc->m_pName);
            pCallbackPair->pre->ScriptContext().Push(pActivator);
            pCallbackPair->pre->ScriptContext().Push(pCaller);
            pCallbackPair->pre->ScriptContext().Push(value);
            pCallbackPair->pre->ScriptContext().Push(flDelay);

            for (auto fnMethodToCall : pCallbackPair->pre->GetFunctions()) {
                if (!fnMethodToCall)
                    continue;
                fnMethodToCall(&pCallbackPair->pre->ScriptContextStruct());

                auto thisResult = pCallbackPair->pre->ScriptContext().GetResult<HookResult>();

                if (thisResult >= HookResult::Stop) {
                    return;
                }

                if (thisResult > result) {
                    result = thisResult;
                }
            }
        }
    }

    if (result >= HookResult::Handled) {
        return;
    }

    m_pFireOutputInternal(pThis, pActivator, pCaller, value, flDelay);

    for (auto pCallbackPair : vecCallbackPairs) {
        if (pCallbackPair->post->GetFunctionCount()) {
            pCallbackPair->post->ScriptContext().Reset();
            pCallbackPair->post->ScriptContext().Push(pThis);
            pCallbackPair->post->ScriptContext().Push(pThis->m_pDesc->m_pName);
            pCallbackPair->post->ScriptContext().Push(pActivator);
            pCallbackPair->post->ScriptContext().Push(pCaller);
            pCallbackPair->post->ScriptContext().Push(value);
            pCallbackPair->post->ScriptContext().Push(flDelay);
            pCallbackPair->post->Execute();
        }
    }
}

} // namespace counterstrikesharp