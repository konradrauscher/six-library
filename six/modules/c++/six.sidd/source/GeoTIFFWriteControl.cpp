/* =========================================================================
 * This file is part of six.sidd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2014, MDA Information Systems LLC
 *
 * six.sidd-c++ is free software; you can redistribute it and/or modify
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

#include <sstream>

#include "io/FileOutputStream.h"
#include "sys/Path.h"
#include "scene/GridECEFTransform.h"
#include "scene/Utilities.h"
#include "six/sidd/GeoTIFFWriteControl.h"

#if !defined(SIX_TIFF_DISABLED)

using namespace six;
using namespace six::sidd;

GeoTIFFWriteControl::GeoTIFFWriteControl()
{
    tiff::KnownTagsRegistry::getInstance().addEntry(Constants::GT_XML_KEY,
                                                    tiff::Const::Type::ASCII,
                                                    Constants::GT_XML_TAG);
}
void GeoTIFFWriteControl::initialize(mem::SharedPtr<Container> container)
{

    if (container->getDataType() != DataType::DERIVED)
        throw except::Exception(Ctxt("SICD spec. does not support GeoTIFF"));

    // There still could be complex data in the container though, so we
    // will keep those around for later

    sys::Uint64_T length = 0;
    for (size_t ii = 0; ii < container->getNumData(); ++ii)
    {
        Data* data = container->getData(ii);
        if (data->getDataType() == DataType::COMPLEX)
            mComplexData.push_back(data);
        else if (data->getDataType() == DataType::DERIVED)
        {
            //length = ??
            length += (sys::Uint64_T) data->getNumBytesPerPixel()
                    * (sys::Uint64_T) data->getNumRows()
                    * (sys::Uint64_T) data->getNumCols();

            if (length > Constants::GT_SIZE_MAX)
                throw except::Exception(Ctxt(
                        "Data segments are too large to be stored in " \
                        "GeoTIFF format"));

            mDerivedData.push_back(data);
        }
        else
            throw except::Exception(Ctxt(FmtX(
                    "Data element at position [%d] in container is undefined",
                    ii)));
    }

}

void GeoTIFFWriteControl::save(const SourceList& sources,
                               const std::string& toFile,
                               const std::vector<std::string>& schemaPaths)
{
    tiff::FileWriter tiffWriter(toFile);

    tiffWriter.writeHeader();
    if (sources.size() != mDerivedData.size())
        throw except::Exception(Ctxt(FmtX(
                "Meta-data count [%d] does not match source list [%d]",
                mDerivedData.size(), sources.size())));

    std::vector<unsigned char> buf;
    for (size_t ii = 0; ii < sources.size(); ++ii)
    {
        tiff::ImageWriter* const imageWriter = tiffWriter.addImage();
        const DerivedData* const data =
            reinterpret_cast<DerivedData*>(mDerivedData[ii]);
        const size_t oneRow =
            data->getNumCols() * data->getNumBytesPerPixel();
        tiff::IFD* ifd = imageWriter->getIFD();
        setupIFD(data, ifd, sys::Path::splitExt(toFile).first, schemaPaths);
        buf.resize(oneRow);
        const size_t numRows = data->getNumRows();
        const size_t numCols = data->getNumCols();

        for (size_t row = 0; row < numRows; ++row)
        {
            sources[ii]->read((sys::byte*)&buf[0], oneRow);
            imageWriter->putData(&buf[0], numCols);
        }
        imageWriter->writeIFD();
    }

    tiffWriter.close();

}

void GeoTIFFWriteControl::setupIFD(const DerivedData* data,
                                   tiff::IFD* ifd,
                                   const std::string& toFilePrefix,
                                   const std::vector<std::string>& schemaPaths)
{
    PixelType pixelType = data->getPixelType();
    sys::Uint32_T numRows = (sys::Uint32_T) data->getNumRows();
    sys::Uint32_T numCols = (sys::Uint32_T) data->getNumCols();

    // Start by initializing the TIFF info
    ifd->addEntry(tiff::KnownTags::IMAGE_WIDTH, numCols);

    ifd->addEntry(tiff::KnownTags::IMAGE_LENGTH, numRows);
    ifd->addEntry(tiff::KnownTags::BITS_PER_SAMPLE);

    tiff::IFDEntry* bitsPerSample = (*ifd)[tiff::KnownTags::BITS_PER_SAMPLE];

    unsigned short numBands = data->getNumChannels();
    unsigned short bitDepth = data->getNumBytesPerPixel() * 8 / numBands;

    for (unsigned int j = 0; j < numBands; ++j)
    {
        bitsPerSample->addValue(tiff::TypeFactory::create(
                (unsigned char*) &bitDepth,
                tiff::Const::Type::SHORT));
    }

    unsigned short photoInterp(1);

    if (pixelType == PixelType::RGB8LU)
    {
        ifd->addEntry("ColorMap");
        tiff::IFDEntry* lutEntry = (*ifd)["ColorMap"];
        LUT& lut = *data->display->remapInformation->remapLUT;

        for (unsigned int j = 0; j < 3; ++j)
        {
            for (unsigned int i = 0; i < lut.numEntries; ++i)
            {
                unsigned short lutij = lut[i][j];
                lutEntry->addValue(tiff::TypeFactory::create(
                        (unsigned char*) &lutij,
                        tiff::Const::Type::SHORT));
            }
        }

        photoInterp = 3;
    }
    else if (pixelType == PixelType::RGB24I)
    {
        short spp(3);
        ifd->addEntry(tiff::KnownTags::SAMPLES_PER_PIXEL, spp);
        photoInterp = 2;
    }
    ifd->addEntry(tiff::KnownTags::PHOTOMETRIC_INTERPRETATION, photoInterp);

    addStringArray(ifd,
                   "ImageDescription",
                   FmtX("SIDD: %s", data->getName().c_str()));

    unsigned short orientation(1);
    ifd->addEntry("Orientation", orientation);
    unsigned short planarConf(1);
    ifd->addEntry("PlanarConfiguration", planarConf);

    addStringArray(ifd,
                   "Software",
                   data->productCreation->processorInformation.application);

    char date[256];
    date[255] = 0;
    data->getCreationTime().format("%Y:%m:%d %H:%M:%S", date, 255);

    addCharArray(ifd, "DateTime", date);

    addStringArray(ifd,
                   "Artist",
                   data->productCreation->processorInformation.site);

    ifd->addEntry(tiff::KnownTags::COMPRESSION,
                  (unsigned short) tiff::Const::CompressionType::NO_COMPRESSION);

    // Only GGD pixel space is supported
    if (!data->measurement.get() || !data->measurement->projection.get())
    {
        throw except::Exception(Ctxt("Projection field must be initialized"));
    }

    if (data->measurement->projection->projectionType !=
        ProjectionType::GEOGRAPHIC)
    {
        throw except::Exception(Ctxt("Only a projection type of " +
            ProjectionType(ProjectionType::GEOGRAPHIC).toString() +
            " is supported but the type is set to " +
            data->measurement->projection->projectionType.toString()));
    }
    const GeographicProjection& projection =
        *reinterpret_cast<GeographicProjection *>(
            data->measurement->projection.get());

    addGeoTIFFKeys(projection,
                   data->getNumRows(),
                   data->getNumCols(),
                   ifd,
                   toFilePrefix + ".tfw");

    // Add in the SIDD and SICD xml in a single IFDEntry
    // Each XML section is separated by a null character
    if (!ifd->exists(Constants::GT_XML_TAG))
    {
        ifd->addEntry(Constants::GT_XML_TAG);
    }
    tiff::IFDEntry* const xmlEntry = (*ifd)[Constants::GT_XML_TAG];

    xmlEntry->addValues(six::toValidXMLString(data, schemaPaths, mLog));

    for (size_t jj = 0; jj < mComplexData.size(); ++jj)
    {
        xmlEntry->addValues(six::toValidXMLString(mComplexData[jj],
                                                  schemaPaths, mLog));
    }
}

void GeoTIFFWriteControl::save(const BufferList& sources,
                               const std::string& toFile,
                               const std::vector<std::string>& schemaPaths)
{
    tiff::FileWriter tiffWriter(toFile);

    tiffWriter.writeHeader();
    if (sources.size() != mDerivedData.size())
        throw except::Exception(Ctxt(FmtX(
                "Meta-data count [%d] does not match source list [%d]",
                mDerivedData.size(), sources.size())));

    for (size_t ii = 0; ii < sources.size(); ++ii)
    {

        tiff::ImageWriter* imageWriter = tiffWriter.addImage();
        tiff::IFD* ifd = imageWriter->getIFD();

        DerivedData* data = (DerivedData*) mDerivedData[ii];
        setupIFD(data, ifd, sys::Path::splitExt(toFile).first, schemaPaths);
        // Now we hack to write

        imageWriter->putData(sources[ii], data->getNumRows()
                * data->getNumCols());

        imageWriter->writeIFD();
    }

    tiffWriter.close();
}

void GeoTIFFWriteControl::addCharArray(tiff::IFD* ifd, const std::string &tag,
                                       const char* cstr, int tiffType)
{
    if (!ifd->exists(tag.c_str()))
    {
        ifd->addEntry(tag);
    }

    (*ifd)[tag]->addValues(cstr, tiffType);
}

void GeoTIFFWriteControl::addStringArray(tiff::IFD* ifd,
                                         const std::string &tag,
                                         const std::string &str,
                                         int tiffType)
{
    if (!ifd->exists(tag.c_str()))
    {
        ifd->addEntry(tag);
    }

    (*ifd)[tag]->addValues(str, tiffType);
}

void GeoTIFFWriteControl::addGeoTIFFKeys(
    const GeographicProjection& projection,
    size_t numRows,
    size_t numCols,
    tiff::IFD* ifd,
    const std::string& tfwPathname)
{
    ifd->addEntry("GeoKeyDirectoryTag");
    tiff::IFDEntry* entry = (*ifd)["GeoKeyDirectoryTag"];

    // Write the gt header
    // The first four elements are...
    // keyDirVersion, keyRevision, keyRevisionMinor, numKeys
    // If keys are added/removed from this header, numKeys must be kept in
    // sync
    const short header[] = {
    1,    1, 2, 4,
    1024, 0, 1, 2, /* GTModelTypeGeoKey - Geographic */
    1025, 0, 1, 1, /* GTRasterTypeGeoKey - RasterPixelIsArea */
    2048, 0, 1, 4326, /* GeographicTypeGeoKey - (GCSE_WGS84) 4030 */
    3072, 0, 1, 0 }; /* ProjectedCSTypeGeoKey */
    for (unsigned int i = 0; i < sizeof(header) / sizeof(short); ++i)
    {
        entry->addValue(tiff::TypeFactory::create((unsigned char*) &header[i],
                                                  tiff::Const::Type::SHORT));
    }

    // Find the lat/lon of the upper left and lower right pixel corners.
    // Since we know this is GGD, just these two are sufficient.
    // Note that we CAN'T use the image corners from the SIDD instead because:
    //   a) They may be approximate and aren't for analytical use
    //   b) They may not (and probably do not) correspond to pixel locations
    //      (0, 0) and (numRows-1, numCols-1) since we project north up -
    //      the image likely looks rotated.
    const scene::GeographicGridECEFTransform
        gridTransform(projection.sampleSpacing,
                      projection.referencePoint.rowCol,
                      scene::Utilities::ecefToLatLon(
                          projection.referencePoint.ecef));

    const LatLon upperLeft =
        scene::Utilities::ecefToLatLon(gridTransform.rowColToECEF(0, 0));

    const LatLon lowerRight = scene::Utilities::ecefToLatLon(
        gridTransform.rowColToECEF(numRows - 1, numCols - 1));

    // ModelTiePointTag = (I, J, K, X, Y, Z) where
    // (I, J, K) is the point at location (I, J) in raster space with pixel
    // value K
    // (X, Y, Z) is the corresponding vector in model space
    // In 2D model space, K and Z should be set to 0
    // So, provide the lat/lon that corresponds to pixel (0, 0)
    ifd->addEntry("ModelTiepointTag");
    entry = (*ifd)["ModelTiepointTag"];

    entry->addValue(0.0, 3);
    entry->addValue(upperLeft.getLon());
    entry->addValue(upperLeft.getLat());
    entry->addValue(0.0);

    // ModelPixelScaleTag specifies the size of raster pixel spacing in the
    // model space units in the form (ScaleX, ScaleY, ScaleZ)
    // Again for 2D, ScaleZ is 0
    // We know the lat/lon at each corner and how many pixels we have, so this
    // calculation is simple
    ifd->addEntry("ModelPixelScaleTag");
    entry = (*ifd)["ModelPixelScaleTag"];

    const double scaleX((lowerRight.getLon() - upperLeft.getLon()) / numCols);
    entry->addValue(scaleX);

    const double scaleY((upperLeft.getLat() - lowerRight.getLat()) / numRows);
    entry->addValue(scaleY);

    entry->addValue(0.0);

    // Generate a TIFF World File.
    // This is needed in order for the GeoTIFF to lay down in Google Earth.
    // See http://www.omg.unb.ca/~jonnyb/processing/geotiff_tifw_format.html
    // for a description of the format.
    // Basically, it's a six row text file.  For the case where it's an
    // unrotated mapsheet (like we have since we projected north up), this is
    // simple and is all stuff we've computed above:
    //    Row 1: x-pixel resolution (i.e. scaleX)
    //    Row 2: 0 (non-applicable rotational component)
    //    Row 3: 0 (non-applicable rotational component)
    //    Row 4: y-pixel resolution (i.e. scaleY).  Negative sign indicates
    //           that the image y-axis is positive down which is the opposite
    //           from real world coordinates.
    //    Row 5: longitude of pixel (0, 0)
    //    Row 6: latitude of pixel (0, 0)

    std::ostringstream ostr;
    ostr << scaleX
         << "\n0"
         << "\n0"
         << "\n" << -scaleY
         << "\n" << upperLeft.getLon()
         << "\n" << upperLeft.getLat()
         << "\n";
    const std::string tfwContents(ostr.str());

    io::FileOutputStream stream(tfwPathname);
    stream.write(reinterpret_cast<const sys::byte*>(tfwContents.c_str()),
                 tfwContents.length());
    stream.flush();
    stream.close();
}

#endif

