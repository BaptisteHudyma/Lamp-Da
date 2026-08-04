#ifndef MODES_NAVIGATION_REQUEST_HPP
#define MODES_NAVIGATION_REQUEST_HPP

#include <cstdint>

namespace lampda::modes::navigation {

struct GroupChangeRequest
{
  uint8_t targetGroup = 0;
  bool pending = false;
};

inline GroupChangeRequest groupChangeRequest;

/**
 * Request a group change at the end of the current mode update.
 */
inline void request_group_change(const uint8_t targetGroup)
{
  groupChangeRequest.targetGroup = targetGroup;
  groupChangeRequest.pending = true;
}

/**
 * Apply a pending navigation request using a root manager context.
 */
template<typename ManagerContext>
void process(ManagerContext& manager)
{
  if (!groupChangeRequest.pending)
    return;

  const uint8_t targetGroup =
      groupChangeRequest.targetGroup;

  // Clear before changing group, as changing group can
  // immediately execute callbacks that request navigation.
  groupChangeRequest.pending = false;

  const uint8_t groupCount =
      manager.get_groups_count();

  if (targetGroup >= groupCount)
    return;

  if (targetGroup == manager.get_active_group())
    return;

  manager.set_active_group(
      targetGroup,
      groupCount);
}

} // namespace lampda::modes::navigation

#endif