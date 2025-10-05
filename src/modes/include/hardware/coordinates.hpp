#ifndef MODES_HARDWARE_COORDINATES_HPP
#define MODES_HARDWARE_COORDINATES_HPP

#include <array>

namespace modes::hardware::details {

// For each Y coord, return the apparent "LED shift required" to align to grid
template<size_t maxOverflowHeight, typename _OutTy = std::array<uint16_t, maxOverflowHeight + 1>>
static constexpr _OutTy computeResidues(float realExactWidth)
{
  uint16_t floorWidth = floor(realExactWidth);

  _OutTy results {};
  for (size_t Y = 0; Y < results.size(); ++Y)
  {
    results[Y] = ((float)Y) * realExactWidth - ((float)Y * floorWidth);
  }

  return results;
}

// For each Y coord, return True iff given row is 1 LED longer than the others
template<size_t maxOverflowHeight, typename _OutTy = std::array<bool, maxOverflowHeight + 1>>
static constexpr _OutTy computeDeltaResidues(float realExactWidth)
{
  auto allResiduesY = computeResidues<maxOverflowHeight>(realExactWidth);

  _OutTy results {};
  for (size_t Y = 0; Y < results.size(); ++Y)
  {
    if (Y + 1 < allResiduesY.size())
      results[Y] = (bool)(allResiduesY[Y] != allResiduesY[Y + 1]);
    else
      results[Y] = false;
  }

  return results;
}

template<size_t maxOverflowHeight, typename _OutTy = std::array<int, maxOverflowHeight + 1>>
static constexpr _OutTy computeExtraShiftResidues(float realExactWidth)
{
  auto allDeltaResiduesY = computeDeltaResidues<maxOverflowHeight>(realExactWidth);

  int total = 0;
  _OutTy results {};
  for (size_t Y = 0; Y < results.size(); ++Y)
  {
    results[Y] = total;
    if (Y + 1 >= allDeltaResiduesY.size())
      continue;
    if (allDeltaResiduesY[Y] && allDeltaResiduesY[Y + 1])
      total += 1;
    if (!allDeltaResiduesY[Y] && !allDeltaResiduesY[Y + 1])
      total -= 1;
  }

  return results;
}

} // namespace modes::hardware::details

#endif
