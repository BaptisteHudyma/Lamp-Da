#include <simulator.h>

#include "default_simulation.h"

#include "src/modes/group_type.h"
#include "src/modes/manager_type.h"
#include "src/modes/mode_type.h"

#include "src/modes/default/fixed_modes.h"
#include "src/modes/legacy/legacy_modes.h"

using ManagerTy = modes::ManagerFor<
    modes::FixedModes,
    modes::MiscFixedModes,
    modes::legacy::CalmModes,
    modes::legacy::PartyModes
  >;

struct modeSimulation : public defaultSimulation {
  float fps = 60.f;

  auto get_context() { return modeManager.get_context(); }

  modeSimulation(LedStrip& strip)
      : defaultSimulation(strip),
        modeManager(strip) { }

  void loop(auto&) { loop(); }

#include "src/modes/behavior_manager.h"

private:
  ManagerTy modeManager;
};

int main() {
  return simulator<modeSimulation>::run();
}

