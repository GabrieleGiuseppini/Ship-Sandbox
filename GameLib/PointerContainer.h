/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <cstring>
#include <vector>

/*
 * This class is a container of pointers to arbitrary types. The container *owns* the pointers
 * and thus frees them each time an element is removed from the container, or when the
 * container itself is deleted.
 *
 * The container is optimized for fast *visit*, so that it can be used to iterate through
 * all elements. 
 *
 * Removing individual elements from the container is achieved by means of setting an "IsDeleted" flag
 * on the element(s) to be removed and then invoking the container's ShrinkToFit() method. Removal of
 * elements is not optimized.
 */
template<typename TElement>
class PointerContainer
{
private:

	/*
	 * Our iterator.
	 */
	template <
		typename TIteratedPointer, 
		typename TIteratedReference,
		typename TIteratedPointerPointer>
	struct _iterator
	{
	public:

		inline bool operator==(_iterator const & other) const noexcept
		{
			return mCurrent == other.mCurrent;
		}

		inline bool operator!=(_iterator const & other) const noexcept
		{
			return !(mCurrent == other.mCurrent);
		}

		inline void operator++() noexcept
		{
			++mCurrent;
		}

		inline TIteratedReference operator*() noexcept
		{
			return *(*mCurrent);
		}

		inline TIteratedPointer operator->() noexcept
		{
			return *mCurrent;
		}

	private:

		friend class PointerContainer<TElement>;

		explicit _iterator(TIteratedPointerPointer current) noexcept
			: mCurrent(current)
		{}

		TIteratedPointerPointer mCurrent;
	};

public:

	typedef _iterator<TElement *, TElement &, TElement **> iterator;
	
	typedef _iterator<TElement const *, TElement const &, TElement const * const *> const_iterator;
	
public:

	explicit PointerContainer(std::vector<TElement *> && pointers)
	{
		// Allocate array of pointers and copy all pointers
		mPointers = new TElement*[pointers.size()];
		memcpy(mPointers, pointers.data(), pointers.size() * sizeof(TElement *));
		mSize = pointers.size();

		pointers.clear();
	}

	~PointerContainer()
	{
		// Delete all pointers we own
		for (size_t i = 0; i < mSize; ++i)
		{
			delete mPointers[i];
		}
	}

	//
	// Visitors
	//

	inline iterator begin() noexcept
	{
		return iterator(mPointers);
	}

	inline iterator end() noexcept
	{
		return iterator(mPointers + mSize);
	}

	inline const_iterator begin() const noexcept
	{
		return const_iterator(mPointers);
	}

	inline const_iterator end() const noexcept
	{
		return const_iterator(mPointers + mSize);
	}

	TElement * operator[](size_t index) noexcept
	{
		assert(index < mSize);
		return mPointers[index];
	}

	TElement const * operator[](size_t index) const noexcept
	{
		assert(index < mSize);
		return mPointers[index];
	}

	size_t size() const noexcept
	{
		return mSize;
	}

	//
	// Modifiers
	//

	void shrink_to_fit()
	{
		// TODO
	}

private:

	// The actual array of pointers
	TElement * * mPointers;

	// The size of the array
	size_t mSize;
};
