/* =========================================================================
 * This file is part of cphd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2019, MDA Information Systems LLC
 *
 * cphd-c++ is free software; you can redistribute it and/or modify
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
 * License along with this program; If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>
#include <sys/Conf.h>
#include <types/RowCol.h>
#include <io/TempFile.h>
#include <io/FileInputStream.h>
#include <io/FileOutputStream.h>
#include <cphd/CPHDWriter.h>
#include <cphd/CPHDReader.h>
#include <cphd/Wideband.h>
#include <cphd/Metadata.h>
#include <cphd/PVP.h>
#include <cphd/PVPBlock.h>
#include <cphd/ReferenceGeometry.h>
#include <cphd/TestDataGenerator.h>
#include <TestCase.h>

/*!
 * Tests write and read of Signal Block with compressed data
 * Fails if values don't match
 */
namespace
{
std::vector<sys::ubyte> generateCompressedData(size_t length)
{
    std::vector<sys::ubyte> data(length);
    srand(0);
    for (size_t ii = 0; ii < data.size(); ++ii)
    {
        data[ii] = rand() % 16;
    }
    return data;
}

void writeCompressedCPHD(const std::string& outPathname, size_t numThreads,
        const types::RowCol<size_t> dims,
        const std::vector<sys::ubyte>& writeData,
        cphd::Metadata& metadata,
        cphd::PVPBlock& pvpBlock)
{
    const size_t numChannels = 1;
    const std::vector<size_t> numVectors(numChannels, dims.row);

    for (size_t ii = 0; ii < numChannels; ++ii)
    {
        for (size_t jj = 0; jj < numVectors[ii]; ++jj)
        {
            cphd::setVectorParameters(ii, jj, pvpBlock);
        }
    }

    cphd::CPHDWriter writer(metadata, outPathname, std::vector<std::string>(), numThreads);

    writer.writeMetadata(pvpBlock);

    writer.writePVPData(pvpBlock);

    for (size_t ii = 0; ii < numChannels; ++ii)
    {
        writer.writeCPHDData(writeData.data(),1,ii);
    }
}

std::vector<sys::ubyte> checkCompressedData(const std::string& pathname,
        size_t numThreads,
        const types::RowCol<size_t>& dims)
{

    cphd::CPHDReader reader(pathname, numThreads);
    const cphd::Wideband& wideband = reader.getWideband();
    std::vector<sys::ubyte> readData(dims.area());

    mem::BufferView<sys::ubyte> data(&readData[0], readData.size());
    for (size_t ii = 0; ii < reader.getMetadata().data.getNumChannels(); ++ii)
    {
        wideband.read(ii, data);
    }
    return readData;
}

bool compareVectors(const std::vector<sys::ubyte>& readData,
                    const std::vector<sys::ubyte>& writeData)
{
    for (size_t ii = 0; ii < readData.size(); ++ii)
    {
        if (writeData[ii] != readData[ii])
        {
            std::cerr << "Value mismatch at index " << ii << std::endl;
            return false;
        }
    }
    return true;
}

bool runTest(const std::vector<sys::ubyte>& writeData)
{
    io::TempFile tempfile;
    const size_t numThreads = sys::OS().getNumCPUs();
    const types::RowCol<size_t> dims(128, 256);
    cphd::Metadata meta = cphd::Metadata();
    meta.data.signalCompressionID = "Huffman";
    cphd::setUpData(meta, dims, writeData);
    cphd::setPVPXML(meta.pvp);
    cphd::PVPBlock pvpBlock(meta.pvp, meta.data);

    writeCompressedCPHD(tempfile.pathname(), numThreads, dims, writeData, meta, pvpBlock);
    const std::vector<sys::ubyte> readData =
            checkCompressedData(tempfile.pathname(), numThreads,
            dims);
    return compareVectors(readData, writeData);
}

TEST_CASE(testCompressed)
{
    const types::RowCol<size_t> dims(128, 256);
    const std::vector<sys::ubyte> writeData =
            generateCompressedData(dims.area());
    TEST_ASSERT_TRUE(runTest(writeData));
}
}

int main(int argc, char** argv)
{
    try
    {
        TEST_CHECK(testCompressed);
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    catch (const except::Exception& ex)
    {
        std::cerr << ex.toString() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception\n";
        return 1;
    }
}

