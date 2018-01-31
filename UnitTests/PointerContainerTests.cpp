#include <GameLib/PointerContainer.h>

#include <vector>

#include "gtest/gtest.h"

struct TestElement
{
	size_t const Id;
	bool IsDeleted;

	static size_t DeletionCount;

	explicit TestElement(size_t id)
		: Id(id)
		, IsDeleted(false)
	{}

	~TestElement()
	{
		++DeletionCount;
	}
};

size_t TestElement::DeletionCount;

size_t GetSum(PointerContainer<TestElement> const & pc)
{
	size_t sum = 0;
	for (auto const & elem : pc)
	{
		sum += elem.Id;
	}

	return sum;
}

TEST(PointerContainerTests, ConstructsFromVector)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));
	input.push_back(new TestElement(4));

	PointerContainer<TestElement> pc(std::move(input));

    EXPECT_EQ(4U, pc.size());
}

TEST(PointerContainerTests, IteratesElements)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> pc(std::move(input));

	auto it = pc.begin();

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(1, it->Id);
	EXPECT_EQ(1, (*it).Id);

	++it;

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(2, it->Id);
	EXPECT_EQ(2, (*it).Id);

	++it;

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(3, it->Id);
	EXPECT_EQ(3, (*it).Id);

	++it;

	EXPECT_EQ(it, pc.end());
}

TEST(PointerContainerTests, IteratesElements_Const)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> const pc(std::move(input));

	auto it = pc.begin();

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(1, it->Id);
	EXPECT_EQ(1, (*it).Id);

	++it;

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(2, it->Id);
	EXPECT_EQ(2, (*it).Id);

	++it;

	EXPECT_NE(it, pc.end());
	EXPECT_EQ(3, it->Id);
	EXPECT_EQ(3, (*it).Id);

	++it;

	EXPECT_EQ(it, pc.end());
}

TEST(PointerContainerTests, IteratesElements_ForLoop)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> pc(std::move(input));

	size_t sum = 0;
	for (auto it : pc)
	{
		sum += it.Id;
	}

	EXPECT_EQ(6u, sum);
}

TEST(PointerContainerTests, IteratesElements_ForLoop_Const)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> const pc(std::move(input));

	size_t sum = 0;
	for (auto it : pc)
	{
		sum += it.Id;
	}

	EXPECT_EQ(6u, sum);
}

TEST(PointerContainerTests, IteratesElements_Index)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> pc(std::move(input));

	size_t sum = 0;
	for (size_t i = 0; i < pc.size(); ++i)
	{
		sum += pc[i]->Id;
	}

	EXPECT_EQ(6u, sum);
}

TEST(PointerContainerTests, IteratesElements_Index_Const)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	PointerContainer<TestElement> const pc(std::move(input));

	size_t sum = 0;
	for (size_t i = 0; i < pc.size(); ++i)
	{
		sum += pc[i]->Id;
	}

	EXPECT_EQ(6u, sum);
}

TEST(PointerContainerTests, FreesPointersOnDestructor)
{
	TestElement::DeletionCount = 0;

	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));

	{
		PointerContainer<TestElement> pc(std::move(input));

		EXPECT_EQ(0u, TestElement::DeletionCount);
	}

	EXPECT_EQ(3u, TestElement::DeletionCount);
}

///////////////////////////////////////////
// ShrinkToFit
///////////////////////////////////////////

TEST(PointerContainerTests, ShrinkToFit_Empty)
{
	std::vector<TestElement *> input;
	PointerContainer<TestElement> pc(std::move(input));

	EXPECT_EQ(0U, pc.size());

	TestElement::DeletionCount = 0u;

	pc.shrink_to_fit();

	EXPECT_EQ(0U, pc.size());

	EXPECT_EQ(0u, TestElement::DeletionCount);
}

TEST(PointerContainerTests, ShrinkToFit_NoDeletions)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));
	input.push_back(new TestElement(4));

	PointerContainer<TestElement> pc(std::move(input));

	EXPECT_EQ(4U, pc.size());

	TestElement::DeletionCount = 0u;

	pc.shrink_to_fit();

	EXPECT_EQ(4U, pc.size());

	EXPECT_EQ(0u, TestElement::DeletionCount);
}

TEST(PointerContainerTests, ShrinkToFit_1Deletion)
{
	for (size_t i = 0; i < 4; ++i)
	{
		std::vector<TestElement *> input;
		input.push_back(new TestElement(1));
		input.push_back(new TestElement(2));
		input.push_back(new TestElement(3));
		input.push_back(new TestElement(4));

		PointerContainer<TestElement> pc(std::move(input));

		EXPECT_EQ(4U, pc.size());

		TestElement::DeletionCount = 0u;

		pc[i]->IsDeleted = true;

		pc.shrink_to_fit();

		EXPECT_EQ(3U, pc.size());

		EXPECT_EQ(1u, TestElement::DeletionCount);

		EXPECT_EQ(10 - (i + 1), GetSum(pc));
	}
}

TEST(PointerContainerTests, ShrinkToFit_2Deletions_Contiguous)
{
	for (size_t i = 0; i < 3; ++i)
	{
		std::vector<TestElement *> input;
		input.push_back(new TestElement(1));
		input.push_back(new TestElement(2));
		input.push_back(new TestElement(3));
		input.push_back(new TestElement(4));

		PointerContainer<TestElement> pc(std::move(input));

		EXPECT_EQ(4U, pc.size());

		TestElement::DeletionCount = 0u;

		pc[i]->IsDeleted = true;
		pc[i + 1]->IsDeleted = true;

		pc.shrink_to_fit();

		EXPECT_EQ(2U, pc.size());

		EXPECT_EQ(2U, TestElement::DeletionCount);

		EXPECT_EQ(10 - ((i + 1) + (i + 1 + 1)), GetSum(pc));
	}
}

TEST(PointerContainerTests, ShrinkToFit_2Deletions_SpacedByOne)
{
	for (size_t i = 0; i < 2; ++i)
	{
		std::vector<TestElement *> input;
		input.push_back(new TestElement(1));
		input.push_back(new TestElement(2));
		input.push_back(new TestElement(3));
		input.push_back(new TestElement(4));

		PointerContainer<TestElement> pc(std::move(input));

		EXPECT_EQ(4U, pc.size());

		TestElement::DeletionCount = 0u;

		pc[i]->IsDeleted = true;
		pc[i + 2]->IsDeleted = true;

		pc.shrink_to_fit();

		EXPECT_EQ(2U, pc.size());

		EXPECT_EQ(2U, TestElement::DeletionCount);

		EXPECT_EQ(10 - ((i + 1) + (i + 2 + 1)), GetSum(pc));
	}
}

TEST(PointerContainerTests, ShrinkToFit_2Deletions_SpacedByTwo)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));
	input.push_back(new TestElement(4));

	PointerContainer<TestElement> pc(std::move(input));

	EXPECT_EQ(4U, pc.size());

	TestElement::DeletionCount = 0u;

	pc[0]->IsDeleted = true;
	pc[3]->IsDeleted = true;

	pc.shrink_to_fit();

	EXPECT_EQ(2U, pc.size());

	EXPECT_EQ(2U, TestElement::DeletionCount);

	EXPECT_EQ(5u, GetSum(pc));
}

TEST(PointerContainerTests, ShrinkToFit_3Deletions_Contiguous)
{
	for (size_t i = 0; i < 2; ++i)
	{
		std::vector<TestElement *> input;
		input.push_back(new TestElement(1));
		input.push_back(new TestElement(2));
		input.push_back(new TestElement(3));
		input.push_back(new TestElement(4));

		PointerContainer<TestElement> pc(std::move(input));

		EXPECT_EQ(4U, pc.size());

		TestElement::DeletionCount = 0u;

		pc[i]->IsDeleted = true;
		pc[i + 1]->IsDeleted = true;
		pc[i + 2]->IsDeleted = true;

		pc.shrink_to_fit();

		EXPECT_EQ(1U, pc.size());

		EXPECT_EQ(3U, TestElement::DeletionCount);

		EXPECT_EQ(10 - ((i + 1) + (i + 1 + 1) + (i + 2 + 1)), GetSum(pc));
	}
}

TEST(PointerContainerTests, ShrinkToFit_AllDeleted)
{
	std::vector<TestElement *> input;
	input.push_back(new TestElement(1));
	input.push_back(new TestElement(2));
	input.push_back(new TestElement(3));
	input.push_back(new TestElement(4));

	PointerContainer<TestElement> pc(std::move(input));

	EXPECT_EQ(4U, pc.size());

	TestElement::DeletionCount = 0u;

	pc[0]->IsDeleted = true;
	pc[1]->IsDeleted = true;
	pc[2]->IsDeleted = true;
	pc[3]->IsDeleted = true;

	pc.shrink_to_fit();

	EXPECT_EQ(0U, pc.size());

	EXPECT_EQ(4U, TestElement::DeletionCount);
}