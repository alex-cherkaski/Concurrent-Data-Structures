#include "pch.h"
#include "CppUnitTest.h"
#include "../Concurrent-List/Source/ConcurrentList.h"
#include <thread>
#include <vector>
#include <future>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

ConcurrentList<int> g_concurrentList;
std::vector<std::thread> g_threads;

auto AppendToFront = [](int value) -> void { g_concurrentList.PushToFront(value); };
auto IsEven = [](int value) -> bool { return value % 2 == 0; };
auto IsOdd = [](int value) -> bool { return value % 2 != 0; };
auto RemoveIfEven = [&]() -> void { g_concurrentList.RemoveIf(IsEven); };
auto RemoveIfOdd = [&]() -> void { g_concurrentList.RemoveIf(IsOdd); };
auto DoubleInteger = [](int& i) -> void { i *= 2; };
auto DoubleListIntegers = [&]() -> void { g_concurrentList.ForEach(DoubleInteger); };

namespace Tests
{
	TEST_CLASS(Tests)
	{
	public:

		TEST_METHOD_CLEANUP(Cleanup)
		{
			// Clear all containers.
			g_concurrentList.RemoveIf([](const int& i) -> bool { return true; });
			g_threads.clear();
		}
		
		TEST_METHOD(PushFindMethodsTest)
		{
			size_t numIterations = 10;
			std::vector<int> integersAdded;
			std::vector<std::future<std::shared_ptr<int>>> foundValues;

			// Launch all threads that will add the list.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(AppendToFront, i)));
				integersAdded.push_back(i);
			}

			//std::async(&ConcurrentList<int>::FindFirstIf, &g_concurrentList, IsEven); doesn't work :(
			foundValues.push_back(std::move(std::async([]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsEven); })));
			foundValues.push_back(std::move(std::async([]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsOdd); })));

			// Wait for all appending threads to finsih.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Wait for each future to have a value.
			// Assert that the value of every non empty pointer is in 'integersAdded' and remove it.
			for (std::future<std::shared_ptr<int>>& future : foundValues)
			{
				std::shared_ptr<int> integerPtr = future.get();
				if (integerPtr != nullptr)
				{
					int integer = *integerPtr;
					auto iterator = std::find(integersAdded.begin(), integersAdded.end(), integer);
					Assert::IsTrue(iterator != integersAdded.end());
					integersAdded.erase(iterator);
				}
			}
		}

		TEST_METHOD(PushRemoveMethodsTest)
		{
			size_t numIterations = 10;
			std::vector<int> integersAdded;

			// Launch all threads that will add the list.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(AppendToFront, i)));
				integersAdded.push_back(i);
			}

			// Wait for all inserting threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });
			g_threads.clear();

			// Launch a thread to remove all even numbers.
			g_threads.push_back(std::move(std::thread(RemoveIfEven)));

			// Launch a thread to remove all odd numbers;
			g_threads.push_back(std::move(std::thread(RemoveIfOdd)));

			// Wait for all removing threads to finsih.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Assert that there are no even integers left in the list.
			std::future<std::shared_ptr<int>> evenIntegerFuture = std::async([&]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsEven); });
			std::shared_ptr<int> evenIntegerPtr = evenIntegerFuture.get();
			Assert::IsTrue(evenIntegerPtr == nullptr);

			// Assert that there are no odd integers left in the list.
			std::future<std::shared_ptr<int>> oddIntegerFuture = std::async([&]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsOdd); });
			std::shared_ptr<int> oddIntegerPtr = oddIntegerFuture.get();
			Assert::IsTrue(oddIntegerPtr == nullptr);
		}

		TEST_METHOD(PushForEachRemoveMethodsTest)
		{
			size_t numIterations = 10;
			std::vector<int> integersAdded;

			// Launch all threads that will add the list.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(AppendToFront, i)));
				integersAdded.push_back(i);
			}

			// Wait for all inserting threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });
			g_threads.clear();
			
			// Launch a thread that will double each integer the list.
			std::thread doublingThread = std::thread(DoubleListIntegers);
			doublingThread.join();

			// Launch a thread to remove all even numbers.
			g_threads.push_back(std::move(std::thread(RemoveIfEven)));

			// Wait for all removing threads to finsih.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Assert that there are no even integers left in the list.
			std::future<std::shared_ptr<int>> evenIntegerFuture = std::async([&]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsEven); });
			std::shared_ptr<int> evenIntegerPtr = evenIntegerFuture.get();
			Assert::IsTrue(evenIntegerPtr == nullptr);

			// Assert that there are no odd integers in the list.
			std::future<std::shared_ptr<int>> oddIntegerFuture = std::async([&]() -> std::shared_ptr<int> { return g_concurrentList.FindFirstIf(IsOdd); });
			std::shared_ptr<int> oddIntegerPtr = oddIntegerFuture.get();
			Assert::IsTrue(oddIntegerPtr == nullptr);
		}
	};
}
