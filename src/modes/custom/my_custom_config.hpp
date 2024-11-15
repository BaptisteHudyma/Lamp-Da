#ifndef MY_CUSTOM_CONFIG_H
#define MY_CUSTOM_CONFIG_H

// uncomment only what you need to override
//
struct MyCustomConfig : public modes::DefaultManagerConfig {
  // static constexpr bool defaultRampSaturates = true;
  // static constexpr bool defaultClearStripOnModeChange = false;
  // static constexpr uint32_t defaultCustomRampStepSpeedMs = 32;
};

template <typename... Groups>
using CustomManagerFor = modes::ManagerForConfig<MyCustomConfig, Groups...>;

#endif
