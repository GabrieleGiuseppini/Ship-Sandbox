/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

/*
* This class implements a simple buffer of "things". The buffer is fixed-size and cannot
* grow more than the size that it is initially constructed with.
*
* The buffer is mem-aligned and takes care of deallocating itself at destruction time.
*/
template <typename TElement>
class Buffer
{
public:

    Buffer(size_t size)
        : mSize(size)
        , mCurrentSize(0)
    {
        size_t elementSize = sizeof(TElement);
        int elementSizePower2 = 2;
        while (elementSize >>= 1) elementSizePower2 <<= 1;
        mBuffer = static_cast<TElement *>(aligned_alloc(elementSizePower2, size * sizeof(TElement)));
        assert(nullptr != mBuffer);
    }

    Buffer(Buffer && other)
        : mBuffer(other.mBuffer)
        , mSize(other.mSize)
        , mCurrentSize(other.mCurrentSize)
    {
        other.mBuffer = nullptr;
    }

    ~Buffer()
    {
        if (nullptr != mBuffer)
        {
            aligned_free(reinterpret_cast<void *>(mBuffer));
        }
    }

    Buffer & operator=(Buffer && other)
    {
        if (nullptr != mBuffer)
        {
            aligned_free(reinterpret_cast<void *>(mBuffer));
        }

        mBuffer = other.mBuffer;
        mSize = other.mSize;
        mCurrentSize = other.mCurrentSize;

        other.mBuffer = nullptr;

        return *this;
    }

    /*
     * Adds an element to the buffer. Assumed to be invoked only at initialization time.
     *
     * Cannot add more elements than the size specified at constructor time.
     */
    template <typename... Args>
    TElement & emplace_back(Args&&... args)
    {
        if (mCurrentSize < mSize)
        {
            return *new(&(mBuffer[mCurrentSize++])) TElement(std::forward<Args>(args)...);
        }
        else
        {
            throw std::runtime_error("The repository is already full");
        }
    }

    /*
     * Gets an element.
     */

    inline TElement const & operator[](size_t index) const noexcept
    {
        assert(index < mCurrentSize);

        return mBuffer[index];
    }

    inline TElement & operator[](size_t index) noexcept
    {
        assert(index < mCurrentSize);

        return mBuffer[index];
    }   

    /*
     * Gets the buffer.
     */

    inline TElement const * data() const
    {
        return mBuffer;
    }

    inline TElement * data()
    {
        return mBuffer;
    }   

private:

    TElement * mBuffer;
    // TODO: make const after phase I
    size_t /*const*/ mSize;
    size_t mCurrentSize;
};