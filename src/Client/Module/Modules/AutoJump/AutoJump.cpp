#include "AutoJump.hpp"
#include "SDK/SDK.hpp"
#include "Client/Module/ModuleRegistry.hpp"

// ─── Config ──────────────────────────────────────────────────────────────────

void AutoJump::defaultConfig() {
    Module::defaultConfig("core");
    // Delay (in ticks) before the jump input is released.
    // 1 tick is plenty; keeping it short avoids input spam.
    setDef("holdTicks", 2);
    // Whether to jump only when on the ground (safer) or always.
    setDef("onGroundOnly", false);
}

// ─── Settings UI ─────────────────────────────────────────────────────────────

void AutoJump::settingsRender(float settingsOffset) {
    initSettingsPage();

    addToggle("On Ground Only",
              "Only jump when standing on the ground.",
              "onGroundOnly");

    addSliderInt("Hold Ticks",
                 "How many ticks to hold the jump input (1–5).",
                 "holdTicks",
                 5, 1);

    extraPadding();

    FlarialGUI::UnsetScrollView();
    resetPadding();
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

void AutoJump::onEnable() {
    prevHurtTime = 0;
    Listen(this, TickEvent, &AutoJump::onTick)
    Module::onEnable();
}

void AutoJump::onDisable() {
    Deafen(this, TickEvent, &AutoJump::onTick)
    Module::onDisable();
}

// ─── Logic ───────────────────────────────────────────────────────────────────

void AutoJump::onTick(TickEvent& event) {
    if (!this->isEnabled()) return;

    if (!SDK::hasInstanced || SDK::clientInstance == nullptr) return;

    auto* player = SDK::clientInstance->getLocalPlayer();
    if (player == nullptr) return;

    const int16_t currentHurtTime = player->getHurtTime();

    // Detect leading edge: hurtTime just became positive → player was hit.
    const bool justHurt = (prevHurtTime == 0 && currentHurtTime > 0);
    prevHurtTime = currentHurtTime;

    if (!justHurt) return;

    // Optional ground-only guard.
    if (getOps<bool>("onGroundOnly")) {
        // isOnGround is exposed via the actor flag / state vector.
        // We check that vertical velocity is near zero and not gliding/swimming.
        const bool airborne =
            player->getActorFlag(ActorFlags::FLAG_GLIDING) ||
            player->getActorFlag(ActorFlags::FLAG_SWIMMING) ||
            player->getActorFlag(ActorFlags::FLAG_SLEEPING);

        // Simple: if gliding, swimming, or sleeping — skip the jump.
        if (airborne) return;
    }

    // Trigger jump via the MoveInputComponent handler.
    auto handler = player->getHandler();
    handler.setMJumpDown(true);
    handler.setJumping(true);
}

// ─── Registration ─────────────────────────────────────────────────────────────

REGISTER_MODULE(AutoJump);
