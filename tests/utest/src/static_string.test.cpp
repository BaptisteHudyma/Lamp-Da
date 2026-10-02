#include "src/system/common/static_string.h"

#include <gtest/gtest.h>
#include <sstream>

namespace lampda::common {

// ============================================================================
// TEST FIXTURES
// ============================================================================

class StaticStringTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Setup before each test
  }

  void TearDown() override
  {
    // Cleanup after each test
  }
};

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================

TEST_F(StaticStringTest, DefaultConstructor)
{
  static_string<32> str;
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0, str.size());
  EXPECT_EQ('\0', str.c_str()[0]);
}

TEST_F(StaticStringTest, CStringConstructor)
{
  static_string<32> str("Hello, World!");
  EXPECT_EQ(13, str.size());
  EXPECT_STREQ("Hello, World!", str.c_str());
  EXPECT_EQ('\0', str.c_str()[13]);
}

TEST_F(StaticStringTest, CStringConstructorWithTruncation)
{
  static_string<5> str("Hello World");
  EXPECT_EQ(5, str.size());
  EXPECT_STREQ("Hello", str.c_str());
  EXPECT_TRUE(str.is_full());
}

TEST_F(StaticStringTest, CStringConstructorWithCount)
{
  static_string<32> str("Hello", 3);
  EXPECT_EQ(3, str.size());
  EXPECT_STREQ("Hel", str.c_str());
}

TEST_F(StaticStringTest, CopyConstructor)
{
  static_string<32> original("Copy Me");
  static_string<32> copy(original);
  EXPECT_STREQ("Copy Me", copy.c_str());
  EXPECT_EQ(original.size(), copy.size());
}

TEST_F(StaticStringTest, CopyConstructorDifferentSizes)
{
  static_string<16> small("Test");
  static_string<64> large(small);
  EXPECT_STREQ("Test", large.c_str());
  EXPECT_EQ(small.size(), large.size());
}

TEST_F(StaticStringTest, CopyConstructorWithTruncation)
{
  static_string<64> large("This is a longer string");
  static_string<8> small(large);
  EXPECT_EQ(8, small.size());
}

// ============================================================================
// ASSIGNMENT TESTS
// ============================================================================

TEST_F(StaticStringTest, AssignmentOperator)
{
  static_string<32> str1("First");
  static_string<32> str2;
  str2 = str1;
  EXPECT_STREQ("First", str2.c_str());
}

TEST_F(StaticStringTest, CStringAssignment)
{
  static_string<32> str;
  str = "Second";
  EXPECT_STREQ("Second", str.c_str());
  EXPECT_EQ(6, str.size());
}

TEST_F(StaticStringTest, AssignmentFromLongerString)
{
  static_string<5> str;
  str = "HelloWorld";
  EXPECT_STREQ("Hello", str.c_str());
  EXPECT_TRUE(str.is_full());
}

TEST_F(StaticStringTest, SelfAssignment)
{
  static_string<32> str("Hello");
  str = str;
  EXPECT_STREQ("Hello", str.c_str());
}

// ============================================================================
// ELEMENT ACCESS TESTS
// ============================================================================

TEST_F(StaticStringTest, OperatorBrackets)
{
  static_string<32> str("Hello");
  EXPECT_EQ('H', str[0]);
  EXPECT_EQ('e', str[1]);
  EXPECT_EQ('o', str[4]);
}

TEST_F(StaticStringTest, Front)
{
  static_string<32> str("Hello");
  EXPECT_EQ('H', str.front());
}

TEST_F(StaticStringTest, Back)
{
  static_string<32> str("Hello");
  EXPECT_EQ('o', str.back());
}

TEST_F(StaticStringTest, Data)
{
  static_string<32> str("Hello");
  const char* ptr = str.data();
  EXPECT_EQ('H', *ptr);
}

TEST_F(StaticStringTest, CStr)
{
  static_string<32> str("Hello");
  const char* c_str = str.c_str();
  EXPECT_STREQ("Hello", c_str);
}

// ============================================================================
// PUSH_BACK AND POP_BACK TESTS
// ============================================================================

TEST_F(StaticStringTest, PushBackSingle)
{
  static_string<32> str;
  str.push_back('A');
  EXPECT_EQ(1, str.size());
  EXPECT_EQ('A', str[0]);
}

TEST_F(StaticStringTest, PushBackMultiple)
{
  static_string<32> str;
  str.push_back('A');
  str.push_back('B');
  str.push_back('C');
  EXPECT_EQ(3, str.size());
  EXPECT_STREQ("ABC", str.c_str());
}

TEST_F(StaticStringTest, PushBackOnFull)
{
  static_string<3> str("ABC");
  str.push_back('D'); // Should silently fail
  EXPECT_EQ(3, str.size());
  EXPECT_STREQ("ABC", str.c_str());
}

TEST_F(StaticStringTest, PopBack)
{
  static_string<32> str("ABC");
  str.pop_back();
  EXPECT_EQ(2, str.size());
  EXPECT_STREQ("AB", str.c_str());
}

TEST_F(StaticStringTest, PopBackOnEmpty)
{
  static_string<32> str;
  str.pop_back(); // Should silently do nothing
  EXPECT_EQ(0, str.size());
}

TEST_F(StaticStringTest, PopBackUntilEmpty)
{
  static_string<32> str("AB");
  str.pop_back();
  str.pop_back();
  EXPECT_TRUE(str.empty());
  EXPECT_STREQ("", str.c_str());
}

// ============================================================================
// CLEAR TESTS
// ============================================================================

TEST_F(StaticStringTest, Clear)
{
  static_string<32> str("Hello");
  str.clear();
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(0, str.size());
}

TEST_F(StaticStringTest, ClearAndReuse)
{
  static_string<32> str("Hello");
  str.clear();
  str = "World";
  EXPECT_STREQ("World", str.c_str());
}

// ============================================================================
// APPEND TESTS
// ============================================================================

TEST_F(StaticStringTest, AppendCString)
{
  static_string<32> str("Hello");
  str.append(" World");
  EXPECT_EQ(11, str.size());
  EXPECT_STREQ("Hello World", str.c_str());
}

TEST_F(StaticStringTest, AppendWithCount)
{
  static_string<32> str("Start");
  str.append(" 123456", 3);
  EXPECT_STREQ("Start 12", str.c_str());
}

TEST_F(StaticStringTest, AppendWithTruncation)
{
  static_string<8> str("Hi");
  str.append(" everyone");
  EXPECT_EQ(8, str.size());
  EXPECT_TRUE(str.is_full());
}

TEST_F(StaticStringTest, AppendAnotherString)
{
  static_string<32> str1("Hello");
  static_string<32> str2(" World");
  str1.append(str2);
  EXPECT_STREQ("Hello World", str1.c_str());
}

TEST_F(StaticStringTest, AppendEmptyString)
{
  static_string<32> str("Hello");
  str.append("");
  EXPECT_STREQ("Hello", str.c_str());
  EXPECT_EQ(5, str.size());
}

// ============================================================================
// OPERATOR+= TESTS
// ============================================================================

TEST_F(StaticStringTest, OperatorPlusEqualsCharacter)
{
  static_string<32> str("Hello");
  str += ' ';
  EXPECT_STREQ("Hello ", str.c_str());
}

TEST_F(StaticStringTest, OperatorPlusEqualsCString)
{
  static_string<32> str("Hello");
  str += "World";
  EXPECT_STREQ("HelloWorld", str.c_str());
}

TEST_F(StaticStringTest, OperatorPlusEqualsString)
{
  static_string<32> str1("Hello");
  static_string<32> str2("World");
  str1 += str2;
  EXPECT_STREQ("HelloWorld", str1.c_str());
}

// ============================================================================
// FIND TESTS
// ============================================================================

TEST_F(StaticStringTest, FindCharacter)
{
  static_string<32> str("Hello World");
  size_t pos = str.find('o');
  EXPECT_EQ(4, pos);
}

TEST_F(StaticStringTest, FindCharacterWithPosition)
{
  static_string<32> str("Hello World");
  size_t pos = str.find('o', 5);
  EXPECT_EQ(7, pos);
}

TEST_F(StaticStringTest, FindCharacterNotFound)
{
  static_string<32> str("Hello");
  size_t pos = str.find('x');
  EXPECT_EQ(static_string<32>::npos, pos);
}

TEST_F(StaticStringTest, FindSubstring)
{
  static_string<32> str("The quick brown fox");
  size_t pos = str.find("quick");
  EXPECT_EQ(4, pos);
}

TEST_F(StaticStringTest, FindSubstringAtStart)
{
  static_string<32> str("Hello World");
  size_t pos = str.find("Hello");
  EXPECT_EQ(0, pos);
}

TEST_F(StaticStringTest, FindSubstringNotFound)
{
  static_string<32> str("Hello");
  size_t pos = str.find("world");
  EXPECT_EQ(static_string<32>::npos, pos);
}

TEST_F(StaticStringTest, FindEmptyString)
{
  static_string<32> str("Hello");
  size_t pos = str.find("");
  EXPECT_EQ(0, pos);
}

// ============================================================================
// RFIND TESTS
// ============================================================================

TEST_F(StaticStringTest, RFindCharacter)
{
  static_string<32> str("abcabc");
  size_t pos = str.rfind('a');
  EXPECT_EQ(3, pos);
}

TEST_F(StaticStringTest, RFindCharacterAtEnd)
{
  static_string<32> str("Hello World");
  size_t pos = str.rfind('d');
  EXPECT_EQ(10, pos);
}

TEST_F(StaticStringTest, RFindCharacterNotFound)
{
  static_string<32> str("Hello");
  size_t pos = str.rfind('x');
  EXPECT_EQ(static_string<32>::npos, pos);
}

// ============================================================================
// STARTS_WITH AND ENDS_WITH TESTS
// ============================================================================

TEST_F(StaticStringTest, StartsWithCString)
{
  static_string<32> str("Hello World");
  EXPECT_TRUE(str.starts_with("Hello"));
}

TEST_F(StaticStringTest, StartsWithCharacter)
{
  static_string<32> str("Hello World");
  EXPECT_TRUE(str.starts_with('H'));
}

TEST_F(StaticStringTest, StartsWithFalse)
{
  static_string<32> str("Hello World");
  EXPECT_FALSE(str.starts_with("World"));
}

TEST_F(StaticStringTest, EndsWithCString)
{
  static_string<32> str("Hello World");
  EXPECT_TRUE(str.ends_with("World"));
}

TEST_F(StaticStringTest, EndsWithCharacter)
{
  static_string<32> str("Hello World");
  EXPECT_TRUE(str.ends_with('d'));
}

TEST_F(StaticStringTest, EndsWithFalse)
{
  static_string<32> str("Hello World");
  EXPECT_FALSE(str.ends_with("Hello"));
}

// ============================================================================
// COMPARE TESTS
// ============================================================================

TEST_F(StaticStringTest, CompareEqual)
{
  static_string<32> str1("apple");
  static_string<32> str2("apple");
  EXPECT_EQ(0, str1.compare(str2));
}

TEST_F(StaticStringTest, CompareLessThan)
{
  static_string<32> str1("apple");
  static_string<32> str2("apricot");
  EXPECT_LT(str1.compare(str2), 0);
}

TEST_F(StaticStringTest, CompareGreaterThan)
{
  static_string<32> str1("apricot");
  static_string<32> str2("apple");
  EXPECT_GT(str1.compare(str2), 0);
}

TEST_F(StaticStringTest, CompareCString)
{
  static_string<32> str("Hello");
  EXPECT_EQ(0, str.compare("Hello"));
}

// ============================================================================
// COMPARISON OPERATORS TESTS
// ============================================================================

TEST_F(StaticStringTest, OperatorEqual)
{
  static_string<32> str1("apple");
  static_string<32> str2("apple");
  EXPECT_TRUE(str1 == str2);
}

TEST_F(StaticStringTest, OperatorEqualCString)
{
  static_string<32> str("apple");
  EXPECT_TRUE(str == "apple");
}

TEST_F(StaticStringTest, OperatorNotEqual)
{
  static_string<32> str1("apple");
  static_string<32> str2("banana");
  EXPECT_TRUE(str1 != str2);
}

TEST_F(StaticStringTest, OperatorLessThan)
{
  static_string<32> str1("apple");
  static_string<32> str2("apricot");
  EXPECT_TRUE(str1 < str2);
}

TEST_F(StaticStringTest, OperatorLessThanOrEqual)
{
  static_string<32> str1("apple");
  static_string<32> str2("apple");
  EXPECT_TRUE(str1 <= str2);
}

TEST_F(StaticStringTest, OperatorGreaterThan)
{
  static_string<32> str1("apricot");
  static_string<32> str2("apple");
  EXPECT_TRUE(str1 > str2);
}

TEST_F(StaticStringTest, OperatorGreaterThanOrEqual)
{
  static_string<32> str1("apple");
  static_string<32> str2("apple");
  EXPECT_TRUE(str1 >= str2);
}

// ============================================================================
// SUBSTR TESTS
// ============================================================================

TEST_F(StaticStringTest, Substr)
{
  static_string<32> str("Hello World");
  auto sub = str.substr(6, 5);
  EXPECT_STREQ("World", sub.c_str());
}

TEST_F(StaticStringTest, SubstrFromStart)
{
  static_string<32> str("Hello World");
  auto sub = str.substr(0, 5);
  EXPECT_STREQ("Hello", sub.c_str());
}

TEST_F(StaticStringTest, SubstrFromPosition)
{
  static_string<32> str("Hello World");
  auto sub = str.substr(6);
  EXPECT_STREQ("World", sub.c_str());
}

// ============================================================================
// ERASE TESTS
// ============================================================================

TEST_F(StaticStringTest, EraseByIterator)
{
  static_string<32> str("Hello");
  str.erase(str.begin() + 1);
  EXPECT_STREQ("Hllo", str.c_str());
}

TEST_F(StaticStringTest, EraseRange)
{
  static_string<32> str("Hello World");
  str.erase(str.begin() + 5, str.begin() + 6);
  EXPECT_STREQ("HelloWorld", str.c_str());
}

TEST_F(StaticStringTest, EraseByPosition)
{
  static_string<32> str("Hello World");
  str.erase(5, 6);
  EXPECT_STREQ("Hello", str.c_str());
}

// ============================================================================
// INSERT TESTS
// ============================================================================

TEST_F(StaticStringTest, InsertCharacter)
{
  static_string<32> str("Hllo");
  str.insert(str.begin() + 1, 'e');
  EXPECT_STREQ("Hello", str.c_str());
}

TEST_F(StaticStringTest, InsertString)
{
  static_string<32> str("Hello");
  str.insert(5, " World");
  EXPECT_STREQ("Hello World", str.c_str());
}

TEST_F(StaticStringTest, InsertAtStart)
{
  static_string<32> str("World");
  str.insert(0, "Hello ");
  EXPECT_STREQ("Hello World", str.c_str());
}

// ============================================================================
// REPLACE TESTS
// ============================================================================

TEST_F(StaticStringTest, Replace)
{
  static_string<32> str("Hello World");
  str.replace(6, 5, "C++");
  EXPECT_STREQ("Hello C++", str.c_str());
}

TEST_F(StaticStringTest, ReplaceAtStart)
{
  static_string<32> str("Hello World");
  str.replace(0, 5, "Hi");
  EXPECT_STREQ("Hi World", str.c_str());
}

// ============================================================================
// OPERATOR+ TESTS
// ============================================================================

TEST_F(StaticStringTest, OperatorPlusStrings)
{
  static_string<16> str1("Hello");
  static_string<16> str2(" World");
  auto result = str1 + str2;
  EXPECT_STREQ("Hello World", result.c_str());
}

TEST_F(StaticStringTest, OperatorPlusStringAndCString)
{
  static_string<16> str("Hello");
  auto result = str + " World";
  EXPECT_STREQ("Hello World", result.c_str());
}

// ============================================================================
// ITERATOR TESTS
// ============================================================================

TEST_F(StaticStringTest, ForwardIteration)
{
  static_string<32> str("ABC");
  int count = 0;
  for (auto it = str.begin(); it != str.end(); ++it)
  {
    count++;
  }
  EXPECT_EQ(3, count);
}

TEST_F(StaticStringTest, ConstIteration)
{
  const static_string<32> str("ABC");
  auto it = str.cbegin();
  EXPECT_EQ('A', *it);
}

TEST_F(StaticStringTest, ReverseIteration)
{
  static_string<32> str("ABC");
  auto rit = str.rbegin();
  EXPECT_EQ('C', *rit);
}

TEST_F(StaticStringTest, RangeBasedFor)
{
  static_string<32> str("ABC");
  int count = 0;
  for (char c: str)
  {
    EXPECT_NE(c, 0);
    count++;
  }
  EXPECT_EQ(3, count);
}

// ============================================================================
// CAPACITY TESTS
// ============================================================================

TEST_F(StaticStringTest, Size)
{
  static_string<32> str("Hello");
  EXPECT_EQ(5, str.size());
}

TEST_F(StaticStringTest, Length)
{
  static_string<32> str("Hello");
  EXPECT_EQ(5, str.length());
}

TEST_F(StaticStringTest, Capacity)
{
  static_string<32> str("Hello");
  EXPECT_EQ(32, str.capacity());
}

TEST_F(StaticStringTest, MaxSize)
{
  static_string<32> str("Hello");
  EXPECT_EQ(32, str.max_size());
}

TEST_F(StaticStringTest, EmptyTrue)
{
  static_string<32> str;
  EXPECT_TRUE(str.empty());
}

TEST_F(StaticStringTest, EmptyFalse)
{
  static_string<32> str("Hello");
  EXPECT_FALSE(str.empty());
}

TEST_F(StaticStringTest, IsFullTrue)
{
  static_string<5> str("Hello");
  EXPECT_TRUE(str.is_full());
}

TEST_F(StaticStringTest, IsFullFalse)
{
  static_string<32> str("Hello");
  EXPECT_FALSE(str.is_full());
}

// ============================================================================
// CONSTEXPR TESTS
// ============================================================================

TEST_F(StaticStringTest, ConstexprConstruction)
{
  constexpr static_string<32> msg("Hello");
  EXPECT_EQ(5, msg.size());
}

TEST_F(StaticStringTest, ConstexprComparison)
{
  constexpr static_string<32> msg("Hello");
  constexpr bool equal = (msg == "Hello");
  EXPECT_TRUE(equal);
}

TEST_F(StaticStringTest, ConstexprFind)
{
  constexpr static_string<32> str("Hello World");
  constexpr size_t pos = str.find('o');
  EXPECT_EQ(4, pos);
}

// ============================================================================
// OVERFLOW HANDLING TESTS
// ============================================================================

TEST_F(StaticStringTest, OverflowSilent)
{
  static_string<3> str;
  str.push_back('A');
  str.push_back('B');
  str.push_back('C');
  str.push_back('D'); // Silently ignored
  EXPECT_EQ(3, str.size());
  EXPECT_STREQ("ABC", str.c_str());
}

TEST_F(StaticStringTest, OverflowNoCorruption)
{
  static_string<3> str;
  str = "ABC";
  str.push_back('D');
  EXPECT_EQ(3, str.size());
  EXPECT_STREQ("ABC", str.c_str());
}

TEST_F(StaticStringTest, OverflowDetection)
{
  static_string<3> str("ABC");
  EXPECT_TRUE(str.is_full());
}

// ============================================================================
// CROSS-SIZE TESTS
// ============================================================================

TEST_F(StaticStringTest, CrossSizeConstruction)
{
  static_string<16> small("Hello");
  static_string<64> large(small);
  EXPECT_STREQ("Hello", large.c_str());
  EXPECT_EQ(small.size(), large.size());
}

TEST_F(StaticStringTest, CrossSizeConstructionWithTruncation)
{
  static_string<64> large("This is a very long string that exceeds capacity");
  static_string<16> small(large);
  EXPECT_EQ(16, small.size());
}

// ============================================================================
// EDGE CASES AND SPECIAL SCENARIOS
// ============================================================================

TEST_F(StaticStringTest, EmptyStringOperations)
{
  static_string<32> str;

  EXPECT_EQ(0, str.size());
  EXPECT_TRUE(str.empty());
  EXPECT_STREQ("", str.c_str());
  EXPECT_EQ(static_string<32>::npos, str.find('a'));
}

TEST_F(StaticStringTest, SingleCharacterString)
{
  static_string<32> str("A");

  EXPECT_EQ(1, str.size());
  EXPECT_EQ('A', str.front());
  EXPECT_EQ('A', str.back());
  EXPECT_TRUE(str.starts_with('A'));
  EXPECT_TRUE(str.ends_with('A'));
}

TEST_F(StaticStringTest, FullCapacityString)
{
  static_string<5> str("Hello");

  EXPECT_EQ(5, str.size());
  EXPECT_TRUE(str.is_full());
  EXPECT_FALSE(str.empty());
}

TEST_F(StaticStringTest, OperatorStreamOutput)
{
  static_string<32> str("Test");
  std::ostringstream oss;
  oss << str;
  EXPECT_EQ("Test", oss.str());
}

TEST_F(StaticStringTest, MultipleAppends)
{
  static_string<32> str;
  str.append("Hello");
  str.append(" ");
  str.append("beautiful");
  str.append(" ");
  str.append("world");
  EXPECT_STREQ("Hello beautiful world", str.c_str());
}

TEST_F(StaticStringTest, ModifyAfterClear)
{
  static_string<32> str("Original");
  str.clear();
  EXPECT_TRUE(str.empty());

  str = "New";
  EXPECT_STREQ("New", str.c_str());
}

TEST_F(StaticStringTest, FindFirstOfMultipleOccurrences)
{
  static_string<32> str("aaa");
  size_t pos = str.find('a');
  EXPECT_EQ(0, pos);
}

TEST_F(StaticStringTest, RFindLastOfMultipleOccurrences)
{
  static_string<32> str("aaa");
  size_t pos = str.rfind('a');
  EXPECT_EQ(2, pos);
}

} // namespace lampda::common
