#pragma once
#include <vector>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <memory>

template <typename TKey, typename TValue, typename THashFunction=std::hash<TKey>>
class ConcurrentHashtable
{
public:
	// Public type aliases.
	using Key = TKey;
	using Value = TValue;
	using HashFunction = THashFunction;

	ConcurrentHashtable(size_t numBuckets = 5, const HashFunction& hashFunction = HashFunction());
	~ConcurrentHashtable() = default;

	// Copy semantics.
	ConcurrentHashtable(const ConcurrentHashtable<TKey, TValue, THashFunction>& other) = delete;
	ConcurrentHashtable<TKey, TValue, THashFunction>& operator=(const ConcurrentHashtable<TKey, TValue, THashFunction>& other) = delete;

	// Move semantics.
	ConcurrentHashtable(ConcurrentHashtable<TKey, TValue, THashFunction>&& other) = delete;
	ConcurrentHashtable<TKey, TValue, THashFunction>& operator=(ConcurrentHashtable<TKey, TValue, THashFunction>&& other) = delete;

	// Return a shared pointer with the data, or an empty shared pointer if no entry for such key exists.
	std::shared_ptr<typename ConcurrentHashtable<TKey, TValue, THashFunction>::Value> GetValueForKey(const Key& key) const;

	// Add or change the key value pair.
	void SetValueForKey(const Key& key, const Value& value);

	// Remove the key value pair with the supplied key if it exists. Otherwise do nothing.
	void RemoveEntry(const Key& key);

	// Clear the contents of each bucket.
	void Clear();

private:
	// Internal type aliases.
	using KeyValuePair = std::pair<Key, Value>;
	using KeyValuePairList = std::list<std::pair<Key, Value>>;
	using KeyValuePairListIterator = typename std::list<std::pair<Key, Value>>::iterator;

	// Hashtable Bucket type.
	struct Bucket
	{
		KeyValuePairList keyValueList;
		std::shared_mutex sharedMutex;

		KeyValuePairListIterator GetEntryForKey(const Key& key);
	};

	// Class Member variables.
	std::vector<std::unique_ptr<Bucket>> m_buckets;
	HashFunction m_hashFunction;

	// Private Helper methods.
	Bucket& GetBucket(const Key& key) const;
};

template<typename TKey, typename TValue, typename THashFunction>
inline ConcurrentHashtable<TKey, TValue, THashFunction>::ConcurrentHashtable(size_t numBuckets, const HashFunction& hashFunction) :
	m_buckets(numBuckets), m_hashFunction(hashFunction)
{
	for (size_t i = 0; i < numBuckets; ++i)
	{
		m_buckets[i] = std::make_unique<Bucket>();
	}
}

template<typename TKey, typename TValue, typename THashFunction>
inline std::shared_ptr<typename ConcurrentHashtable<TKey, TValue, THashFunction>::Value> ConcurrentHashtable<TKey, TValue, THashFunction>::GetValueForKey(const Key& key) const
{
	Bucket& bucket = GetBucket(key);

	// Ensure multiple threads can read at once.
	std::shared_lock<std::shared_mutex> lock(bucket.sharedMutex);

	// Retrieve an iterator to determine if the key is in the list.
	KeyValuePairListIterator iterator = bucket.GetEntryForKey(key);

	// If the key is in the list return a shared pointer encapsulating the data.
	if (iterator != bucket.keyValueList.end())
	{
		return std::make_shared<typename ConcurrentHashtable<TKey, TValue, THashFunction>::Value>(iterator->second);
	}

	// Else return an empty shared pointer.
	return std::shared_ptr<typename ConcurrentHashtable<TKey, TValue, THashFunction>::Value>();
}

template<typename TKey, typename TValue, typename THashFunction>
inline void ConcurrentHashtable<TKey, TValue, THashFunction>::SetValueForKey(const Key& key, const Value& value)
{
	Bucket& bucket = GetBucket(key);

	// Ensure only one thread can write at a time.
	std::unique_lock<std::shared_mutex> lock(bucket.sharedMutex);
	
	// Retrieve an iterator to determine if the key is in the list.
	KeyValuePairListIterator iterator = bucket.GetEntryForKey(key);

	// If the key is already in the list replace its value.
	if (iterator != bucket.keyValueList.end())
	{
		iterator->second = value;
	}

	// Else create a new key-value pair.
	else
	{
		bucket.keyValueList.emplace_back(key, value);
	}
}

template<typename TKey, typename TValue, typename THashFunction>
inline void ConcurrentHashtable<TKey, TValue, THashFunction>::RemoveEntry(const Key& key)
{
	Bucket& bucket = GetBucket(key);
	
	// Ensure only one thread can modify the table at a time.
	std::unique_lock<std::shared_mutex> lock(bucket.sharedMutex);

	// Retrieve and iterator to determine if the key is in the list.
	KeyValuePairListIterator iterator = bucket.GetEntryForKey(key);

	// If the key exists remove it.
	if (iterator != bucket.keyValueList.end())
	{
		bucket.keyValueList.erase(iterator);
	}
}

template<typename TKey, typename TValue, typename THashFunction>
inline void ConcurrentHashtable<TKey, TValue, THashFunction>::Clear()
{
	for (std::unique_ptr<Bucket>& bucket : m_buckets)
	{
		std::unique_lock<std::shared_mutex> lock(bucket->sharedMutex);
		bucket->keyValueList.clear();
	}
}

template<typename TKey, typename TValue, typename THashFunction>
inline typename ConcurrentHashtable<TKey, TValue, THashFunction>::Bucket& ConcurrentHashtable<TKey, TValue, THashFunction>::GetBucket(const Key& key) const
{
	const size_t bucketIndex = m_hashFunction(key) % m_buckets.size();
	return *m_buckets[bucketIndex];
}

template<typename TKey, typename TValue, typename THashFunction>
inline typename ConcurrentHashtable<TKey, TValue, THashFunction>::KeyValuePairListIterator ConcurrentHashtable<TKey, TValue, THashFunction>::Bucket::GetEntryForKey(const Key& key)
{
	return std::find_if(keyValueList.begin(), keyValueList.end(),
		[&](const KeyValuePair& pair) -> bool { return pair.first == key; });
}

