#include <simulator.h>

#include "default_simulation.h"

#include "src/modes/include/group_type.hpp"
#include "src/modes/include/manager_type.hpp"

#include "src/modes/default/fixed_modes.hpp"
#include "src/modes/legacy/legacy_modes.hpp"

struct MyCustomConfig : public modes::DefaultManagerConfig
{
  static constexpr uint32_t defaultCustomRampStepSpeedMs = 8;
  static constexpr uint32_t scrollRampStepSpeedMs = 256;
};

using ManagerTy = modes::ManagerForConfig<MyCustomConfig,
                                          modes::FixedModes,
                                          modes::MiscFixedModes,
                                          modes::legacy::CalmModes,
                                          modes::legacy::PartyModes,
                                          modes::legacy::SoundModes>;

struct modeSimulation : public defaultSimulation
{
  float fps = 60.f;

  auto get_context() { return modeManager.get_context(); }

  modeSimulation(LampTy& lamp) : defaultSimulation(lamp), modeManager(lamp) {}

  void loop(auto&) { loop(); }

#include "src/modes/user/indexable_behavior.hpp"

private:
  ManagerTy modeManager;
};

int main() { return simulator<modeSimulation>::run(); }
