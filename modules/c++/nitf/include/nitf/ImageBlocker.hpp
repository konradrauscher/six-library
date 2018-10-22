/* =========================================================================
 * This file is part of NITRO
 * =========================================================================
 *
 * (C) Copyright 2004 - 2018, MDA Information Systems LLC
 *
 * NITRO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __NITF_IMAGE_BLOCKER_HPP__
#define __NITF_IMAGE_BLOCKER_HPP__

#include <stddef.h>
#include <string.h>
#include <vector>

#include <sys/Conf.h>

namespace nitf
{
/*!
 * \class ImageBlocker
 * \brief Supports efficiently rearranging an image to fulfill NITF blocking
 * conventions, including pad rows and columns (NITF spec specifies that full
 * blocks will always be written).  Only supports layouts where image segments
 * are stacked vertically.  Following convention of other areas of the code that
 * write NITFs, will limit the block size to the size of the dimension (which
 * means for multi-segment cases, there may be a different num rows / block in
 * each segment).
 */
class ImageBlocker
{
public:
    /*!
     * Use when there's a single image segment
     *
     * \param numRows Number of rows
     * \param numCols Number of columns
     * \param numRowsPerBlock Number of rows per block
     * \param numColsPerBlock Number of columns per block
     */
    ImageBlocker(size_t numRows,
                 size_t numCols,
                 size_t numRowsPerBlock,
                 size_t numColsPerBlock);

    /*!
     * Use when there are multiple image segments
     *
     * \param numRowsPerSegment Number of rows in each segment
     * \param numCols Number of columns
     * \param numRowsPerBlock Number of rows per block
     * \param numColsPerBlock Number of columns per block
     */
    ImageBlocker(const std::vector<size_t>& numRowsPerSegment,
                 size_t numCols,
                 size_t numRowsPerBlock,
                 size_t numColsPerBlock);

    /*!
     * \tparam DataT Data type of image pixels
     *
     * \param startRow Start row.  This must start on a block boundary (within
     * a segment).
     * \param numRows Number of rows.  This must be a multiple of the block size
     * unless it's at the end of a segment.
     *
     * \return The number of bytes required to store this AOI of the image in
     * blocked format, including pad rows and columns
     */
    template <typename DataT>
    size_t getNumBytesRequired(size_t startRow,
                               size_t numRows) const
    {
        return getNumBytesRequired(startRow, numRows, sizeof(DataT));
    }

    /*!
     * \param startRow Start row.  This must start on a block boundary (within
     * a segment).
     * \param numRows Number of rows.  This must be a multiple of the block size
     * unless it's at the end of a segment.
     * \param numBytesPerPixel Number of bytes per pixel of image pixels
     *
     * \return The number of bytes required to store this AOI of the image in
     * blocked format, including pad rows and columns
     */
    size_t getNumBytesRequired(size_t startRow,
                               size_t numRows,
                               size_t numBytesPerPixel) const;

    /*!
     * \param input Input image of size 'numRows' x numCols (from constructor)
     * \param startRow Start row in the global image that 'input' points to.
     * This must start on a block boundary (within a segment).
     * \param numRows Number of rows.  This must be a multiple of the block size
     * unless it's at the end of a segment.
     * \param numBytesPerPixel Number of bytes per pixel of 'input'
     * \param[out] output Blocked representation of 'input', including pad rows
     * and columns
     */
    void block(const void* input,
               size_t startRow,
               size_t numRows,
               size_t numBytesPerPixel,
               void* output) const;

    /*!
     * \tparam DataT Data type of 'input' and 'output'
     *
     * \param input Input image of size 'numRows' x numCols (from constructor)
     * \param startRow Start row in the global image that 'input' points to.
     * This must start on a block boundary (within a segment).
     * \param numRows Number of rows.  This must be a multiple of the block size
     * unless it's at the end of a segment.
     * \param[out] output Blocked representation of 'input', including pad rows
     * and columns
     */
    template <typename DataT>
    void block(const DataT* input,
               size_t startRow,
               size_t numRows,
               DataT* output) const
    {
        block(input, startRow, numRows, sizeof(DataT), output);
    }

    //! \return The number of columns of blocks
    size_t getNumColsOfBlocks() const
    {
        return mNumBlocksAcrossCols;
    }

    /*!
     * \param seg Segment
     *
     * \return The number of rows of blocks for the specified segment.
     */
    size_t getNumRowsOfBlocks(size_t seg) const
    {
        return mNumBlocksDownRows.at(seg);
    }

    /*!
     * \param seg Segment
     *
     * \return The number of pad rows in the final block of the specified
     * segment
     */
    size_t getNumPadRowsInFinalBlock(size_t seg) const
    {
        return mNumPadRowsInFinalBlock.at(seg);
    }

    /*!
     * \param seg Segment
     *
     * \return The global start row of the specified segment
     */
    size_t getStartRow(size_t seg) const
    {
        return mStartRow.at(seg);
    }

    /*!
     * \param seg Segment
     *
     * \return The number of rows of the specified segment
     */
    size_t getNumRows(size_t seg) const
    {
        return mNumRows.at(seg);
    }

    //! \return The number of segments
    size_t getNumSegments() const
    {
        return mNumBlocksDownRows.size();
    }

    /*!
     * \return The number of rows per block for each segment.  If a segment
     * contains more rows than numRowsPerBlock, this will be numRowsPerBlock.
     * Otherwise it will be the number of rows in the segment.
     */
    std::vector<size_t> getNumRowsPerBlock() const
    {
        return mNumRowsPerBlock;
    }

    /*!
     * \return The number of columns per block (always the same value for all
     * segments).  If the number of columns in the image are less than the
     * constructed number of columns per block, this will be the number of
     * columns in the image (i.e. no reason to bother to create blocks larger
     * than the image).
     */
    size_t getNumColsPerBlock() const
    {
        return mNumColsPerBlock;
    }

    /*
     * \return The segment the block-row index falls in
     * \throws If blockRow exceeds the total block rows in the image
     */
    size_t getSegmentFromGlobalBlockRow(size_t blockRow) const;

private:
    void findSegment(size_t row,
                     size_t& segIdx,
                     size_t& rowWithinSegment,
                     size_t& blockWithinSegment) const;

    bool isFirstRowInBlock(size_t seg, size_t rowWithinSeg) const
    {
        return (rowWithinSeg % mNumRowsPerBlock[seg] == 0);
    }

    void findSegmentRange(size_t startRow,
                          size_t numRows,
                          size_t& firstSegIdx,
                          size_t& startBlockWithinFirstSeg,
                          size_t& lastSegIdx,
                          size_t& lastBlockWithinLastSeg) const;

    void blockImpl(size_t seg,
                   const sys::byte* input,
                   size_t numValidRowsInBlock,
                   size_t numValidColsInBlock,
                   size_t numBytesPerPixel,
                   sys::byte* output) const;

    void blockAcrossRow(size_t seg,
                        const sys::byte*& input,
                        size_t numValidRowsInBlock,
                        size_t numBytesPerPixel,
                        sys::byte*& output) const;

private:
    // Vectors all indexed by segment
    std::vector<size_t> mStartRow;
    const std::vector<size_t> mNumRows;
    const size_t mTotalNumRows;
    const size_t mNumCols;
    std::vector<size_t> mNumRowsPerBlock;
    const size_t mNumColsPerBlock;

    std::vector<size_t> mNumBlocksDownRows;
    std::vector<size_t> mNumPadRowsInFinalBlock;

    size_t mNumBlocksAcrossCols;
    size_t mNumPadColsInFinalBlock;
};
}

#endif