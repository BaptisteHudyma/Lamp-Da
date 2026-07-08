/*! \file overlay.hpp
    \brief Lamp overlay manager
*/

#ifndef MODES_DRAW_OVERLAY_HPP
#define MODES_DRAW_OVERLAY_HPP

namespace lampda::modes::draw {
/// Contain the overlay menu handles
namespace overlay {

///< Define an UI element type
enum class ElementType : uint8_t
{
  NONE = 0,
  RAMP, ///< Display a ramp
  DOT,  ///< Display a pixel dot
};

namespace __private {

///< define a display color
struct Color
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint32_t get_color() const { return (red << 16) | (green << 8) | blue; }
};

/**
 * \brief Define an UI element
 */
struct UIElement
{
  ElementType type = ElementType::NONE; ///< type of UI elements
  uint8_t progress;                     ///< 0-255 animation progress

  uint16_t coordinateX; ///< start X coordinates
  uint16_t coordinateY; ///< start Y coordinates

  colors::PaletteTy color; ///< color palette to display

  int32_t timeout_ms = -1; ///< If set to a positive number, this element will delete itself at the set time.
};

} // namespace __private

template<int UIElementSize = 16, uint8_t freezeSize = 5, uint8_t nbBlackLines = 2> class Manager
{
public:
  Manager() : activeUiElements(0) {}

  /**
   * \brief Display and update the overlay
   */
  void display_update(auto& ctx)
  {
    assert(activeUiElements < UIElementSize);

    // restore first led skip to be able to draw
    ctx.skipFirstLedsForFrames(0);

    if (activeUiElements == 0)
      return;

    // clear overlay area first
    for (uint8_t i = 0; i < ctx.lamp.maxWidth * nbBlackLines; ++i)
    {
      ctx.lamp.setPixelColor(i, 0);
    }

    // keep track of the last active element index
    size_t lastActiveElementIndex = 0;
    const auto timeNow = ctx.lamp.now;
    const size_t activeUiElementsToDisplay = activeUiElements;
    // display all elements
    for (uint8_t i = 0; i < activeUiElementsToDisplay; ++i)
    {
      const auto elementTimeout = elements[i].timeout_ms;
      // auto deletion of element
      if (elementTimeout > 0 and timeNow > elementTimeout)
      {
        // remove one active element
        if (activeUiElements > 0)
          --activeUiElements;
      }
      else
      {
        drawElement(elements[i], ctx);

        // if an element before this was delete, shift this element index
        if (i != lastActiveElementIndex)
        {
          // shift the saved elements
          elements[lastActiveElementIndex] = elements[i];
        }
        ++lastActiveElementIndex;
      }
    }

    // relock the display capabilities for the remaining time
    ctx.skipFirstLedsForFrames(ctx.lamp.maxWidth * nbBlackLines, freezeSize);
  }

  /// Clear all UI elements
  void clear() { activeUiElements = 0; }

  /// Return the count of elements of a target type
  uint8_t get_element_count(const ElementType type) const
  {
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < activeUiElements; ++i)
    {
      // find elements of a type
      if (elements[i].type == type)
      {
        cnt++;
      }
    }
    return cnt;
  }

  /**
   * \brief Add a new UI element to the overlay
   * \param[in] type Type of the element. Depending on the type, some parameters may be ignored
   * \param[in] palette Color palette to apply, controled by the progress parameter
   * \param[in] coordinateX Start x coordinate
   * \param[in] coordinateY Start y coordinate
   * \param[in] progress Progress of the element, controls the color and other aspects
   * \param[in] activityDelay_ms Longevity delay affected to this element, after which it destroys itselfs
   * \return True if the element was successfully added
   */
  bool add_ui_element(const auto& ctx,
                      const ElementType type,
                      const colors::PaletteTy& palette,
                      const uint16_t coordinateX = 0,
                      const uint16_t coordinateY = 0,
                      const uint8_t progress = 0,
                      const uint32_t activityDelay_ms = 50)
  {
    if (activeUiElements >= UIElementSize - 1)
      return false;

    // set the UI element
    __private::UIElement& element = elements[activeUiElements];
    element.type = type;
    element.progress = progress;
    element.coordinateX = coordinateX;
    element.coordinateY = coordinateY;
    element.color = palette;
    // set optionnal auto destroy
    if (activityDelay_ms > 0)
      element.timeout_ms = ctx.lamp.now + activityDelay_ms;

    // increase element count
    activeUiElements++;
    return true;
  }

  /**
   * \brief Update the target UI element progress
   * \param[in] type Type of the element to update
   * \param[in] desiredIndex Nth element of the target type to select
   * \param[in] progress Progress value to update
   * \return True if the element was found and updated
   */
  template<bool shouldUpdateTimeout = true>
  bool update_type_progress(const auto& ctx, const ElementType type, uint16_t desiredIndex, const uint8_t progress)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      elements[index].progress = progress;

      // if requested, update timeout
      const uint32_t newTimeout = ctx.lamp.now + freezeSize * ctx.lamp.frameDurationMs;
      if (shouldUpdateTimeout and elements[index].timeout_ms <= newTimeout)
      {
        // add two frames of breathing room
        elements[index].timeout_ms = newTimeout;
      }
      return true;
    }
    return false;
  }

  /**
   * \brief Update the target UI element color palette
   * \param[in] type Type of the element to update
   * \param[in] desiredIndex Nth element of the target type to select
   * \param[in] palette Color value to update
   * \return True if the element was found and updated
   */
  template<bool shouldUpdateTimeout = true> bool update_type_color(const auto& ctx,
                                                                   const ElementType type,
                                                                   uint16_t desiredIndex,
                                                                   const colors::PaletteTy& palette)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      elements[index].color = palette;

      // if requested, update timeout
      const uint32_t newTimeout = ctx.lamp.now + freezeSize * ctx.lamp.frameDurationMs;
      if (shouldUpdateTimeout and elements[index].timeout_ms <= newTimeout)
      {
        // add two frames of breathing room
        elements[index].timeout_ms = newTimeout;
      }
      return true;
    }
    return false;
  }

  bool update_type_timeout(const auto& ctx, const ElementType type, uint16_t desiredIndex, const uint32_t timeout_ms)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      // add two frames of breathing room
      elements[index].timeout_ms = ctx.lamp.now + timeout_ms;
      return true;
    }
    return false;
  }

  /**
   * \brief Update the target UI element
   * \param[in] type Type of the element to update
   * \param[in] desiredIndex Nth element of the target type to select
   * \param[in] progress Progress value to update
   * \param[in] palette Colors to update
   * \return True if the element was found and updated
   */
  template<bool shouldUpdateTimeout = true> bool update_type(const auto& ctx,
                                                             const ElementType type,
                                                             uint16_t desiredIndex,
                                                             const uint8_t progress,
                                                             const colors::PaletteTy& palette)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      elements[index].progress = progress;
      elements[index].color = palette;

      // if requested, update timeout
      const uint32_t newTimeout = ctx.lamp.now + freezeSize * ctx.lamp.frameDurationMs;
      if (shouldUpdateTimeout and elements[index].timeout_ms <= newTimeout)
      {
        // add two frames of breathing room
        elements[index].timeout_ms = newTimeout;
      }
      return true;
    }
    return false;
  }

protected:
  /// Find and return the Nth element of a given type
  bool get_N_of_type(const ElementType type, uint16_t desiredIndex, size_t& elementId) const
  {
    // display all elements
    for (uint8_t i = 0; i < activeUiElements; ++i)
    {
      // find elements of a type
      if (elements[i].type == type)
      {
        // check that we reached the desired element
        if (desiredIndex <= 0)
        {
          elementId = i;
          return true;
        }
        // or decrement the element count
        desiredIndex -= 1;
      }
    }
    return false;
  }

  void drawElement(const __private::UIElement& e, auto& ctx) const
  {
    switch (e.type)
    {
      case ElementType::RAMP:
        {
          display_ramp(ctx, e.progress, e.color, e.coordinateY);
          break;
        }
      case ElementType::DOT:
        {
          display_dot(ctx, e.progress, e.color, e.coordinateX, e.coordinateY);
          break;
        }
      case ElementType::NONE:
        {
          break;
        }

        // NO DEFAULT: so we have errors on compile
    }
  }

  /// Display a colored ramp
  void display_ramp(auto& ctx,
                    const uint8_t progress,
                    const colors::PaletteTy& palette,
                    const uint16_t startCoordinateY) const
  {
    const uint16_t rampScale = (progress / 255.0) * ctx.lamp.maxWidth;
    for (uint16_t i = 0; i < rampScale; ++i)
    {
      const auto color = modes::colors::from_palette<false>(lmpd_map<uint8_t>(i, 0, rampScale, 0, 255), palette);
      ctx.lamp.setPixelColorXY(i, startCoordinateY, color);
    }
  }

  /// Display a single colored pixel
  void display_dot(auto& ctx,
                   const uint8_t progress,
                   const colors::PaletteTy& palette,
                   const uint16_t coordinateX,
                   const uint16_t coordinateY) const
  {
    const auto color = modes::colors::from_palette<false>(progress, palette);
    ctx.lamp.setPixelColorXY(coordinateX, coordinateY, color);
  }

private:
  /// ordered storage for UI elements: higher priority are stored at lower indices
  std::array<__private::UIElement, UIElementSize> elements;
  /// stored UI elements
  size_t activeUiElements = 0;
};

} // namespace overlay
} // namespace lampda::modes::draw

#endif
