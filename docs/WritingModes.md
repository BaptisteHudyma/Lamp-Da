
# Writing your own lighting mode

This document can be viewed as an introduction to development on the project,
as well as a way for beginners to write a new "lighting mode" for one of the
boards specified in this project.

## 0. Introduction: enable your first mode

First, the directory structure:

 - when writing a mode, you will mostly interact with code in `src/modes`
 - most board behaviors and lower level implementations is in `src/system`
 - the two worlds integrate together in `src/user`

The examples used in this guide are provided in `modes/examples`.

We will start with reading the `00_intro_mode.hpp` example:

```cpp
namespace modes::examples {

struct IntroMode : public BasicMode
{
  static void loop(auto& ctx) {
    ctx.lamp.fill(colors::Chartreuse);
    ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);
  }
};

} // namespace modes::examples
```

We recommend that you put your modes inside a `modes::` namespace.

For example, if you are going to write some games for this board, feel free to
create a new `modes/custom/games/` directory, then a
`modes/custom/games/my_game_mode.hpp` file, with the following content:

```cpp
// keep the global namespaces clean! (& it gives you access to modes:: also)
namespace modes::games {

struct MyGameMode : public BasicMode
{
  /*
        ... some BasicMode callbacks implementation ...
  */
};

} // namespace modes::games
```

There are several rules while writing a mode:

 - your new mode object needs to inherit from `public BasicMode`
 - all your code shall be contained inside your new mode object
 - **all the mode methods and callbacks must be defined `static`**
 - all the mode members must be declared `static constexpr`

What you will observe as you experiment, is that mode structures such as
`IntroMode` and `MyGameMode` are completely state-less and are never
actually constructed during the runtime of the program. Hence, all the
following usages are **strongly discouraged**:

```cpp

int badGlobalVariable = 5;
const int badConstant = 3;
static int forbiddenStuff = 1;
// +everything shall be inside the mode struct!

struct VeryBadMode : public BasicMode
{
    //
    // the bad way
    //

    int badMember = 8;
    const int badConstant = 3;
    static int forbiddenWhat = 5;
    constexpr int badMistake = 1;

    static void loop(auto& ctx) {
      static int counter = 0; // !! do NOT use inline static, use StateTy !!
      counter += 1;
    }

    void _stateful_method() {

    }

    //
    // the good way
    //

    static constexpr int goodConstant = 0;

    static void _stateful_method(auto& ctx)
    {
      // ... some interaction with ctx.state ...
    }

    static constexpr int _stateless_helper(int ax, int ay, int bx, int by)
    {
      int dx = abs(ax - bx);
      int dy = abs(ay - by);
      return dx * dx + dy * dy;
    }
};

```

You can view that structure as *a collection of static callbacks* that
implements your mode.

**Note that only these callbacks will be called at runtime**.

For example, if we look at the default `BasicMode` implementation of the
`void loop(auto& ctx)` callback:

```cpp
  /** \brief Custom user mode loop function (default)
   *
   * Loop function each tick called whenever the mode is set as the currently
   * active mode by the user
   *
   * \param[in] ctx The current context, providing a interface to the local
   * state, the mode manager, as well as the hardware through its lamp object
   */
  static void loop(auto& ctx) { return; }
```

We see that by default, the `.loop()` callback does nothing.

Coming back at `00_intro_mode.hpp` we see that it implements it as follows:

```cpp
  static void loop(auto& ctx) {
    ctx.lamp.fill(colors::Chartreuse);
    ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);
  }
```

Here the `ctx` object makes available two important members:

 - the `ctx.lamp` object that exposes to you user the lamp hardware
 - the `ctx.state` object that exposes to you your own mode state

We will later see how to make your mode stateful by defining a
custom `StateTy`.

Here, looking at the `LampTy` documentation, we see the following:

```cpp
  /// \brief Set brightness of the lamp
  void LMBD_INLINE setBrightness(const brightness_t brightness, /* ... */)

  /// \brief (indexable) Fill lamp with target color
  void LMBD_INLINE fill(uint32_t color)

  /// \brief (physical) Tick number, ever-increasing at 60fps
  volatile const uint32_t tick;
```

The `IntroMode` loop function thus fills the lamp with `colors::Chartreuse`
(see documentation for `modes/include/colors/palettes.hpp`) and then, vary
the brightness of the lamp depending on the current tick number.

One remark here, is that "colors" are meaningful only if the board is set up
with an RGB LED strip, meaning that this mode will fail if `make simple` or
`make cct` have been used during build.

To support multiple board flavors within a mode, use the following:

```cpp
struct IntroMode : public BasicMode
{
  static void loop(auto& ctx) {

    // this code is NOT included if "lamp flavor" is not RGB-indexable
    if constexpr (ctx.lamp.flavor == hardware::LampTypes::indexable) {
      ctx.lamp.fill(colors::Chartreuse);
    }

    ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);
  }
};
```

If you tried to modify `modes/examples/00_intro_mode.hpp` by yourself during
this guide, you may have noticed that it is not compiled / not included at all.

This is because you need to *enable the mode* inside the global mode manager.

In order to do that, you can edit `src/user/indexable_functions.cpp` as
follows:

```cpp
// ...

#include "src/modes/default/fixed_modes.hpp"
#include "src/modes/legacy/legacy_modes.hpp"

// we include our mode file last
#include "src/modes/example/00_intro_mode.hpp"

namespace user {

//
// list your groups & modes here
//

using IntroGroup = modes::GroupFor<modes::examples::IntroMode>;
using ManagerTy = modes::ManagerFor<IntroGroup>;

/* !! you will need to comment the default mode manager config !!
using ManagerTy = modes::ManagerFor</* all the default groups */>;
```

Here, we declare a *mode group* named `IntroGroup` that contains only one mode,
our new `modes::examples::IntroMode` and nothing else. We are then able to pass
a list to the manager of all the groups we want to include in our
configuration, here our `IntroGroup` that only includes `IntroMode`.
