#pragma once
#include <atomic>
#include <memory>

template<typename T>
class LockFreeStack
{
public:
	LockFreeStack();
	~LockFreeStack();

	// Copy semantics.
	LockFreeStack(const LockFreeStack<T>& other) = delete;
	LockFreeStack<T>& operator=(const LockFreeStack<T>& other) = delete;

	// Move semantics.
	LockFreeStack(LockFreeStack<T>&& other) = delete;
	LockFreeStack<T>& operator=(LockFreeStack<T>&& other) = delete;

	void Push(const T& data);
	std::shared_ptr<T> Pop();

private:
	struct Node
	{
		std::shared_ptr<T> data;
		Node* next;

		Node(const T& data) : data(std::make_shared<T>(data)), next(nullptr) {}
	};

	std::atomic<Node*> m_head;
};

template<typename T>
inline LockFreeStack<T>::LockFreeStack() : m_head(nullptr)
{
}

template<typename T>
inline LockFreeStack<T>::~LockFreeStack()
{
	while (m_head != nullptr)
	{
		Node* nodeToDelete = m_head;
		m_head.store(m_head.load()->next);
		delete nodeToDelete;
	}
}

template<typename T>
inline void LockFreeStack<T>::Push(const T& data)
{
	Node* newNode = new Node(data);
	newNode->next = m_head;

	// If the exchange fails newNode->next pointer will be updated to the new head.
	// If the exchange succeeds the head will point to the new node.
	while (!m_head.compare_exchange_weak(newNode->next, newNode, std::memory_order_seq_cst, std::memory_order_relaxed));
}

template<typename T>
inline std::shared_ptr<T> LockFreeStack<T>::Pop()
{
	// Move the head node to poin to the next node on the stack.
	Node* nodeToPop = m_head.load();
	while (nodeToPop != nullptr && !m_head.compare_exchange_weak(nodeToPop, nodeToPop->next));

	std::shared_ptr<T> dataPtr(nullptr);
	if (nodeToPop != nullptr)
	{
		dataPtr.swap(nodeToPop->data);
		delete nodeToPop;
	}

	return (dataPtr == nullptr) ? std::shared_ptr<T>() : dataPtr;
}
