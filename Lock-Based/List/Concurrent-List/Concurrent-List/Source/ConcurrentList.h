#pragma once
#include <mutex>
#include <memory>
#include <functional>

template<typename T>
class ConcurrentList
{
public:
	ConcurrentList();
	~ConcurrentList();

	// Copy Semantics
	ConcurrentList(const ConcurrentList<T>& other) = delete;
	ConcurrentList<T>& operator=(const ConcurrentList<T>& other) = delete;

	// Move Semantics
	ConcurrentList(ConcurrentList<T>&& other) = delete;
	ConcurrentList<T>& operator=(ConcurrentList<T>&& other) = delete;

	// Append a new node to the front of the list.
	void PushToFront(const T& data);

	// Apply function to each piece of data on the list.
	template<typename TFunction> void ForEach(TFunction function);

	// Find the first occurance of 'data' in the list if it exists.
	// Return an empty shared pointer otherwise.
	template<typename TFunction> std::shared_ptr<T> FindFirstIf(TFunction predicate);

	// Remove all items in the list that satisfy the predicate.
	template<typename TFunction> void RemoveIf(TFunction predicate);

private:

	// The list consists of one or more of these nodes.
	struct Node
	{
		std::mutex mutex;
		std::shared_ptr<T> data;
		Node* next;

		Node() : data(std::shared_ptr<T>()), next(nullptr) {}
		Node(const T& data) : data(std::make_shared<T>(data)), next(nullptr) {}
	};

	Node* m_head;
};

template<typename T>
inline ConcurrentList<T>::ConcurrentList() : m_head(new Node)
{
}

template<typename T>
inline ConcurrentList<T>::~ConcurrentList()
{
	while (m_head != nullptr)
	{
		Node* previousNode = m_head;
		m_head = m_head->next;
		delete previousNode;
	}
}

template<typename T>
inline void ConcurrentList<T>::PushToFront(const T& data)
{
	Node* newNode = new Node(data);
	std::lock_guard<std::mutex> lock(m_head->mutex);
	newNode->next = m_head;
	m_head = newNode;
}

template<typename T>
template<typename TFunction>
inline void ConcurrentList<T>::ForEach(TFunction function)
{
	std::unique_lock<std::mutex> firstLock(m_head->mutex);
	Node* currentNode = m_head;

	// The empty shared pointer is the dummy node.
	while (currentNode->data != nullptr)
	{
		function(*(currentNode->data));
		std::unique_lock<std::mutex> secondLock(currentNode->next->mutex);
		currentNode = currentNode->next;
		firstLock = std::move(secondLock);
	}
}

template<typename T>
template<typename TFunction>
inline std::shared_ptr<T> ConcurrentList<T>::FindFirstIf(TFunction predicate)
{
	std::unique_lock<std::mutex> firstLock(m_head->mutex);
	Node* currentNode = m_head;

	// The empty shared pointer is the dummy node.
	while (currentNode->data != nullptr)
	{
		if (predicate(*(currentNode->data)))
		{
			return currentNode->data;
		}

		std::unique_lock<std::mutex> secondLock(currentNode->next->mutex);
		currentNode = currentNode->next;
		firstLock = std::move(secondLock);
	}

	return std::shared_ptr<T>(nullptr);
}

