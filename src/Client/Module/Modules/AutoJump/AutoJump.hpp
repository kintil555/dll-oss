#pragma once

#include "../Module.hpp"
#include "Events/Game/TickEvent.hpp"

/**
 * AutoJump Module
 * Automatically jumps when the local player receives damage.
 *
 * Detection: monitors Actor::getHurtTime() — when it transitions
 * from 0 to a positive value the player was just hit.
 *
 * Category: Utility  (no unfair combat advantage — purely reactive)
 */
class AutoJump : public Module {
public:
    AutoJump() : Module(
        "Auto Jump",
        "Automatically jumps when you take damage.",
        IDR_HURT_PNG,
        "",
        false,
        {"autojump", "jump on hit", "damage jump"}
    ) {}

    void onEnable()  override;
    void onDisable() override;
    void defaultConfig() override;
    void settingsRender(float settingsOffset) override;

private:
    int16_t prevHurtTime = 0;

    void onTick(TickEvent& event);
};
