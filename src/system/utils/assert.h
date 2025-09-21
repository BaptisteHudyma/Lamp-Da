#ifndef SRC_SYSTEM_ASSERT_H
#define SRC_SYSTEM_ASSERT_H

#include <cassert>

#ifdef NDEBUG
static bool assertBool(int expression) { return expression; }
#else
#define assertBool(condition) \
  assert(condition);          \
  true
#endif

#endif
