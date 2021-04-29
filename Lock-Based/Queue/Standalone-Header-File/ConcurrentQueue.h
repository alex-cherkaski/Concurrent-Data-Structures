#pragma once
#include <memory>
#include <mutex>

template <typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue();
	~ConcurrentQueue();

	ConcurrentQueue(const ConcurrentQueue<T>& other) = delete;
	ConcurrentQueue<T>& operator=(const ConcurrentQueue<T>& other) = delete;

	ConcurrentQueue(ConcurrentQueue<T>&& other) = delete;
	ConcurrentQueue<T>& operator=(ConcurrentQueue<T>&& other) = delete;

	void Push(const T& value);

	std::shared_ptr<T> TryPop();
	bool TryPop(T& result);

	std::shared_ptr<T> WaitAndPop();
	bool WaitAndPop(T& result);

	bool Empty() const;

private:
	// The ConcurrentQueue is implemented using a unidirectional list of nodes.
	struct Node
	{
		Node() : data(nullptr), next(nullptr) {}
		Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}

		std::shared_ptr<T> data;
		Node* next;
	};

private:
	Node* m_head;
	Node* m_tail;

	mutable std::mutex m_headMutex;
	mutable std::mutex m_tailMutex;

	std::condition_variable m_notEmptyCondition;
};

template<typename T>
inline ConcurrentQueue<T>::ConcurrentQueue() : m_head(new Node()), m_tail(m_head)
{
}

template<typename T>
inline ConcurrentQueue<T>::~ConcurrentQueue()
{
	while (m_head != nullptr)
	{
		Node* currentNode = m_head;
		m_head = m_head->next;
		delete currentNode;
	}
}

template<typename T>
inline void ConcurrentQueue<T>::Push(const T& value)
{
	std::lock_guard<std::mutex> lock(m_tailMutex);

	// Append the new node to the end of the queue.
	m_tail->next = new Node(value);

	// Swap data values with the dummy node moving the dummy node to the end of the queue.
	std::swap(m_tail->data, m_tail->next->data);

	// Update tail to point to the new dummy node.
	m_tail = m_tail->next;

	m_notEmptyCondition.notify_all();
}

template<typename T>
inline std::shared_ptr<T> ConcurrentQueue<T>::TryPop()
{
	std::unique_lock<std::mutex> tailLock(m_tailMutex);
	std::unique_lock<std::mutex> headLock(m_headMutex);

	// If only the dummy node is present than the queue is empty.
	if (m_head == m_tail)
	{
		return std::shared_ptr<T>();
	}

	// Tail is not examined beyond this point.
	tailLock.unlock();

	std::unique_ptr<Node> dequedNodePtr(m_head);
	m_head = m_head->next;

	// Head pointer not examined beyond this point.
	headLock.unlock();

	// --- From this point on multiple threads can delete their old nodes and return their data safely. ---

	std::shared_ptr<T> dataPtr = dequedNodePtr->data;
	return dataPtr;
}

template<typename T>
inline bool ConcurrentQueue<T>::TryPop(T& result)
{
	std::unique_lock<std::mutex> tailLock(m_tailMutex);
	std::unique_lock<std::mutex> headLock(m_headMutex);

	// If head and tail both point to the empty node the deque was unsuccessful.
	if (m_head == m_tail)
	{
		return false;
	}

	// The tail pointer is not examine beyond this point.
	tailLock.unlock();

	std::unique_ptr<Node> dequedNodePtr(m_head);
	m_head = m_head->next;

	// New head node not examined beyond this point.
	headLock.unlock();

	// --- Multiple threads can safely delete the dequed node and copy its data beyond this point. ---

	result = *(dequedNodePtr->data.get());
	return true;
}

template<typename T>
inline std::shared_ptr<T> ConcurrentQueue<T>::WaitAndPop()
{
	std::unique_lock<std::mutex> headLock(m_headMutex);

	auto notEmptyPredicate = [&]() -> bool
	{
		std::lock_guard<std::mutex> tailLock(m_tailMutex);
		return m_head != m_tail;
	};

	// --- The tail lock is not needed beyond the above predicate. ---

	m_notEmptyCondition.wait(headLock, notEmptyPredicate);

	std::unique_ptr<Node> dequedNodePtr(m_head);
	std::shared_ptr<T> dataPtr = dequedNodePtr->data;
	m_head = m_head->next;

	// --- The head lock is not required beyond this point. ---
	headLock.unlock();

	// Multiple threads are now able to return their data and delete their front node.
	return dataPtr;
}

template<typename T>
inline bool ConcurrentQueue<T>::WaitAndPop(T& result)
{
	std::unique_lock<std::mutex> headLock(m_headMutex);

	auto notEmptyPredicate = [&]() -> bool
	{
		std::lock_guard<std::mutex> tailLock(m_tailMutex);
		return m_head != m_tail;
	};

	// --- tail is not accessed outside of the predicate. ---

	m_notEmptyCondition.wait(headLock, notEmptyPredicate);

	std::unique_ptr<Node> dequedNodePtr(m_head);
	m_head = m_head->next;

	headLock.unlock();

	// --- No further modifications to the head pointer past this point. ---

	// Multiple threads are safe to copy the data and delete the dequed node.
	result = *(dequedNodePtr->data);
	return true;
}

template<typename T>
inline bool ConcurrentQueue<T>::Empty() const
{
	std::lock_guard<std::mutex> headLock(m_headMutex);
	std::lock_guard<std::mutex> tailLock(m_tailMutex);
	return m_head == m_tail;
}
