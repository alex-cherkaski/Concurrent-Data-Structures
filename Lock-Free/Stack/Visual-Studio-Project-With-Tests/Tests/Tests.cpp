#include "pch.h"
#include "CppUnitTest.h"
#include "../Lock-Free-Stack/Source/LockFreeStack.h"
#include <vector>
#include <thread>
#include <algorithm>
#include <memory>
#include <future>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

LockFreeStack<int> g_stack;
std::vector<std::thread> g_threads;

auto PushToStack = [&](int i) -> void { g_stack.Push(i); };
auto PopFromStack = [&]() -> std::shared_ptr<int> { return g_stack.Pop(); };

namespace Tests
{
	TEST_CLASS(Tests)
	{
	public:
		TEST_METHOD_CLEANUP(Cleanup)
		{
			g_threads.clear();
		}

		TEST_METHOD(PushThenPopMethodsTest)
		{
			size_t numIterations = 10;
			std::vector<int> integersPushed;
			std::vector<std::future<std::shared_ptr<int>>> valuesPoped;

			// Launch all threads that will push onto the stack.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToStack, i)));
				integersPushed.push_back(i);
			}

			// Wait for all pushing threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });
			
			// Launch all threads that will pop onto from the stack.
			for (size_t i = 0; i < numIterations; ++i)
			{
				valuesPoped.push_back(std::move(std::async(PopFromStack)));
			}

			// Wait for all futures to have a value.
			// Assert that each non empty value in a future is in integersPushed and remove it.
			for (std::future<std::shared_ptr<int>>& future : valuesPoped)
			{
				std::shared_ptr<int> integerPtr = future.get();
				if (integerPtr != nullptr)
				{
					int integer = *(integerPtr);
					auto iterator = std::find(integersPushed.begin(), integersPushed.end(), integer);
					Assert::IsTrue(iterator != integersPushed.end());
					integersPushed.erase(iterator);
				}
			}
		}

		TEST_METHOD(PushAndPopMethodsTest)
		{
			size_t numIterations = 10;
			std::vector<int> integersPushed;
			std::vector<std::future<std::shared_ptr<int>>> valuesPoped;

			// Launch all threads that will push and pop from the stack.
			for (size_t i = 0; i < numIterations; ++i)
			{
				g_threads.push_back(std::move(std::thread(PushToStack, i)));
				integersPushed.push_back(i);
				valuesPoped.push_back(std::move(std::async(PopFromStack)));
			}

			// Wait for all pushing threads to finish.
			std::for_each(g_threads.begin(), g_threads.end(), [](std::thread& thread) -> void { thread.join(); });

			// Wait for all futures to have a value.
			// Assert that each non empty value in a future is in integersPushed and remove it.
			for (std::future<std::shared_ptr<int>>& future : valuesPoped)
			{
				std::shared_ptr<int> integerPtr = future.get();
				if (integerPtr != nullptr)
				{
					int integer = *(integerPtr);
					auto iterator = std::find(integersPushed.begin(), integersPushed.end(), integer);
					Assert::IsTrue(iterator != integersPushed.end());
					integersPushed.erase(iterator);
				}
			}
		}
	};
}
