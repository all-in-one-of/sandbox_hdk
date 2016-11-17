/*
 * Copyright (c) 2015
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <iostream>
#include <UT/UT_Assert.h>
#include <UT/UT_IOTable.h>
#include <UT/UT_Set.h>
#include <UT/UT_StringMap.h>
#include <GA/GA_Handle.h>
#include <GEO/GEO_AttributeHandle.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimVolume.h>
#include <GEO/GEO_IOTranslator.h>
#include <SOP/SOP_Node.h>

// These are required for Field3d.
#undef drand48
#undef srand48

#include <Field3D/Field.h>
#include <Field3D/Field3DFile.h>
#include <Field3D/InitIO.h>
#include <Field3D/DenseField.h>
#include <Field3D/SparseField.h>
#include <Field3D/MACField.h>

#include "f3d_io.h"

using namespace HDK_Sample;

//
// Converts between F3D matrices and Houdini matrices.  
//
void
f3d_matrixConvertToF3D(const UT_DMatrix4 &src, Field3D::M44d &dst)
{
    for (int y = 0; y < 4; y++)
	for (int x = 0; x < 4; x++)
	    dst[x][y] = src(x, y);
}

void
f3d_matrixConvertToUT(const Field3D::M44d &src, UT_DMatrix4 &dst)
{
    for (int y = 0; y < 4; y++)
	for (int x = 0; x < 4; x++)
	    dst(x, y) = src[x][y];
}

template <typename FIELD_PTR>
void
f3d_getTransformFromField(const FIELD_PTR field, GEO_PrimVolume *vol,
			    bool ismacfield, int axis, int res)
{
    UT_DMatrix4			 xform4;
    Field3D::FieldMapping	*raw_mapping = field->mapping().get();
    Field3D::MatrixFieldMapping *mtx_mapping = dynamic_cast<Field3D::MatrixFieldMapping *>(raw_mapping);

    xform4.identity();

    // Null mappings aren't entirely trivial as Houdini uses 0 centered
    // mappings
    if (dynamic_cast<Field3D::NullFieldMapping *>(raw_mapping))
    {
	xform4.identity();
	xform4.scale(0.5, 0.5, 0.5);
	xform4.translate(0.5, 0.5, 0.5);
    }
    else if (mtx_mapping)
    {
	const Field3D::M44d f3dmtx = mtx_mapping->localToWorld();

	f3d_matrixConvertToUT(f3dmtx, xform4);

	// The field3d matrix, now in xform4, will map 0..1 to the desired
	// space.  We instead want to map [-1..1] to the desired space.
	// So we prefix with the appropriate xform.
	UT_DMatrix4	tohoudini;

	tohoudini.identity();
	tohoudini.scale(0.5, 0.5, 0.5);
	tohoudini.translate(0.5, 0.5, 0.5);

	xform4 = tohoudini * xform4;
    }
    else
    {
	std::cerr << "Warning: Unknown mapping type found." << std::endl;
    }

    if (ismacfield)
    {
	UT_DMatrix4	tomac;

	tomac.identity();
	double		scalefactor[3] = { 1, 1, 1 };

	// Since we are 0 centered, the mac correction can be done
	// by scaling by one voxel.
	scalefactor[axis] *= ((double)res) / ((double)(res+1));

	tomac.scale(scalefactor[0], scalefactor[1], scalefactor[2]);

	xform4 = tomac * xform4;
    }

    vol->setTransform4(xform4);
}

template <typename FIELD_PTR>
void
f3d_getTransformFromVolume(const GEO_PrimVolume *vol, FIELD_PTR field)
{
    UT_DMatrix4		xform4;

    vol->getTransform4(xform4);

    // The houdini matrix maps the world coords to -1..1.  We want
    // to convert it to 0..1 for the field3d equivalent.
    UT_DMatrix4	fromhoudini;

    fromhoudini.identity();
    fromhoudini.scale(2, 2, 2);
    fromhoudini.translate(-1, -1, -1);

    xform4 = fromhoudini * xform4;

    Field3D::M44d	f3dmtx;

    f3d_matrixConvertToF3D(xform4, f3dmtx);

    Field3D::MatrixFieldMapping::Ptr mapping(new Field3D::MatrixFieldMapping);

    mapping->setLocalToWorld(f3dmtx);

    field->setMapping(mapping);

    // Simple test to verify our field3d mapping matches our Houdini one.
#if 0
    {
	UT_Vector3	pt(0, 0, 0);
	Field3D::V3d	wpt(0, 0, 0), vpt;

	cerr << "volume to world " << vol->fromVoxelSpace(pt) << endl;
	mapping->localToWorld(wpt, vpt);
	cerr << "Mapping to world " << vpt << endl;
    }{
	UT_Vector3	pt(1, 0, 0);
	Field3D::V3d	wpt(1, 0, 0), vpt;

	cerr << "volume to world " << vol->fromVoxelSpace(pt) << endl;
	mapping->localToWorld(wpt, vpt);
	cerr << "Mapping to world " << vpt << endl;
    }{
	UT_Vector3	pt(0, 1, 0);
	Field3D::V3d	wpt(0, 1, 0), vpt;

	cerr << "volume to world " << vol->fromVoxelSpace(pt) << endl;
	mapping->localToWorld(wpt, vpt);
	cerr << "Mapping to world " << vpt << endl;
    }{
	UT_Vector3	pt(0, 0, 1);
	Field3D::V3d	wpt(0, 0, 1), vpt;

	cerr << "volume to world " << vol->fromVoxelSpace(pt) << endl;
	mapping->localToWorld(wpt, vpt);
	cerr << "Mapping to world " << vpt << endl;
    }
#endif
}

///
/// f3d_loadField loads any generic field through the virtual value()
///
template <typename FIELD_PTR>
void
f3d_loadField(UT_VoxelArrayF *dst, const FIELD_PTR field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.
    UT_VoxelArrayIteratorF vit;

    Field3D::V3i database;
    database = field->dataWindow().min;

    vit.setArray(dst);
    vit.setCompressOnExit(true);
    for (vit.rewind(); !vit.atEnd(); vit.advance())
    {
	float		v;

	v = field->value(vit.x() + database.x, 
			vit.y() + database.y, 
			vit.z() + database.z);

	vit.setValue(v);
    }
}

///
/// f3d_loadDenseField loads fully specified fields via fastValue.
/// The Dense in the name is because it relies on fast random access
///
template <typename FIELD_PTR>
void
f3d_loadDenseField(UT_VoxelArrayF *dst, const FIELD_PTR field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.
    UT_VoxelArrayIteratorF vit;

    Field3D::V3i database;
    database = field->dataWindow().min;

    vit.setArray(dst);
    vit.setCompressOnExit(true);
    for (vit.rewind(); !vit.atEnd(); vit.advance())
    {
	float		v;

	v = field->fastValue(vit.x() + database.x, 
			vit.y() + database.y, 
			vit.z() + database.z);

	vit.setValue(v);
    }
}

template <typename FIELD>
void
f3d_loadSparseField(UT_VoxelArrayF *dst, const FIELD *field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.
    UT_VoxelArrayIteratorF vit;

    Field3D::V3i database;
    database = field->dataWindow().min;

    // Determine if our block sizes match.  If they do, we can detect
    // constant blocks in our source and avoid unnecessary load/compression
    // This could be optimized for larger block orders, but for now
    // only exact matches are allowed
    if (field->blockSize() != 16)
    {
	// Not matching, so just do a dense-style load.
	// Since we have fully specified our FIELD_PTR, we know fastValue
	// will exist.
	f3d_loadDenseField(dst, field);
	return;
    }
    
    // We rely on alignment of the blocks.  If the data window
    // isn't a multiple of 16, the blocks won't be aligned.
    if (database.x & 15 ||
	database.y & 15 ||
	database.z & 15)
    {
	f3d_loadDenseField(dst, field);
	return;
    }

    typename FIELD::block_iterator bi = field->blockBegin();

    // Determine if we are a half float field.
    bool		isfp16 = sizeof(field->getBlockEmptyValue(0, 0, 0)) == sizeof(fpreal16);

    for (; bi != field->blockEnd(); ++bi)
    {
	Field3D::V3i		bmin, bmax, bsize;
	UT_VoxelTile<float>	*tile;

	tile = dst->getTile(bi.x, bi.y, bi.z);

	if (!field->blockIsAllocated(bi.x, bi.y, bi.z))
	{
	    float	v;

	    v = field->getBlockEmptyValue(bi.x, bi.y, bi.z);

	    tile->makeConstant(v);
	}
	else
	{
	    // A fully allocated block.
	    bmin = bi.blockBoundingBox().min;
	    bmax = bi.blockBoundingBox().max;
	    bsize = bmax - bmin + Field3D::V3i(1);

	    for (int z = 0; z < bsize.z; z++)
	    {
		for (int y = 0; y < bsize.y; y++)
		{
		    for (int x = 0; x < bsize.x; x++)
		    {
			float 		v;

			// This will recompute the block index which
			// seems wasteful, but I'm unsure how to access
			// the block in a raw manner.
			v = field->fastValue(x+bmin.x, y+bmin.y, z+bmin.z);

			tile->setValue(x, y, z, v);
		    }
		}
	    }

	    // If we are 16 bit float, force the tile to compress
	    // right away.
	    if (isfp16)
		tile->makeFpreal16();
	}
    }
}

///
/// f3d_loadField loads any generic field through the virtual value()
///
template <typename FIELD_PTR>
void
f3d_loadField(UT_VoxelArrayF *dst[3], const FIELD_PTR field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.
    UT_VoxelArrayIteratorF vit[3];

    Field3D::V3i database;
    database = field->dataWindow().min;

    for (int i = 0; i < 3; i++)
    {
	vit[i].setArray(dst[i]);
	vit[i].setCompressOnExit(true);
	vit[i].rewind();
    }
    for (; !vit[0].atEnd(); )
    {
	Field3D::V3f		v;

	v = field->value(vit[0].x() + database.x, 
			vit[0].y() + database.y, 
			vit[0].z() + database.z);

	vit[0].setValue(v.x);
	vit[1].setValue(v.y);
	vit[2].setValue(v.z);
	vit[0].advance();
	vit[1].advance();
	vit[2].advance();
    }
}

///
/// f3d_loadDenseField loads fully specified fields via fastValue.
/// The Dense in the name is because it relies on fast random access
///
template <typename FIELD_PTR>
void
f3d_loadDenseField(UT_VoxelArrayF *dst[3], const FIELD_PTR field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.
    UT_VoxelArrayIteratorF vit[3];

    Field3D::V3i database;
    database = field->dataWindow().min;

    for (int i = 0; i < 3; i++)
    {
	vit[i].setArray(dst[i]);
	vit[i].setCompressOnExit(true);
	vit[i].rewind();
    }
    for (; !vit[0].atEnd(); )
    {
	Field3D::V3f		v;

	v = field->fastValue(vit[0].x() + database.x, 
			vit[0].y() + database.y, 
			vit[0].z() + database.z);

	vit[0].setValue(v.x);
	vit[1].setValue(v.y);
	vit[2].setValue(v.z);
	vit[0].advance();
	vit[1].advance();
	vit[2].advance();
    }
}

///
/// f3d_loadMACField handles the special u/v/w passes of mac fields.
///
template <typename FIELD_PTR>
void
f3d_loadMACField(UT_VoxelArrayF *dst[3], const FIELD_PTR field)
{
    // Iterate over the field in voxel block order so we can compress
    // efficiently.

    for (int i = 0; i < 3; i++)
    {
	UT_VoxelArrayIteratorF vit;

	Field3D::V3i database;
	database = field->dataWindow().min;

	vit.setArray(dst[i]);
	vit.setCompressOnExit(true);
	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
	    float		v;

	    if (i == 0)
		v = field->u(vit.x() + database.x, 
			    vit.y() + database.y, 
			    vit.z() + database.z);
	    else if (i == 1)
		v = field->v(vit.x() + database.x, 
			    vit.y() + database.y, 
			    vit.z() + database.z);
	    else
		v = field->w(vit.x() + database.x, 
			    vit.y() + database.y, 
			    vit.z() + database.z);

	    vit.setValue(v);
	}
    }
}

GA_RWAttributeRef
f3d_createAttribute(GEO_Detail *gdp, const char *name, float value)
{
    return gdp->addFloatTuple(GA_ATTRIB_PRIMITIVE, name, 1);
}

GA_RWAttributeRef
f3d_createAttribute(GEO_Detail *gdp, const char *name, Field3D::V3f value)
{
    return gdp->addFloatTuple(GA_ATTRIB_PRIMITIVE, name, 3);
}

GA_RWAttributeRef
f3d_createAttribute(GEO_Detail *gdp, const char *name, int value)
{
    return gdp->addIntTuple(GA_ATTRIB_PRIMITIVE, name, 1);
}

GA_RWAttributeRef
f3d_createAttribute(GEO_Detail *gdp, const char *name, Field3D::V3i value)
{
    return gdp->addIntTuple(GA_ATTRIB_PRIMITIVE, name, 3);
}

GA_RWAttributeRef
f3d_createAttribute(GEO_Detail *gdp, const char *name, std::string value)
{
    return gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, name, 1);
}

void
f3d_setAttribute(GA_RWAttributeRef &ar, GA_Offset offset, float value)
{
    GA_RWHandleF	h(ar);
    h.set(offset, value);
}

void
f3d_setAttribute(GA_RWAttributeRef &ar, GA_Offset offset, Field3D::V3f value)
{
    GA_RWHandleF	h(ar);
    
    h.set(offset, 0, value.x);
    h.set(offset, 1, value.x);
    h.set(offset, 2, value.x);
}

void
f3d_setAttribute(GA_RWAttributeRef &ar, GA_Offset offset, int value)
{
    GA_RWHandleI	h(ar);
    h.set(offset, value);
}

void
f3d_setAttribute(GA_RWAttributeRef &ar, GA_Offset offset, Field3D::V3i value)
{
    GA_RWHandleI	h(ar);
    
    h.set(offset, 0, value.x);
    h.set(offset, 1, value.x);
    h.set(offset, 2, value.x);
}

void
f3d_setAttribute(GA_RWAttributeRef &ar, GA_Offset offset, std::string value)
{
    GA_RWHandleS	h(ar);
    
    h.set(offset, value.c_str());
}

template <typename T>
void
f3d_singleMetadataToPrimitive(GEO_Detail *gdp, GU_PrimVolume *vol,
		    const char *name, const T &value)
{
    UT_String			sname(name);

    if (sname.startsWith("_houdini_."))
    {
	// Special internal attribute.
	return;
    }

    GA_RWAttributeRef	aref = gdp->findAttribute(GA_ATTRIB_PRIMITIVE, name);
    if (!aref.isValid())
	aref = f3d_createAttribute(gdp, name, value);
    
    if (aref.isValid())
	f3d_setAttribute(aref, vol->getMapOffset(), value);
}

template <typename T>
void
f3d_metadataListToPrimitive(GEO_Detail *gdp, GU_PrimVolume *vol,
		    const T &metadata)
{
    typename T::const_iterator	i;

    for (i = metadata.begin(); i != metadata.end(); ++i)
    {
	f3d_singleMetadataToPrimitive(gdp, vol, i->first.c_str(), i->second);
    }
}

template <typename T>
void
f3d_vectorMetadataListToPrimitive(int vecidx, 
		    GEO_Detail *gdp, GU_PrimVolume *vol,
		    const T &metadata)
{
    typename T::const_iterator	i;

    for (i = metadata.begin(); i != metadata.end(); ++i)
    {
	if (vecidx == 0)
	    f3d_singleMetadataToPrimitive(gdp, vol, i->first.c_str(), i->second.x);
	else if (vecidx == 1)
	    f3d_singleMetadataToPrimitive(gdp, vol, i->first.c_str(), i->second.y);
	else
	    f3d_singleMetadataToPrimitive(gdp, vol, i->first.c_str(), i->second.z);
    }
}

template <typename FIELD_PTR>
void
f3d_metadataToPrimitive(GEO_Detail *gdp, GU_PrimVolume *vol,
			FIELD_PTR field)
{
    f3d_metadataListToPrimitive(gdp, vol, field->metadata().vecFloatMetadata());
    f3d_metadataListToPrimitive(gdp, vol, field->metadata().floatMetadata());
    f3d_metadataListToPrimitive(gdp, vol, field->metadata().vecIntMetadata());
    f3d_metadataListToPrimitive(gdp, vol, field->metadata().intMetadata());
    f3d_metadataListToPrimitive(gdp, vol, field->metadata().strMetadata());
}

template <typename FIELD_PTR>
void
f3d_vectorMetadataToPrimitive(int vecidx, GEO_Detail *gdp, GU_PrimVolume *vol,
			FIELD_PTR field)
{
    f3d_vectorMetadataListToPrimitive(
	vecidx, gdp, vol, field->metadata().vecFloatMetadata());
    f3d_metadataListToPrimitive(
	gdp, vol, field->metadata().floatMetadata());
    f3d_vectorMetadataListToPrimitive(
	vecidx, gdp, vol, field->metadata().vecIntMetadata());
    f3d_metadataListToPrimitive(
	gdp, vol, field->metadata().intMetadata());
    f3d_metadataListToPrimitive(
	gdp, vol, field->metadata().strMetadata());
}

template <typename T>
void
f3d_LoadFields(GEO_Detail *gdp, Field3D::Field3DInputFile &infile, UT_FprealArray &primsortlist)
{
    GA_RWHandleS name_gah(gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, "name", 1));

    typename Field3D::Field<T>::Vec		scalarfields;
    scalarfields = infile.readScalarLayers<T>();

    typename Field3D::Field<T>::Vec::const_iterator i = scalarfields.begin();
    for (; i != scalarfields.end(); ++i)
    {
	UT_String		name((**i).name);
	UT_String		attribute((**i).attribute);

	int			rx, ry, rz;

	Field3D::V3i			rawres;
	Field3D::V3i			database;

	rawres = (**i).dataResolution();
	database = (**i).dataWindow().min;
	rx = rawres.x;
	ry = rawres.y;
	rz = rawres.z;

	// Rebuild our attribute as Cd.x provided it differs from the name.
	if (name != attribute)
	{
	    name += ".";
	    name += attribute;
	}

	GU_PrimVolume		*vol;

	vol = (GU_PrimVolume *)GU_PrimVolume::build((GU_Detail *)gdp);

	f3d_metadataToPrimitive(gdp, vol, (*i));

	// Look for houdini specific attributes.
	float		taperx = (*i)->metadata().floatMetadata("_houdini_.taperx", 1.0);
	float		tapery = (*i)->metadata().floatMetadata("_houdini_.tapery", 1.0);
	vol->setTaperX(taperx);
	vol->setTaperY(tapery);
	int		primnum = (*i)->metadata().intMetadata("_houdini_.primnum", -1);

	if (primnum == -1)
	    primnum = primsortlist.entries();
	primsortlist.append(primnum);


	// Set the name of the primitive
	name_gah.set(vol->getMapOffset(), name);

	f3d_getTransformFromField((*i), vol, false, 0, 0);

	UT_VoxelArrayWriteHandleF	handle = vol->getVoxelWriteHandle();

	// Set the compression options to 16 bit if the source is.
	if (sizeof(T) == sizeof(fpreal16))
	{
	    UT_VoxelCompressOptions options = handle->getCompressionOptions();
	    options.myAllowFP16 = true;
	    handle->setCompressionOptions(options);
	}

	// Resize the array.
	handle->size(rx, ry, rz);

	// Support several different reading options.  If we can't
	// cast to a type we know, we just use the field interface
	typename Field3D::DenseField<T>::Ptr dense_field = Field3D::field_dynamic_cast< Field3D::DenseField<T> > (*i);
	typename Field3D::SparseField<T>::Ptr sparse_field = Field3D::field_dynamic_cast< Field3D::SparseField<T> > (*i);

	if (dense_field)
	{
	    f3d_loadDenseField(&*handle, dense_field);
	}
	else if (sparse_field)
	{
	    f3d_loadSparseField(&*handle, sparse_field.get());
	}
	else
	{
	    f3d_loadField(&*handle, *i);
	}
    }

    typename Field3D::Field< FIELD3D_VEC3_T<T> >::Vec	vectorfields;
    vectorfields = infile.readVectorLayers<T>();
    for (typename Field3D::Field< FIELD3D_VEC3_T<T> >::Vec::const_iterator i = vectorfields.begin(); i != vectorfields.end(); ++i)
    {
	UT_String		name((**i).name);
	UT_String		attribute((**i).attribute);

	int			rx, ry, rz;

	Field3D::V3i			rawres;
	Field3D::V3i			database;

	rawres = (**i).dataResolution();
	database = (**i).dataWindow().min;
	rx = rawres.x;
	ry = rawres.y;
	rz = rawres.z;

	// Rebuild our attribute as Cd.x provided it differs from the name.
	if (name != attribute)
	{
	    name += ".";
	    name += attribute;
	}

	GU_PrimVolume			*vol[3];
	UT_VoxelArrayWriteHandleF	 handle[3];
	UT_VoxelArrayF			*vox[3];

	// Support several different reading options.  If we can't
	// cast to a type we know, we just use the field interface
	typename Field3D::DenseField< FIELD3D_VEC3_T<T> >::Ptr dense_field = Field3D::field_dynamic_cast< Field3D::DenseField< FIELD3D_VEC3_T<T> > > (*i);
	typename Field3D::SparseField< FIELD3D_VEC3_T<T> >::Ptr sparse_field = Field3D::field_dynamic_cast< Field3D::SparseField< FIELD3D_VEC3_T<T> > > (*i);
	typename Field3D::MACField< FIELD3D_VEC3_T<T> >::Ptr mac_field = Field3D::field_dynamic_cast< Field3D::MACField< FIELD3D_VEC3_T<T> > > (*i);

	// Set the name of the primitive
	for (int j = 0; j < 3; j++)
	{
	    vol[j] = (GU_PrimVolume *)GU_PrimVolume::build((GU_Detail *)gdp);

	    // All must share the same meta data.
	    // Except if we are working with vector metadata, which we
	    // will auto-separate.
	    f3d_vectorMetadataToPrimitive(j, gdp, vol[j], (*i));

	    float	taperx = (*i)->metadata().floatMetadata("_houdini_.taperx", 1.0);
	    float	tapery = (*i)->metadata().floatMetadata("_houdini_.tapery", 1.0);
	    vol[j]->setTaperX(taperx);
	    vol[j]->setTaperY(tapery);
	    int		primnum = (*i)->metadata().intMetadata("_houdini_.primnum", -1);
	    // We ensure the unrolled vector fields show up in order.
	    if (primnum == -1)
		primnum = primsortlist.entries();
	    primsortlist.append(primnum + (j * 0.3));

	    UT_String		nameext;
	    char		ext[2] = "x";
	    int			res[3];

	    nameext = name;
	    nameext += ".";
	    ext[0] += j;
	    nameext += ext;

	    name_gah.set(vol[j]->getMapOffset(), nameext);

	    res[0] = rx;
	    res[1] = ry;
	    res[2] = rz;

	    f3d_getTransformFromField((*i), vol[j], mac_field != 0, j, res[j]);

	    handle[j] = vol[j]->getVoxelWriteHandle();

	    // Set the compression options to 16 bit if the source is.
	    if (sizeof(T) == sizeof(fpreal16))
	    {
		UT_VoxelCompressOptions options = handle[j]->getCompressionOptions();
		options.myAllowFP16 = true;
		handle[j]->setCompressionOptions(options);
	    }

	    // Resize the array.

	    // Mac fields get an extra resolution.
	    if (mac_field)
		res[j]++;
	    handle[j]->size(res[0], res[1], res[2]);

	    vox[j] = &*handle[j];
	}

	if (dense_field)
	{
	    f3d_loadDenseField(vox, dense_field);
	}
	else if (sparse_field)
	{
	    f3d_loadDenseField(vox, sparse_field);
	}
	else if (mac_field)
	{
	    f3d_loadMACField(vox, mac_field);
	}
	else
	{
	    f3d_loadField(vox, *i);
	}
    }
}

template <typename FIELD_PTR>
void
f3d_primitiveToMetadata(const GEO_Detail *gdp, const GEO_PrimVolume *vol, const GEO_PrimVolume *vol2, const GEO_PrimVolume *vol3, FIELD_PTR field)
{
    for (GA_AttributeDict::iterator it = gdp->primitiveAttribs().begin();
	 !it.atEnd(); ++it)
    {
	const GA_Attribute *atr = it.attrib();

	if (!strcmp(atr->getName(), "name"))
	    continue;

	std::string		 aname = (const char *) atr->getName();
	GA_Offset		 prim_offset = vol->getMapOffset();

	if (atr->getStorageClass() == GA_STORECLASS_STRING)
	{
	    GA_ROHandleS	h(atr);
	    UT_String		str(h.get(prim_offset));
	    
	    if (str.isstring())
	    {
		std::string		avalue = (const char *) str;
		field->metadata().setStrMetadata(aname, avalue);
	    }
	}
	else if (atr->getStorageClass() == GA_STORECLASS_FLOAT)
	{
	    GA_ROHandleF	h(atr);
	    
	    if (atr->getTupleSize() == 1)
	    {
		// However, if the incoming is a triple, we wish
		// to collate!
		if (vol2 && vol3)
		{
		    Field3D::V3f	mv;
		    mv.x = h.get(prim_offset);
		    mv.y = h.get(vol2->getMapOffset());
		    mv.z = h.get(vol3->getMapOffset());
		    field->metadata().setVecFloatMetadata(aname, mv);
		}
		else
		    field->metadata().setFloatMetadata(aname, 
						       h.get(prim_offset));
	    }
	    else
	    {
		Field3D::V3f	mv;

		mv.x = h.get(prim_offset, 0);
		mv.y = h.get(prim_offset, 1);
		mv.z = h.get(prim_offset, 2);
		field->metadata().setVecFloatMetadata(aname, mv);
	    }
	}
	else if (atr->getStorageClass() == GA_STORECLASS_INT)
	{
	    GA_ROHandleI	h(atr);
	    
	    // Integer.
	    if (atr->getTupleSize() == 1)
	    {
		// However, if the incoming is a triple, we wish
		// to collate!
		if (vol2 && vol3)
		{
		    Field3D::V3i	mv;
		    mv.x = h.get(prim_offset);
		    mv.y = h.get(vol2->getMapOffset());
		    mv.z = h.get(vol3->getMapOffset());
		    field->metadata().setVecIntMetadata(aname, mv);
		}
		else
		    field->metadata().setIntMetadata(aname,
						     h.get(prim_offset));
	    }
	    else
	    {
		Field3D::V3i	mv;

		mv.x = h.get(prim_offset, 0);
		mv.y = h.get(prim_offset, 1);
		mv.z = h.get(prim_offset, 2);
		field->metadata().setVecIntMetadata(aname, mv);
	    }
	}
    }
}

template <typename T, typename FIELD_PTR>
void
f3d_SaveField(Field3D::Field3DOutputFile &out, FIELD_PTR scalarfield, const GEO_Detail *gdp, const GEO_PrimVolume *vol)
{
    UT_String			 name, attribute;
    UT_WorkBuffer		 buf;
    
    GA_ROHandleS		 name_gah(gdp, GA_ATTRIB_PRIMITIVE, "name");

    int				 resx, resy, resz;
    UT_VoxelArrayReadHandleF     handle = vol->getVoxelHandle();

    // Default name
    buf.sprintf("volume_%" SYS_PRId64, exint(vol->getMapIndex()));
    name.harden(buf.buffer());

    // Which is overridden by any name attribute.
    if (name_gah.isValid())
	name = name_gah.get(vol->getMapOffset());

    // Save resolution
    vol->getRes(resx, resy, resz);
    scalarfield->setSize(Field3D::V3i(resx, resy, resz));
    scalarfield->clear(0.0f);

    if (name.fileExtension())
    {
	// Split the name/attribute according to the . so
	// Cd.x will go into Name Cd and Attribute x
	UT_String		namewithoutextension;

	namewithoutextension = name.pathUpToExtension();
	scalarfield->name = (const char *) namewithoutextension;
	// +1 to get past the .
	scalarfield->attribute = name.fileExtension()+1;
    }
    else
    {
	// Use the doubling rule for plain names
	scalarfield->name = (const char *) name;
	scalarfield->attribute = (const char *) name;
    }

    // Write out all of our non-name primitive attributes as
    // metadata
    f3d_primitiveToMetadata(gdp, vol, 0, 0, scalarfield);

    f3d_getTransformFromVolume(vol, scalarfield);

    // Add our houdini specific attributes.
    scalarfield->metadata().setIntMetadata("_houdini_.primnum", vol->getMapIndex());
    scalarfield->metadata().setFloatMetadata("_houdini_.taperx", vol->getTaperX());
    scalarfield->metadata().setFloatMetadata("_houdini_.tapery", vol->getTaperY());

    typename Field3D::DenseField<T>::Ptr densefield = Field3D::field_dynamic_cast< Field3D::DenseField<T> > (scalarfield);
    typename Field3D::SparseField<T>::Ptr sparsefield = Field3D::field_dynamic_cast< Field3D::SparseField<T> > (scalarfield);

    if (densefield)
    {
	UT_VoxelArrayIteratorF		vit;
	vit.setHandle(handle);
	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
	    densefield->fastLValue(vit.x(), vit.y(), vit.z()) = vit.getValue();
	}
    }
    else if (sparsefield)
    {
	// ensure we have our desired block size.
	sparsefield->setBlockOrder(4);

	// Currently we always have a datawindow of 0, so our
	// blocks should be aligned.

	typename Field3D::SparseField<T>::block_iterator bi = sparsefield->blockBegin();

	for (; bi != sparsefield->blockEnd(); ++bi)
	{
	    Field3D::V3i		bmin, bmax, bsize;
	    UT_VoxelTile<float>	*tile;

	    tile = handle->getTile(bi.x, bi.y, bi.z);

	    if (tile->isConstant())
	    {
		T	v = (*tile)(0, 0, 0);
		sparsefield->setBlockEmptyValue(bi.x, bi.y, bi.z, v);
	    }
	    else
	    {
		// A fully allocated block.
		bmin = bi.blockBoundingBox().min;
		bmax = bi.blockBoundingBox().max;
		bsize = bmax - bmin + Field3D::V3i(1);

		for (int z = 0; z < bsize.z; z++)
		{
		    for (int y = 0; y < bsize.y; y++)
		    {
			for (int x = 0; x < bsize.x; x++)
			{
			    T 		v;

			    v = (*tile)(x, y, z);

			    // This will recompute the block index which
			    // seems wasteful, but I'm unsure how to access
			    // the block in a raw manner.
			    sparsefield->fastLValue(x+bmin.x, y+bmin.y, z+bmin.z) = v;
			}
		    }
		}
	    }
	}
    }
    else
    {
	// use generic interface.
	UT_VoxelArrayIteratorF		vit;
	vit.setHandle(handle);
	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
	    scalarfield->lvalue(vit.x(), vit.y(), vit.z()) = vit.getValue();
	}
    }
    out.writeScalarLayer<T>(scalarfield);
}

template <typename T, typename FIELD_PTR>
void
f3d_SaveVectorField(Field3D::Field3DOutputFile &out, FIELD_PTR vectorfield, const GEO_Detail *gdp, const GEO_PrimVolume *vol[3])
{
    UT_String			 name, attribute;
    UT_WorkBuffer		 buf;
    
    GA_ROHandleS		 name_gah(gdp, GA_ATTRIB_PRIMITIVE, "name");
    
    int				 resx, resy, resz;
    int				 i;
    UT_VoxelArrayReadHandleF     handle[3];
    
    handle[0] = vol[0]->getVoxelHandle();
    handle[1] = vol[1]->getVoxelHandle();
    handle[2] = vol[2]->getVoxelHandle();

    // Default name
    buf.sprintf("volume_%" SYS_PRId64, exint(vol[0]->getMapIndex()));
    name.harden(buf.buffer());

    // Which is overridden by any name attribute.
    if (name_gah.isValid())
	name = name_gah.get(vol[0]->getMapOffset());

    if (name.fileExtension())
    {
	// Strip the .x....
	name = name.pathUpToExtension();
    }

    // Save resolution
    vol[0]->getRes(resx, resy, resz);
    vectorfield->setSize(Field3D::V3i(resx, resy, resz));

    FIELD3D_VEC3_T<T> zero;
    zero.x = 0.0;
    zero.y = 0.0;
    zero.z = 0.0;

    vectorfield->clear(zero);

    if (name.fileExtension())
    {
	// Split the name/attribute according to the . 
	UT_String		namewithoutextension;

	namewithoutextension = name.pathUpToExtension();
	vectorfield->name = (const char *) namewithoutextension;
	// +1 to get past the .
	vectorfield->attribute = name.fileExtension()+1;
    }
    else
    {
	// Use the doubling rule for plain names
	vectorfield->name = (const char *) name;
	vectorfield->attribute = (const char *) name;
    }

    // Write out all of our non-name primitive attributes as
    // metadata
    f3d_primitiveToMetadata(gdp, vol[0], vol[1], vol[2], vectorfield);

    f3d_getTransformFromVolume(vol[0], vectorfield);

    // Add our houdini specific attributes.
    vectorfield->metadata().setIntMetadata("_houdini_.primnum", vol[0]->getMapIndex());
    vectorfield->metadata().setFloatMetadata("_houdini_.taperx", vol[0]->getTaperX());
    vectorfield->metadata().setFloatMetadata("_houdini_.tapery", vol[0]->getTaperY());

    typename Field3D::DenseField<FIELD3D_VEC3_T<T> >::Ptr densefield = Field3D::field_dynamic_cast< Field3D::DenseField<FIELD3D_VEC3_T<T> > > (vectorfield);
    typename Field3D::SparseField<FIELD3D_VEC3_T<T> >::Ptr sparsefield = Field3D::field_dynamic_cast< Field3D::SparseField<FIELD3D_VEC3_T<T> > > (vectorfield);

    if (densefield)
    {
	UT_VoxelArrayIteratorF		vit[3];
	for (i = 0; i < 3; i++)
	{
	    vit[i].setHandle(handle[i]);
	    vit[i].rewind();
	}
	for (; !vit[0].atEnd(); )
	{
	    FIELD3D_VEC3_T<T>	value;

	    value.x = vit[0].getValue();
	    value.y = vit[1].getValue();
	    value.z = vit[2].getValue();
	    densefield->fastLValue(vit[0].x(), vit[0].y(), vit[0].z()) = value;

	    for (i = 0; i < 3; i++)
		vit[i].advance();
	}
    }
    else if (sparsefield)
    {
	// ensure we have our desired block size.
	sparsefield->setBlockOrder(4);

	// Currently we always have a datawindow of 0, so our
	// blocks should be aligned.

	typename Field3D::SparseField<FIELD3D_VEC3_T<T> >::block_iterator bi = sparsefield->blockBegin();

	for (; bi != sparsefield->blockEnd(); ++bi)
	{
	    Field3D::V3i		bmin, bmax, bsize;
	    UT_VoxelTile<float>		*tile[3];

	    for (i = 0; i < 3; i++)
		tile[i] = handle[i]->getTile(bi.x, bi.y, bi.z);

	    // Only if they are all constant can we make a constant
	    // vector tile.
	    if (tile[0]->isConstant() &&
		tile[1]->isConstant() &&
		tile[2]->isConstant())
	    {
		FIELD3D_VEC3_T<T>	v;
		v.x = (*tile[0])(0, 0, 0);
		v.y = (*tile[1])(0, 0, 0);
		v.z = (*tile[2])(0, 0, 0);
		sparsefield->setBlockEmptyValue(bi.x, bi.y, bi.z, v);
	    }
	    else
	    {
		// A fully allocated block.
		bmin = bi.blockBoundingBox().min;
		bmax = bi.blockBoundingBox().max;
		bsize = bmax - bmin + Field3D::V3i(1);

		for (int z = 0; z < bsize.z; z++)
		{
		    for (int y = 0; y < bsize.y; y++)
		    {
			for (int x = 0; x < bsize.x; x++)
			{
			    FIELD3D_VEC3_T<T>	v;

			    v.x = (*tile[0])(x, y, z);
			    v.y = (*tile[1])(x, y, z);
			    v.z = (*tile[2])(x, y, z);

			    // This will recompute the block index which
			    // seems wasteful, but I'm unsure how to access
			    // the block in a raw manner.
			    sparsefield->fastLValue(x+bmin.x, y+bmin.y, z+bmin.z) = v;
			}
		    }
		}
	    }
	}
    }
    else
    {
	// use generic interface.
	UT_VoxelArrayIteratorF		vit[3];
	for (i = 0; i < 3; i++)
	{
	    vit[i].setHandle(handle[i]);
	    vit[i].rewind();
	}
	for (; !vit[0].atEnd(); )
	{
	    FIELD3D_VEC3_T<T>	value;

	    value.x = vit[0].getValue();
	    value.y = vit[1].getValue();
	    value.z = vit[2].getValue();
	    vectorfield->lvalue(vit[0].x(), vit[0].y(), vit[0].z()) = value;

	    for (i = 0; i < 3; i++)
		vit[i].advance();
	}
    }
    out.writeVectorLayer<T>(vectorfield);
}

bool
f3d_SaveCollated(Field3D::Field3DOutputFile &out, 
		    F3D_BitDepth bitdepth,
		    F3D_GridType gridtype,
		    const GEO_Detail *gdp, 
		    GA_Offset xnum, GA_Offset ynum, GA_Offset znum)
{
    const GEO_PrimVolume		*vol[3];
    
    vol[0] = (const GEO_PrimVolume *)gdp->getPrimitive(xnum);
    vol[1] = (const GEO_PrimVolume *)gdp->getPrimitive(ynum);
    vol[2] = (const GEO_PrimVolume *)gdp->getPrimitive(znum);

    // Verify resolutions match.
    // TODO: Also look for MAC grids here.
    int			resx, resy, resz;
    int			oresx, oresy, oresz;

    vol[0]->getRes(resx, resy, resz);
    vol[1]->getRes(oresx, oresy, oresz);
    if (resx != oresx || resy != oresy || resz != oresz)
    {
	// Not matching, so can't collate.
	return false;
    }
    vol[2]->getRes(oresx, oresy, oresz);
    if (resx != oresx || resy != oresy || resz != oresz)
    {
	// Not matching, so can't collate.
	return false;
    }

    // Now invoke the proper templated method
    UT_VoxelArrayReadHandleF     handle = vol[0]->getVoxelHandle();

    F3D_BitDepth		desireddepth;

    if (bitdepth == F3D_BITDEPTH_AUTO)
    {
	UT_VoxelCompressOptions	options = handle->getCompressionOptions();

	desireddepth = F3D_BITDEPTH_FLOAT;
	if (options.myAllowFP16)
	    desireddepth = F3D_BITDEPTH_HALF;
    }
    else
	desireddepth = bitdepth;

    if (desireddepth == F3D_BITDEPTH_HALF)
    {
	if (gridtype == F3D_GRIDTYPE_DENSE)
	{
	    Field3D::DenseField< FIELD3D_VEC3_T<Field3D::half> >::Ptr	vectorfield(new Field3D::DenseField< FIELD3D_VEC3_T<Field3D::half> >);
	    f3d_SaveVectorField<Field3D::half>(out, vectorfield, gdp, vol);
	}
	else if (gridtype == F3D_GRIDTYPE_SPARSE)
	{
	    Field3D::SparseField< FIELD3D_VEC3_T<Field3D::half> >::Ptr	vectorfield(new Field3D::SparseField< FIELD3D_VEC3_T<Field3D::half> >);
	    f3d_SaveVectorField<Field3D::half>(out, vectorfield, gdp, vol);
	}
    }
    else if (desireddepth == F3D_BITDEPTH_FLOAT)
    {
	if (gridtype == F3D_GRIDTYPE_DENSE)
	{
	    Field3D::DenseField<FIELD3D_VEC3_T<float> >::Ptr	vectorfield(new Field3D::DenseField<FIELD3D_VEC3_T<float> >);
	    f3d_SaveVectorField<float>(out, vectorfield, gdp, vol);
	}
	else if (gridtype == F3D_GRIDTYPE_SPARSE)
	{
	    Field3D::SparseField<FIELD3D_VEC3_T<float> >::Ptr	vectorfield(new Field3D::SparseField<FIELD3D_VEC3_T<float> >);
	    f3d_SaveVectorField<float>(out, vectorfield, gdp, vol);
	}
    }
    else if (desireddepth == F3D_BITDEPTH_DOUBLE)
    {
	if (gridtype == F3D_GRIDTYPE_DENSE)
	{
	    Field3D::DenseField<FIELD3D_VEC3_T<double> >::Ptr	vectorfield(new Field3D::DenseField<FIELD3D_VEC3_T<double> >);
	    f3d_SaveVectorField<double>(out, vectorfield, gdp, vol);
	}
	else if (gridtype == F3D_GRIDTYPE_SPARSE)
	{
	    Field3D::SparseField<FIELD3D_VEC3_T<double> >::Ptr	vectorfield(new Field3D::SparseField<FIELD3D_VEC3_T<double> >);
	    f3d_SaveVectorField<double>(out, vectorfield, gdp, vol);
	}
    }

    return true;
}


namespace HDK_Sample {

GA_Detail::IOStatus
f3d_fileLoad(GEO_Detail *gdp, const char *fname)
{
    Field3D::Field3DInputFile infile;

    if (!infile.open(fname))
    {
	std::cerr << "Error: Failed to open " << fname << " as a Field3D file.\n";
	return false;
    }

    UT_FprealArray primsortlist;

    f3d_LoadFields<Field3D::half>(gdp, infile, primsortlist);
    f3d_LoadFields<float>(gdp, infile, primsortlist);
    f3d_LoadFields<double>(gdp, infile, primsortlist);


    UT_ASSERT(primsortlist.entries() == gdp->getNumPrimitives());

    if (primsortlist.entries() == gdp->getNumPrimitives())
    {
	static_cast<GU_Detail *>(gdp)->sortPrimitiveList(primsortlist.array());
    }

    // All done successfully
    return true;
}

GA_Detail::IOStatus
f3d_fileSave(const GEO_Detail *gdp, const char *fname,
		F3D_BitDepth bitdepth,
		F3D_GridType gridtype,
		bool collatevector)
{
    // Write our magic token.
    Field3D::Field3DOutputFile out;

    out.create(fname);

    // Now, for each volume in our gdp...
    UT_Set<GA_Offset> processed;

    GA_ROHandleS name_gah(gdp, GA_ATTRIB_PRIMITIVE, "name");

    // If name_gah isn't valid, we can't collate.
    if (collatevector && name_gah.isValid())
    {
	// First build a lookup of all of our names so we can
	// swiftly find .y & .z given a .x without triggering some
	// O(n^2) stuff.
	UT_StringMap<GA_Offset> name_lut;

        const GEO_Primitive *prim;
	GA_FOR_ALL_PRIMITIVES(gdp, prim)
	{
	    if (prim->getTypeId() != GA_PRIMVOLUME)
		continue;
	    
	    GA_Offset prim_off = prim->getMapOffset();
            const char *name = name_gah.get(prim_off);
	    name_lut[name] = prim_off;
	}

	// Now, find any .x primitives.
	GA_FOR_ALL_PRIMITIVES(gdp, prim)
	{
	    if (prim->getPrimitiveId() != GEO_PrimTypeCompat::GEOPRIMVOLUME)
		continue;

	    GA_Offset		prim_off = prim->getMapOffset();
	    
	    UT_String		name(name_gah.get(prim_off));

	    if (name.fileExtension() && !strcmp(name.fileExtension(), ".x"))
	    {
		UT_String	othername;
		GA_Offset	xnum, ynum, znum;
		
		xnum = prim->getMapOffset();
		ynum = GA_INVALID_OFFSET;
		znum = GA_INVALID_OFFSET;

		othername = name.pathUpToExtension();
		othername += ".y";
                auto it = name_lut.find(othername);
		if (it != name_lut.end())
		    ynum = it->second;

		othername = name.pathUpToExtension();
		othername += ".z";
                it = name_lut.find(othername);
		if (it != name_lut.end())
		    znum = it->second;

		if (GAisValid(ynum) && GAisValid(znum))
		{
		    // Yay, we have a matching set of volumes.
		    // If our attempt to save succeeds, we'll mark them
		    // all as processed.
		    if (f3d_SaveCollated(out, bitdepth, gridtype, 
					gdp, xnum, ynum, znum))
		    {
			processed.insert(xnum);
			processed.insert(ynum);
			processed.insert(znum);
		    }
		}
	    }
	}
    }

    const GEO_Primitive *prim;
    GA_FOR_ALL_PRIMITIVES(gdp, prim)
    {
	GA_Offset prim_off = prim->getMapOffset();
	
	if (processed.find(prim_off) != processed.end())
	    continue;

	processed.insert(prim_off);

	if (prim->getTypeId() == GA_PRIMVOLUME)
	{
	    const GEO_PrimVolume *vol = static_cast<const GEO_PrimVolume *>(prim);
	    UT_VoxelArrayReadHandleF handle = vol->getVoxelHandle();

	    F3D_BitDepth desireddepth;

	    if (bitdepth == F3D_BITDEPTH_AUTO)
	    {
		UT_VoxelCompressOptions	options = handle->getCompressionOptions();

		desireddepth = F3D_BITDEPTH_FLOAT;
		if (options.myAllowFP16)
		    desireddepth = F3D_BITDEPTH_HALF;
	    }
	    else
		desireddepth = bitdepth;

	    if (desireddepth == F3D_BITDEPTH_HALF)
	    {
		if (gridtype == F3D_GRIDTYPE_DENSE)
		{
		    Field3D::DenseField<Field3D::half>::Ptr	scalarfield(new Field3D::DenseField<Field3D::half>);
		    f3d_SaveField<Field3D::half>(out, scalarfield, gdp, vol);
		}
		else if (gridtype == F3D_GRIDTYPE_SPARSE)
		{
		    Field3D::SparseField<Field3D::half>::Ptr	scalarfield(new Field3D::SparseField<Field3D::half>);
		    f3d_SaveField<Field3D::half>(out, scalarfield, gdp, vol);
		}
	    }
	    else if (desireddepth == F3D_BITDEPTH_FLOAT)
	    {
		if (gridtype == F3D_GRIDTYPE_DENSE)
		{
		    Field3D::DenseField<float>::Ptr	scalarfield(new Field3D::DenseField<float>);
		    f3d_SaveField<float>(out, scalarfield, gdp, vol);
		}
		else if (gridtype == F3D_GRIDTYPE_SPARSE)
		{
		    Field3D::SparseField<float>::Ptr	scalarfield(new Field3D::SparseField<float>);
		    f3d_SaveField<float>(out, scalarfield, gdp, vol);
		}
	    }
	    else if (desireddepth == F3D_BITDEPTH_DOUBLE)
	    {
		if (gridtype == F3D_GRIDTYPE_DENSE)
		{
		    Field3D::DenseField<double>::Ptr	scalarfield(new Field3D::DenseField<double>);
		    f3d_SaveField<double>(out, scalarfield, gdp, vol);
		}
		else if (gridtype == F3D_GRIDTYPE_SPARSE)
		{
		    Field3D::SparseField<double>::Ptr	scalarfield(new Field3D::SparseField<double>);
		    f3d_SaveField<double>(out, scalarfield, gdp, vol);
		}
	    }
	}
    }

    return true;
}

}
