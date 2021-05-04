#pragma once
#include <mutex>
#include <memory>

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

		Node() : data(nullptr), next(nullptr) {}
		Node(const T& data) : data(std::make_shared<T>(data)), next(nullptr) {}
	};

	Node* m_head;
};

template<typename T>
inline ConcurrentList<T>::ConcurrentList() : m_head(new Node) // Each list has single dummy node.
{
}

template<typename T>
inline ConcurrentList<T>::~ConcurrentList()
{
	std::unique_lock<std::mutex> lock(m_head->mutex);
	Node* currentNode = m_head;

	while (currentNode->next != nullptr)
	{
		Node* nodeToDelete = currentNode;
		currentNode = currentNode->next;
		lock = std::move(std::unique_lock<std::mutex>(currentNode->mutex));
	}

	lock.unlock();
	delete currentNode; // All but the last node were deleted above.
}

template<typename T>
inline void ConcurrentList<T>::PushToFront(const T& data)
{
	Node* newNode = new Node(data);
	std::lock_guard<std::mutex> lock(m_head->mutex);
	newNode->next = m_head->next;
	m_head->next = newNode;
}

template<typename T>
template<typename TFunction>
inline void ConcurrentList<T>::ForEach(TFunction function)
{
	std::unique_lock<std::mutex> lock(m_head->mutex);
	Node* currentNode = m_head->next; // The head node is a dummy enpty node.

	while (currentNode != nullptr)
	{
		lock = std::move(std::unique_lock<std::mutex>(currentNode->mutex));
		function(*(currentNode->data));
		currentNode = currentNode->next;
	}
}

template<typename T>
template<typename TFunction>
inline std::shared_ptr<T> ConcurrentList<T>::FindFirstIf(TFunction predicate)
{
	std::unique_lock<std::mutex> lock(m_head->mutex);
	Node* currentNode = m_head->next;

	while (currentNode != nullptr)
	{
		lock  = std::move(std::unique_lock<std::mutex>(currentNode->mutex));

		if (predicate(*(currentNode->data)))
		{
			return currentNode->data;
		}

		currentNode = currentNode->next;
	}
	return std::shared_ptr<T>(nullptr);
}

template<typename T>
template<typename TFunction>
inline void ConcurrentList<T>::RemoveIf(TFunction predicate)
{
	std::unique_lock<std::mutex> firstLock(m_head->mutex);
	Node* previouseNode = m_head;
	Node* currentNode = m_head->next;

	while (currentNode != nullptr)
	{
		std::unique_lock<std::mutex> secondLock(currentNode->mutex);

		if (predicate(*(currentNode->data)))
		{
			previouseNode->next = currentNode->next;
			Node* nodeToDelete = currentNode;
			currentNode = currentNode->next;
			secondLock.unlock();
			delete nodeToDelete;
		}

		else
		{
			previouseNode = currentNode;
			currentNode = currentNode->next;
			firstLock = std::move(secondLock);
		}
	}
}
