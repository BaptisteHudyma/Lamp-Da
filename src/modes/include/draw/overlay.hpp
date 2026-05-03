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

    skipedFrames++;
    if (skipedFrames >= freezeSize)
      clear();

    if (activeUiElements == 0)
      return;

    // clear overlay area first
    for (uint8_t i = 0; i < ctx.lamp.maxWidth * nbBlackLines; ++i)
    {
      ctx.lamp.setPixelColor(i, 0);
    }

    // display all elements
    for (uint8_t i = 0; i < activeUiElements; ++i)
    {
      drawElement(elements[i], ctx);
    }

    // relock the display capabilities for the remaning time
    ctx.skipFirstLedsForFrames(ctx.lamp.maxWidth * nbBlackLines, freezeSize - skipedFrames);
  }

  /// Clear all UI elements
  void clear()
  {
    activeUiElements = 0;
    skipedFrames = 0;
  }

  /// Add a new UI element
  bool add_ui_element(const ElementType type,
                      const colors::PaletteTy& palette,
                      const uint16_t coordinateX = 0,
                      const uint16_t coordinateY = 0,
                      const uint8_t progress = 0)
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

    // increase element count
    activeUiElements++;
    skipedFrames = 0;
    return true;
  }

  /**
   * \brief Update the target UI element progress
   * \param[in] type Type of the element to update
   * \param[in] desiredIndex Nth element of the target type to select
   * \param[in] progress Progress value to update
   * \return True if the element was found and updated
   */
  bool update_type_progress(const ElementType type, uint16_t desiredIndex, const uint8_t progress)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      skipedFrames = 0;
      elements[index].progress = progress;
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
  bool update_type_color(const ElementType type, uint16_t desiredIndex, const colors::PaletteTy& palette)
  {
    size_t index;
    if (get_N_of_type(type, desiredIndex, index))
    {
      skipedFrames = 0;
      elements[index].color = palette;
      return true;
    }
    return false;
  }

protected:
  /// Find and return the Nth element of a given type
  bool get_N_of_type(const ElementType type, uint16_t desiredIndex, size_t& elementId)
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

  void drawElement(const __private::UIElement& e, auto& ctx)
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
                    const uint16_t startCoordinateY)
  {
    const uint16_t rampScale = (progress / 255.0) * ctx.lamp.maxWidth;
    for (uint16_t i = 0; i < rampScale; ++i)
    {
      const auto color = modes::colors::from_palette(lmpd_map<uint8_t>(i, 0, rampScale, 0, 255), palette);
      ctx.lamp.setPixelColorXY(i, startCoordinateY, color);
    }
  }

  /// Display a single colored pixel
  void display_dot(auto& ctx,
                   const uint8_t progress,
                   const colors::PaletteTy& palette,
                   const uint16_t coordinateX,
                   const uint16_t coordinateY)
  {
    const auto color = modes::colors::from_palette<false>(progress, palette);
    ctx.lamp.setPixelColorXY(coordinateX, coordinateY, color);
  }

private:
  /// keep track of skiped frames since the last update
  volatile size_t skipedFrames = 0;
  /// ordered storage for UI elements: higher priority are stored at lower indices
  std::array<__private::UIElement, UIElementSize> elements;
  /// stored UI elements
  size_t activeUiElements = 0;
};

} // namespace overlay
} // namespace lampda::modes::draw

#endif
