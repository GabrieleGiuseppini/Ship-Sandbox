/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <cassert>
#include <cstdint>
#include <iterator>

/*
 * This class is the base class of all containers of core elements.
 *
 * For data locality, we don't work with "objects" in the OO way, but rather
 * with sets of objects, whose properties are located in multiple, non-overlapping buffers.
 *
 * The container itself is not modifiable once all its elements have been created.
 */
class ElementContainer
{
public:

    /*
     * Our iterator, which simply iterates through indices.
     */
    struct iterator
    {
    public:

        typedef std::input_iterator_tag iterator_category;

    public:

        inline bool operator==(iterator const & other) const noexcept
        {
            return mCurrent == other.mCurrent;
        }

        inline bool operator!=(iterator const & other) const noexcept
        {
            return !(mCurrent == other.mCurrent);
        }

        inline void operator++() noexcept
        {
            ++mCurrent;
        }

        inline ElementIndex operator*() noexcept
        {
            return mCurrent;
        }

    private:

        friend class ElementContainer;

        explicit iterator(ElementIndex current) noexcept
            : mCurrent(current)
        {}

        ElementIndex mCurrent;
    };

public:

    /*
     * Gets the number of elements in this container.
     */
    ElementCount GetElementCount() const { return mElementCount; }

    /*
     * Visitors. These iterators iterate the *indices* of the elements.
     */

    inline iterator begin() const noexcept
    {
        return iterator(0u);
    }

    inline iterator end() const noexcept
    {
        return iterator(mElementCount);
    }

protected:

    ElementContainer(ElementCount elementCount)
        : mElementCount(elementCount)
    {
    }

    ElementCount const mElementCount;
};
