#include "AutoJump.hpp"
#include "SDK/SDK.hpp"

void AutoJump::defaultConfig() {
    Module::defaultConfig("core");
    setDef("onGroundOnly", false);
}

void AutoJump::settingsRender(float settingsOffset) {
    initSettingsPage();

    addToggle("On Ground Only",
              "Only jump when standing on the ground.",
              "onGroundOnly");

    FlarialGUI::UnsetScrollView();
    resetPadding();
}

void AutoJump::onEnable() {
    prevHurtTime = 0;
    Listen(this, TickEvent, &AutoJump::onTick)
    Module::onEnable();
}

void AutoJump::onDisable() {
    Deafen(this, TickEvent, &AutoJump::onTick)
    Module::onDisable();
}

void AutoJump::onTick(TickEvent& event) {
    if (!this->isEnabled()) return;
    if (!SDK::hasInstanced || !SDK::clientInstance) return;

    auto* player = SDK::clientInstance->getLocalPlayer();
    if (!player) return;

    const int16_t currentHurtTime = player->getHurtTime();
    const bool justHurt = (prevHurtTime == 0 && currentHurtTime > 0);
    prevHurtTime = currentHurtTime;

    if (!justHurt) return;

    if (getOps<bool>("onGroundOnly")) {
        if (player->getActorFlag(ActorFlags::FLAG_GLIDING)  ||
            player->getActorFlag(ActorFlags::FLAG_SWIMMING) ||
            player->getActorFlag(ActorFlags::FLAG_SLEEPING))
            return;
    }

    auto handler = player->getHandler();
    handler.setMJumpDown(true);
    handler.setJumping(true);
}
