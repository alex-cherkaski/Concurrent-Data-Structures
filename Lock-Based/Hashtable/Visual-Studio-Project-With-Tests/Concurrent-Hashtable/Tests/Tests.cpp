#include "pch.h"
#include "CppUnitTest.h"
#include "../Concurrent-Hashtable/Source/ConcurrentHashtable.h"
#include <thread>
#include <vector>
#include <future>

ConcurrentHashtable<int, int> g_concurrentHashtable;
std::vector<std::thread> g_threads;

auto InsertKeyValuePair = [](ConcurrentHashtable<int, int>& concurrentHashtable, int key, int value) -> void
{
	concurrentHashtable.SetValueForKey(key, value);
};

auto GetValueForKey = [](ConcurrentHashtable<int, int>& concurrentHashtable, int key) -> std::shared_ptr<int>
{
	return concurrentHashtable.GetValueForKey(key);
};

auto RemoveEntry = [](ConcurrentHashtable<int, int>& concurrentHashtable, int key) -> void
{
	concurrentHashtable.RemoveEntry(key);
};

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(Tests)
	{
	public:
		TEST_METHOD_CLEANUP(Clean)
		{
			g_concurrentHashtable.Clear();
			g_threads.clear();
		}

		TEST_METHOD(SetGetMethodsTest)
		{
			size_t numIterations = 25;
			std::vector<std::pair<int, int>> insertedPairs;
			std::vector<std::future<std::shared_ptr<int>>> retrievedValueFutures;

			// Launch threads that will insert and remove values from the table.
			for (size_t i = 0; i < numIterations; ++i)
			{
				insertedPairs.emplace_back(i, i);
				g_threads.push_back(std::move(std::thread(InsertKeyValuePair, std::ref(g_concurrentHashtable), i, i)));
				retrievedValueFutures.push_back(std::move(std::async(GetValueForKey, std::ref(g_concurrentHashtable), i)));
			}

			// Wait for all inserting threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Wait for each future to finsih and if the pointer is not empty assert that its value is in 'insertedPairs' and remove it.
			for (std::future<std::shared_ptr<int>>& future : retrievedValueFutures)
			{
				std::shared_ptr<int> integerPointer = future.get();
				if (integerPointer != nullptr)
				{
					int integer = *integerPointer;
					
					auto iterator = std::find_if(insertedPairs.begin(), insertedPairs.end(),
						[&](const std::pair<int, int>& pair) -> bool { return pair.second == integer; });

					Assert::IsTrue(iterator != insertedPairs.end());

					insertedPairs.erase(iterator);
				}
			}
		}

		TEST_METHOD(SetRemoveMethodsTest)
		{
			size_t numIterations = 25;
			std::vector<std::pair<int, int>> insertedPairs;
			std::vector<std::future<std::shared_ptr<int>>> removedValues;

			// Launch threads that will insert values from the table.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(InsertKeyValuePair, std::ref(g_concurrentHashtable), i, i)));
				insertedPairs.emplace_back(i, i);
			}

			// Wait for inserting threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			g_threads.clear();

			// Launch threads to remove entries from the table.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(RemoveEntry, std::ref(g_concurrentHashtable), i)));
			}

			// Wait for deleting threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Assert that each key returns an empty shared pointer, implying that there is no entry for this key in the table.
			for (size_t i = 0; i < numIterations; ++i)
			{
				std::shared_ptr<int> pointer = g_concurrentHashtable.GetValueForKey(i);
				Assert::IsTrue(pointer == nullptr);
			}
		}

		TEST_METHOD(SetRemoveGetMethodTests)
		{
			size_t numiterations = 25;
			std::vector<std::pair<int, int>> insertedPairs;
			std::vector<std::future<std::shared_ptr<int>>> retrievedValues;

			// Launch threads to insert, remove, and retrieve values from the table.
			for (size_t i = 0; i < numiterations; ++i)
			{
				
				g_threads.push_back(std::move(std::thread(InsertKeyValuePair, std::ref(g_concurrentHashtable), i, i)));
				insertedPairs.emplace_back(i, i);
				retrievedValues.push_back(std::move(std::async(GetValueForKey, std::ref(g_concurrentHashtable), i)));
				g_threads.push_back(std::move(std::thread(RemoveEntry, std::ref(g_concurrentHashtable), i)));
			}

			// Wait for all inserting and removing threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [&](std::thread& thread) -> void { thread.join(); });

			// Wait for all futures to have a value.
			// For each future with a non-empty shared pointer assert that the value pointed to is in 'insertedPairs' and remove it.
			for (std::future<std::shared_ptr<int>>& future : retrievedValues)
			{
				std::shared_ptr<int> integerPointer = future.get();
				if (integerPointer != nullptr)
				{
					int integer = *integerPointer;

					auto iterator = std::find_if(insertedPairs.begin(), insertedPairs.end(),
						[&](const std::pair<int, int>& pair) -> bool { return pair.second == integer; });

					Assert::IsTrue(iterator != insertedPairs.end());

					insertedPairs.erase(iterator);
				}
			}
		}
	};
}
