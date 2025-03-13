#include <gtest/gtest.h>
#include "List.h"

class BiListTest : public ::testing::Test
{
  protected:
  void SetUp() override
  {
    list  = new BiList();
    node1 = new BiListNode();
    node2 = new BiListNode();
  }

  void TearDown() override {}

  BiList     *list;
  BiListNode *node1;
  BiListNode *node2;
};

TEST_F(BiListTest, InitiallyEmpty) { EXPECT_TRUE(list->empty()); }

TEST_F(BiListTest, AddNode)
{
  list->add(node1);
  EXPECT_FALSE(list->empty());
  EXPECT_EQ(list->size(), 1);
}

TEST_F(BiListTest, AddToTail)
{
  list->addToTail(node1);
  list->addToTail(node2);
  EXPECT_FALSE(list->empty());
  EXPECT_EQ(list->size(), 2);
}

TEST_F(BiListTest, DeleteNode)
{
  list->add(node1);
  list->del(node1);
  EXPECT_TRUE(list->empty());
  EXPECT_EQ(list->size(), 0);
}

TEST_F(BiListTest, MoveNode)
{
  BiList list2;
  list->add(node1);
  list->move(node1, list2);
  EXPECT_TRUE(list->empty());
  EXPECT_FALSE(list2.empty());
  EXPECT_EQ(list2.size(), 1);
}

TEST_F(BiListTest, MoveToTail)
{
  BiList list2;
  list->add(node1);
  list->moveToTail(node1, list2);
  EXPECT_TRUE(list->empty());
  EXPECT_FALSE(list2.empty());
  EXPECT_EQ(list2.size(), 1);
}

TEST_F(BiListTest, MergeLists)
{
  BiList list2;
  list->add(node1);
  list2.add(node2);
  list->merge(list2, true);
  EXPECT_FALSE(list->empty());
  EXPECT_TRUE(list2.empty());
  EXPECT_EQ(list->size(), 2);
}

TEST_F(BiListTest, ClearList)
{
  list->add(node1);
  list->add(node2);
  list->clear();
  EXPECT_TRUE(list->empty());
  EXPECT_EQ(list->size(), 0);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
