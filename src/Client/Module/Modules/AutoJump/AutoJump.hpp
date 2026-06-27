#pragma once

#include "../Module.hpp"
#include "Events/Game/TickEvent.hpp"

class AutoJump : public Module {
public:
    AutoJump() : Module(
        "Auto Jump",
        "Automatically jumps when you take damage.",
        IDR_HURT_PNG,
        ""
    ) {}

    void onEnable()  override;
    void onDisable() override;
    void defaultConfig() override;
    void settingsRender(float settingsOffset) override;

private:
    int16_t prevHurtTime = 0;
    void onTick(TickEvent& event);
};
