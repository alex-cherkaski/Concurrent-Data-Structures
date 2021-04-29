#include "pch.h"
#include "CppUnitTest.h"
#include "../Concurrent-Queue/Source/ConcurrentQueue.h"
#include <vector>
#include <thread>
#include <future>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

ConcurrentQueue<int> g_concurrentQueue;
std::vector<std::thread> g_threads;

auto PushToConcurrentQueue = [](ConcurrentQueue<int>& concurrentQueue, int value) -> void { concurrentQueue.Push(value); };
auto PopPtrFromConcurrentQueue = [](ConcurrentQueue<int>& concurrentQueue) -> std::shared_ptr<int> { return concurrentQueue.TryPop(); };
auto PopRefFromConcurrentQueue = [](ConcurrentQueue<int>& concurrentQueue, int& result) -> bool { return concurrentQueue.TryPop(result); };
auto WaitPopPtrFromConcurrentQueue = [](ConcurrentQueue<int>& concurrentQueue) -> std::shared_ptr<int> { return concurrentQueue.WaitAndPop(); };
auto WaitPopRefFromConcurrentQueue = [](ConcurrentQueue<int>& concurrentQueue, int& result) -> bool { return concurrentQueue.WaitAndPop(result); };

namespace Tests
{
	TEST_CLASS(Tests)
	{
	public:

		TEST_METHOD_CLEANUP(Cleanup)
		{
			while (!g_concurrentQueue.Empty())
			{
				g_concurrentQueue.TryPop();
			}

			g_threads.clear();
		}

		TEST_METHOD(PushTryPopPtrMethod)
		{
			size_t numIterations = 10;

			std::vector<int> integersPushed;
			std::vector<std::future<std::shared_ptr<int>>> futures;

			// Launch all threads that will enque and deque from the queue.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToConcurrentQueue, std::ref(g_concurrentQueue), i)));
				futures.push_back(std::move(std::async(PopPtrFromConcurrentQueue, std::ref(g_concurrentQueue))));
				integersPushed.push_back(i);
			}

			// Wait for all threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [&](std::thread& thread) -> void { thread.join(); });

			// Assert that each integer that was dequed is in 'integersPushed' and remove it for the vector.
			for (std::future<std::shared_ptr<int>>& future : futures)
			{
				std::shared_ptr<int> integerPtr = future.get();
				if (integerPtr != nullptr)
				{
					int integer = *integerPtr;
					Assert::IsTrue(std::find(integersPushed.begin(), integersPushed.end(), integer) != integersPushed.end());
					integersPushed.erase(std::remove(integersPushed.begin(), integersPushed.end(), integer), integersPushed.end());
				}
			}
		}

		TEST_METHOD(PushTryPopRefMethod)
		{
			size_t numIterations = 10;

			std::vector<int> integersPushed;
			std::vector<std::future<bool>> futures;
			std::vector<int> integersPoped(numIterations);

			// Launch all threads that will enque and deque from the queue.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToConcurrentQueue, std::ref(g_concurrentQueue), i)));
				futures.push_back(std::move(std::async(PopRefFromConcurrentQueue, std::ref(g_concurrentQueue), std::ref(integersPoped[i]))));
				integersPushed.push_back(i);
			}

			// Wait for all push threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [&](std::thread& thread) -> void { thread.join(); });

			// Assert that each integer in 'integersPoped' that is associated with a valid future is also in 'integersPushed' and erase it.
			for (size_t i = 0; i < numIterations; ++i)
			{
				bool successfulDeque = futures[i].get();
				if (successfulDeque)
				{
					Assert::IsTrue(std::find(integersPushed.begin(), integersPushed.end(), integersPoped[i]) != integersPushed.end());
					integersPushed.erase(std::remove(integersPushed.begin(), integersPushed.end(), integersPoped[i]), integersPushed.end());
				}
			}
		}

		TEST_METHOD(PushWaitPopPtrMethod)
		{
			size_t numIterations = 10;

			std::vector<int> integersPushed;
			std::vector<std::future<std::shared_ptr<int>>> futures;

			// Launch all threads that will enque and deque from the queue.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToConcurrentQueue, std::ref(g_concurrentQueue), i)));
				futures.push_back(std::move(std::async(WaitPopPtrFromConcurrentQueue, std::ref(g_concurrentQueue))));
				integersPushed.push_back(i);
			}

			// Wait for pushing threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [&](std::thread& thread) -> void { thread.join(); });

			// Wait for all futures to get a value.
			// Assert that each dequed values is in 'integersPushed' and remove it.
			for (std::future<std::shared_ptr<int>>& future : futures)
			{
				int integer = *(future.get());
				Assert::IsTrue(std::find(integersPushed.begin(), integersPushed.end(), integer) != integersPushed.end());
				integersPushed.erase(std::remove(integersPushed.begin(), integersPushed.end(), integer), integersPushed.end());
			}
		}

		TEST_METHOD(PushWaitPopRefMethod)
		{
			size_t numIterations = 10;

			std::vector<int> integersPushed;
			std::vector<std::future<bool>> futures;
			std::vector<int> integersPoped(numIterations);

			// Launch all threads that will enque and deque from the queue.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToConcurrentQueue, std::ref(g_concurrentQueue), i)));
				futures.push_back(std::move(std::async(PopRefFromConcurrentQueue, std::ref(g_concurrentQueue), std::ref(integersPoped[i]))));
				integersPushed.push_back(i);
			}

			// Wait for all pushing threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [&](std::thread& thread) -> void { thread.join(); });

			// Wait for all futures to get a value;
			// Assert that all dequed integers are in the 'integersPushed' vector and remove each.
			for (size_t i = 0; i < numIterations; ++i)
			{
				bool successfulDeque = futures[i].get();
				if (successfulDeque)
				{
					Assert::IsTrue(std::find(integersPushed.begin(), integersPushed.end(), integersPoped[i]) != integersPushed.end());
					integersPushed.erase(std::remove(integersPushed.begin(), integersPushed.end(), integersPoped[i]), integersPushed.end());
				}
			}
		}
	};
}
