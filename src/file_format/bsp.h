
#pragma once

#include <vec3.hpp>
typedef unsigned char	byte;

// NOTE: These are stored in a short in the engine now.  Don't use more than 16 bits
#define	SURF_LIGHT		0x0001		// value will hold the light strength
#define	SURF_SKY2D		0x0002		// don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS		0x0010
#define SURF_NOPORTAL	0x0020	// the surface can not have a portal placed on it
#define	SURF_TRIGGER	0x0040	// FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
#define	SURF_NODRAW		0x0080	// don't bother referencing the texture

#define	SURF_HINT		0x0100	// make a primary bsp splitter

#define	SURF_SKIP		0x0200	// completely ignore, allowing non-closed brushes
#define SURF_NOLIGHT	0x0400	// Don't calculate light
#define SURF_BUMPLIGHT	0x0800	// calculate three lightmaps for the surface for bumpmapping
#define SURF_NOSHADOWS	0x1000	// Don't receive shadows
#define SURF_NODECALS	0x2000	// Don't receive decals
#define SURF_NOCHOP		0x4000	// Don't subdivide patches on this surface
#define SURF_HITBOX		0x8000	// surface is part of a hitbox

// little-endian "VBSP"
#define BSPIDENT	(('P'<<24)+('S'<<16)+('B'<<8)+'V')

enum
{
	LUMP_ENTITIES					= 0,	// *
	LUMP_PLANES						= 1,	// *
	LUMP_TEXDATA					= 2,	// *
	LUMP_VERTEXES					= 3,	// *
	LUMP_VISIBILITY					= 4,	// *
	LUMP_NODES						= 5,	// *
	LUMP_TEXINFO					= 6,	// *
	LUMP_FACES						= 7,	// *
	LUMP_LIGHTING					= 8,	// *
	LUMP_OCCLUSION					= 9,
	LUMP_LEAFS						= 10,	// *
	LUMP_FACEIDS					= 11,
	LUMP_EDGES						= 12,	// *
	LUMP_SURFEDGES					= 13,	// *
	LUMP_MODELS						= 14,	// *
	LUMP_WORLDLIGHTS				= 15,	//
	LUMP_LEAFFACES					= 16,	// *
	LUMP_LEAFBRUSHES				= 17,	// *
	LUMP_BRUSHES					= 18,	// *
	LUMP_BRUSHSIDES					= 19,	// *
	LUMP_AREAS						= 20,	// *
	LUMP_AREAPORTALS				= 21,	// *
	LUMP_UNUSED0					= 22,
	LUMP_UNUSED1					= 23,
	LUMP_UNUSED2					= 24,
	LUMP_UNUSED3					= 25,
	LUMP_DISPINFO					= 26,
	LUMP_ORIGINALFACES				= 27,
	LUMP_PHYSDISP					= 28,
	LUMP_PHYSCOLLIDE				= 29,
	LUMP_VERTNORMALS				= 30,
	LUMP_VERTNORMALINDICES			= 31,
	LUMP_DISP_LIGHTMAP_ALPHAS		= 32,
	LUMP_DISP_VERTS					= 33,		// CDispVerts
	LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
												//     For each lightmap sample
												//         byte for index
												//         if 255, then index = next byte + 255
												//         3 bytes for barycentric coordinates
	// The game lump is a method of adding game-specific lumps
	// FIXME: Eventually, all lumps could use the game lump system
	LUMP_GAME_LUMP					= 35,
	LUMP_LEAFWATERDATA				= 36,
	LUMP_PRIMITIVES					= 37,
	LUMP_PRIMVERTS					= 38,
	LUMP_PRIMINDICES				= 39,
	// A pak file can be embedded in a .bsp now, and the file system will search the pak
	//  file first for any referenced names, before deferring to the game directory
	//  file system/pak files and finally the base directory file system/pak files.
	LUMP_PAKFILE					= 40,
	LUMP_CLIPPORTALVERTS			= 41,
	// A map can have a number of cubemap entities in it which cause cubemap renders
	// to be taken after running vrad.
	LUMP_CUBEMAPS					= 42,
	LUMP_TEXDATA_STRING_DATA		= 43,
	LUMP_TEXDATA_STRING_TABLE		= 44,
	LUMP_OVERLAYS					= 45,
	LUMP_LEAFMINDISTTOWATER			= 46,
	LUMP_FACE_MACRO_TEXTURE_INFO	= 47,
	LUMP_DISP_TRIS					= 48,
	LUMP_PHYSCOLLIDESURFACE			= 49,	// deprecated.  We no longer use win32-specific havok compression on terrain
	LUMP_WATEROVERLAYS              = 50,
	LUMP_LEAF_AMBIENT_INDEX_HDR		= 51,	// index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
	LUMP_LEAF_AMBIENT_INDEX         = 52,	// index of LUMP_LEAF_AMBIENT_LIGHTING

	// optional lumps for HDR
	LUMP_LIGHTING_HDR				= 53,
	LUMP_WORLDLIGHTS_HDR			= 54,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR	= 55,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.
	LUMP_LEAF_AMBIENT_LIGHTING		= 56,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.

	LUMP_XZIPPAKFILE				= 57,   // deprecated. xbox 1: xzip version of pak file
	LUMP_FACES_HDR					= 58,	// HDR maps may have different face data.
	LUMP_MAP_FLAGS                  = 59,   // extended level-wide flags. not present in all levels
	LUMP_OVERLAY_FADES				= 60,	// Fade distances for overlays
};

struct bsplump_t
{
	int		fileofs, filelen;
	int		version;		// default to zero
	char	fourCC[4];		// default to ( char )0, ( char )0, ( char )0, ( char )0
};

#define	HEADER_LUMPS		64
struct bspheader_t
{
	int			ident;
	int			version;
	bsplump_t	lumps[HEADER_LUMPS];
	int			mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)
};

// planes (x&~1) and (x&~1)+1 are always opposites
struct bspplane_t
{
	glm::vec3	normal;
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};

struct bsptexdata_t
{
//	DECLARE_BYTESWAP_DATADESC();
	glm::vec3	reflectivity;
	int			nameStringTableID;				// index into g_StringTable for the texture name
	int			width, height;					// source image
	int			view_width, view_height;		//
};

struct bspnode_t
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
	short			area;		// If all leaves below this node are in the same area, then
								// this is the area index. If not, this is -1.
};

struct bsptexinfo_t
{
	//float		textureVecsTexelsPerWorldUnits[2][4];			// [s/t][xyz offset]
	glm::vec3 textureVecsS;
	float textureOffsS;
	glm::vec3 textureVecsT;
	float textureOffsT;
	//float		lightmapVecsLuxelsPerWorldUnits[2][4];			// [s/t][xyz offset] - length is in units of texels/area
	glm::vec3  lightmapVecsS;
	float lightmapOffsS;
	glm::vec3  lightmapVecsT;
	float lightmapOffsT;
	int			flags;				// miptex flags + overrides
	int			texdata;			// Pointer to texture name, size, etc.
};

#define	MAXLIGHTMAPS	4

struct bspface_t
{
	unsigned short	planenum;
	byte		side;	// faces opposite to the node's plane direction
	byte		onNode; // 1 if on node, 0 if in leaf

	int			firstedge;		// we must support > 64k edges
	short		numedges;
	short		texinfo;
	// This is a union under the assumption that a fog volume boundary (ie. water surface)
	// isn't a displacement map.
	// FIXME: These should be made a union with a flags or type field for which one it is
	// if we can add more to this.
//	union
//	{
		short       dispinfo;
		// This is only for surfaces that are the boundaries of fog volumes
		// (ie. water surfaces)
		// All of the rest of the surfaces can look at their leaf to find out
		// what fog volume they are in.
		short		surfaceFogVolumeID;
//	};

	// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
	float       area;

	// TODO: make these unsigned chars?
	int			m_LightmapTextureMinsInLuxels[2];
	int			m_LightmapTextureSizeInLuxels[2];

	int origFace;				// reference the original face this face was derived from


public:

/*	unsigned short GetNumPrims() const;
	void SetNumPrims( unsigned short nPrims );
	bool AreDynamicShadowsEnabled();
	void SetDynamicShadowsEnabled( bool bEnabled );
*/
	// non-polygon primitives (strips and lists)
private:
	unsigned short m_NumPrims;	// Top bit, if set, disables shadows on this surface (this is why there are accessors).

public:
	unsigned short	firstPrimID;

	unsigned int	smoothingGroups;
};

// compressed color format
struct ColorRGBExp32
{
	byte r, g, b;
	signed char exponent;
};

// helps get the offset of a bitfield
#define BEGIN_BITFIELD( name ) \
	union \
	{ \
		char name; \
		struct \
		{

#define END_BITFIELD() \
		}; \
	};

struct CompressedLightCube
{
	ColorRGBExp32 m_Color[6];
};

// version 0
struct bspleaf_version_0_t
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

BEGIN_BITFIELD( bf )
	short			area:9;
	short			flags:7;			// Per leaf flags.
END_BITFIELD()

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water

	// Precaculated light info for entities.
	CompressedLightCube m_AmbientLighting;
};

// version 1
struct bspleaf_t
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

BEGIN_BITFIELD( bf )
	short			area:9;
	short			flags:7;			// Per leaf flags.
END_BITFIELD()

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water
};

struct bspedge_t
{
	unsigned short	v[2];		// vertex numbers
};

struct bspmodel_t
{
	glm::vec3		mins, maxs;
	glm::vec3		origin;					// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct bspareaportal_t
{
	unsigned short	m_PortalKey;		// Entities have a key called portalnumber (and in vbsp a variable
									// called areaportalnum) which is used
									// to bind them to the area portals by comparing with this value.

	unsigned short	otherarea;		// The area this portal looks into.

	unsigned short	m_FirstClipPortalVert;	// Portal geometry.
	unsigned short	m_nClipPortalVerts;

	int				planenum;
};


struct bsparea_t
{
	int		numareaportals;
	int		firstareaportal;
};

// These denote where one dispinfo fits on another.
// Note: tables are generated based on these indices so make sure to update
//       them if these indices are changed.
typedef enum
{
	CORNER_TO_CORNER=0,
	CORNER_TO_MIDPOINT=1,
	MIDPOINT_TO_CORNER=2
} NeighborSpan;


// These define relative orientations of displacement neighbors.
typedef enum
{
	ORIENTATION_CCW_0=0,
	ORIENTATION_CCW_90=1,
	ORIENTATION_CCW_180=2,
	ORIENTATION_CCW_270=3
} NeighborOrientation;


// NOTE: see the section above titled "displacement neighbor rules".
struct CDispSubNeighbor
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	unsigned short		GetNeighborIndex() const		{ return m_iNeighbor; }
	NeighborSpan		GetSpan() const					{ return (NeighborSpan)m_Span; }
	NeighborSpan		GetNeighborSpan() const			{ return (NeighborSpan)m_NeighborSpan; }
	NeighborOrientation	GetNeighborOrientation() const	{ return (NeighborOrientation)m_NeighborOrientation; }

	bool				IsValid() const				{ return m_iNeighbor != 0xFFFF; }
	void				SetInvalid()				{ m_iNeighbor = 0xFFFF; }


public:
	unsigned short		m_iNeighbor;		// This indexes into ddispinfos.
											// 0xFFFF if there is no neighbor here.

	unsigned char		m_NeighborOrientation;		// (CCW) rotation of the neighbor wrt this displacement.

	// These use the NeighborSpan type.
	unsigned char		m_Span;						// Where the neighbor fits onto this side of our displacement.
	unsigned char		m_NeighborSpan;				// Where we fit onto our neighbor.
};


// NOTE: see the section above titled "displacement neighbor rules".
class CDispNeighbor
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	void				SetInvalid()	{ m_SubNeighbors[0].SetInvalid(); m_SubNeighbors[1].SetInvalid(); }
	
	// Returns false if there isn't anything touching this edge.
	bool				IsValid()		{ return m_SubNeighbors[0].IsValid() || m_SubNeighbors[1].IsValid(); }


public:
	// Note: if there is a neighbor that fills the whole side (CORNER_TO_CORNER),
	//       then it will always be in CDispNeighbor::m_Neighbors[0]
	CDispSubNeighbor	m_SubNeighbors[2];
};

// Max # of neighboring displacement touching a displacement's corner.
#define MAX_DISP_CORNER_NEIGHBORS	4

class CDispCornerNeighbors
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	void			SetInvalid()	{ m_nNeighbors = 0; }


public:
	unsigned short	m_Neighbors[MAX_DISP_CORNER_NEIGHBORS];	// indices of neighbors.
	unsigned char	m_nNeighbors;
};


class CDispVert
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	glm::vec3		m_vVector;		// Vector field defining displacement volume.
	float		m_flDist;		// Displacement distances.
	float		m_flAlpha;		// "per vertex" alpha values.
};

#define DISPTRI_TAG_SURFACE			(1<<0)
#define DISPTRI_TAG_WALKABLE		(1<<1)
#define DISPTRI_TAG_BUILDABLE		(1<<2)
#define DISPTRI_FLAG_SURFPROP1		(1<<3)
#define DISPTRI_FLAG_SURFPROP2		(1<<4)
#define DISPTRI_TAG_REMOVE			(1<<5)

class CDispTri
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	unsigned short m_uiTags;		// Displacement triangle tags.
};

// upper design bounds
#define MIN_MAP_DISP_POWER		2	// Minimum and maximum power a displacement can be.
#define MAX_MAP_DISP_POWER		4	

#define NUM_DISP_POWER_VERTS(power)	( ((1 << (power)) + 1) * ((1 << (power)) + 1) )
#define NUM_DISP_POWER_TRIS(power)	( (1 << (power)) * (1 << (power)) * 2 )

#define MAX_DISPVERTS					NUM_DISP_POWER_VERTS( MAX_MAP_DISP_POWER )

class bspdispinfo_t
{
public:
//	DECLARE_BYTESWAP_DATADESC();
	int			NumVerts() const		{ return NUM_DISP_POWER_VERTS(power); }
	int			NumTris() const			{ return NUM_DISP_POWER_TRIS(power); }

public:
	glm::vec3		startPosition;						// start position used for orientation -- (added BSPVERSION 6)
	int			m_iDispVertStart;					// Index into LUMP_DISP_VERTS.
	int			m_iDispTriStart;					// Index into LUMP_DISP_TRIS.

    int         power;                              // power - indicates size of map (2^power + 1)
    int         minTess;                            // minimum tesselation allowed
    float       smoothingAngle;                     // lighting smoothing angle
    int         contents;                           // surface contents

	unsigned short	m_iMapFace;						// Which map face this displacement comes from.
	
	int			m_iLightmapAlphaStart;				// Index into ddisplightmapalpha.
													// The count is m_pParent->lightmapTextureSizeInLuxels[0]*m_pParent->lightmapTextureSizeInLuxels[1].

	int			m_iLightmapSamplePositionStart;		// Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.

	CDispNeighbor			m_EdgeNeighbors[4];		// Indexed by NEIGHBOREDGE_ defines.
	CDispCornerNeighbors	m_CornerNeighbors[4];	// Indexed by CORNER_ defines.

//	enum unnamed { ALLOWEDVERTS_SIZE = PAD_NUMBER( MAX_DISPVERTS, 32 ) / 32 };
	#define ALLOWEDVERTS_SIZE 10
	unsigned long	m_AllowedVerts[ALLOWEDVERTS_SIZE];	// This is built based on the layout and sizes of our neighbors
														// and tells us which vertices are allowed to be active.
};

#define GAMELUMP_MAKE_CODE(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d) << 0)
#define GAMELUMP_STATIC_PROPS GAMELUMP_MAKE_CODE('s', 'p', 'r', 'p')

struct bspgamelump_t
{
	//DECLARE_BYTESWAP_DATADESC();
	int	id;
	unsigned short	flags;
	unsigned short	version;
	int				fileofs;
	int				filelen;
};

enum
{
	STATIC_PROP_NAME_LENGTH  = 128,

	// Flags field
	// These are automatically computed
	STATIC_PROP_FLAG_FADES	= 0x1,
	STATIC_PROP_USE_LIGHTING_ORIGIN	= 0x2,
	STATIC_PROP_NO_DRAW = 0x4,	// computed at run time based on dx level

	// These are set in WC
	STATIC_PROP_IGNORE_NORMALS	= 0x8,
	STATIC_PROP_NO_SHADOW	= 0x10,
	STATIC_PROP_SCREEN_SPACE_FADE	= 0x20,

	STATIC_PROP_NO_PER_VERTEX_LIGHTING = 0x40,				// in vrad, compute lighting at
															// lighting origin, not for each vertex

	STATIC_PROP_NO_SELF_SHADOWING = 0x80,					// disable self shadowing in vrad

	STATIC_PROP_WC_MASK		= 0xd8,							// all flags settable in hammer (?)
};

struct StaticPropLumpBase_t
{
	glm::vec3		m_Origin;
	glm::vec3		m_Angles;
	unsigned short	m_PropType;
	unsigned short	m_FirstLeaf;
	unsigned short	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	glm::vec3		m_LightingOrigin;
};

struct bspcubemapsample_t
{
	int			origin[3];			// position of light snapped to the nearest integer
									// the filename for the vtf file is derived from the position
	unsigned char size;				// 0 - default
									// otherwise, 1<<(size-1)
};

// lights that were used to illuminate the world
enum emittype_t
{
	emit_surface,		// 90 degree spotlight
	emit_point,			// simple point light source
	emit_spotlight,		// spotlight with penumbra
	emit_skylight,		// directional light with no falloff (surface must trace to SKY texture)
	emit_quakelight,	// linear falloff, non-lambertian
	emit_skyambient,	// spherical light source with no falloff (surface must trace to SKY texture)
};

struct bspworldlight_t
{
	glm::vec3		origin;
	glm::vec3		intensity;
	glm::vec3		normal;			// for surfaces and spotlights
	int			cluster;
	emittype_t	type;
	int			style;
	float		stopdot;		// start of penumbra for emit_spotlight
	float		stopdot2;		// end of penumbra for emit_spotlight
	float		exponent;		//
	float		radius;			// cutoff distance
	// falloff for emit_spotlight + emit_point:
	// 1 / (constant_attn + linear_attn * dist + quadratic_attn * dist^2)
	float		constant_attn;
	float		linear_attn;
	float		quadratic_attn;
	int			flags;			// Uses a combination of the DWL_FLAGS_ defines.
	int			texinfo;		//
	int			owner;			// entity that this light it relative to
};

struct bspleafambientlighting_t
{
	CompressedLightCube	cube;
	byte x;		// fixed point fraction of leaf bounds
	byte y;		// fixed point fraction of leaf bounds
	byte z;		// fixed point fraction of leaf bounds
	byte pad;	// unused
};

struct bspleafambientindex_t
{
	unsigned short ambientSampleCount;
	unsigned short firstAmbientSample;
};

