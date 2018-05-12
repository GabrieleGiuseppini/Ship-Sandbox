/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>

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
     * These types define the cardinality of elements in the container. 
     * 
     * Indices are equivalent to pointers in OO terms. Given that we don't believe
     * we'll ever have more than 4 billion elements, a 32-bit integer suffices. 
     *
     * This also implies that where we used to store one pointer, we can now store two indices,
     * resulting in even better data locality.
     */
    using ElementCount = std::uint32_t;
    using ElementIndex = std::uint32_t;
    static constexpr ElementIndex NoneElementIndex = std::numeric_limits<ElementIndex>::max();

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
     * Visitors.
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

    // TODO: make const after phase I - this is now needed by move assignment
    ElementCount /*const*/ mElementCount;
};
