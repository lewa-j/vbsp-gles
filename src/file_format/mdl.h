
#pragma once

typedef glm::vec3 Vector;
typedef glm::vec2 Vector2D;
typedef glm::vec4 Quaternion;
typedef glm::vec3 RadianEuler;
typedef glm::mat3x4 matrix3x4_t;
typedef unsigned char byte;

// ----------------------------------------------------------
// Studio Model Vertex Data File
// Position independent flat data for cache manager
// ----------------------------------------------------------

// little-endian "IDSV"
#define MODEL_VERTEX_FILE_ID		(('V'<<24)+('S'<<16)+('D'<<8)+'I')
#define MODEL_VERTEX_FILE_VERSION	4
// this id (IDCV) is used once the vertex data has been compressed (see CMDLCache::CreateThinVertexes)
#define MODEL_VERTEX_FILE_THIN_ID	(('V'<<24)+('C'<<16)+('D'<<8)+'I')

#define MAX_NUM_LODS 8
#define MAX_NUM_BONES_PER_VERT 3

// 16 bytes
struct mstudioboneweight_t
{
//	DECLARE_BYTESWAP_DATADESC();
	float	weight[MAX_NUM_BONES_PER_VERT];
	char	bone[MAX_NUM_BONES_PER_VERT];
	byte	numbones;
};

// NOTE: This is exactly 48 bytes
struct mstudiovertex_t
{
//	DECLARE_BYTESWAP_DATADESC();
	mstudioboneweight_t	m_BoneWeights;
	Vector				m_vecPosition;
	Vector				m_vecNormal;
	Vector2D			m_vecTexCoord;

	mstudiovertex_t() {}

private:
	// No copy constructors allowed
	mstudiovertex_t(const mstudiovertex_t& vOther);
};

struct vertexFileHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int		id;								// MODEL_VERTEX_FILE_ID
	int		version;						// MODEL_VERTEX_FILE_VERSION
	long	checksum;						// same as studiohdr_t, ensures sync
	int		numLODs;						// num of valid lods
	int		numLODVertexes[MAX_NUM_LODS];	// num verts for desired root lod
	int		numFixups;						// num of vertexFileFixup_t
	int		fixupTableStart;				// offset from base to fixup table
	int		vertexDataStart;				// offset from base to vertex block
	int		tangentDataStart;				// offset from base to tangent block

/*public:

	// Accessor to fat vertex data
	const mstudiovertex_t *GetVertexData() const
	{
		if ( ( id == MODEL_VERTEX_FILE_ID ) && ( vertexDataStart != 0 ) )
			return ( mstudiovertex_t * ) ( vertexDataStart + (byte *)this );
		else
			return NULL;
	}
	// Accessor to (fat) tangent vertex data (tangents aren't stored in compressed data)
	const Vector4D *GetTangentData() const
	{
		if ( ( id == MODEL_VERTEX_FILE_ID ) && ( tangentDataStart != 0 ) )
			return ( Vector4D * ) ( tangentDataStart + (byte *)this );
		else
			return NULL;
	}
	// Accessor to thin vertex data
	const  thinModelVertices_t *GetThinVertexData() const
	{
		if ( ( id == MODEL_VERTEX_FILE_THIN_ID ) && ( vertexDataStart != 0 ) )
			return ( thinModelVertices_t * ) ( vertexDataStart + (byte *)this );
		else
			return NULL;
	}*/
};

// apply sequentially to lod sorted vertex and tangent pools to re-establish mesh order
struct vertexFileFixup_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int		lod;				// used to skip culled root lod
	int		sourceVertexID;		// absolute index from start of vertex/tangent blocks
	int		numVertexes;
};



struct mstudio_modelvertexdata_t
{
//	DECLARE_BYTESWAP_DATADESC();
/*	Vector				*Position( int i ) const;
	Vector				*Normal( int i ) const;
	Vector4D			*TangentS( int i ) const;
	Vector2D			*Texcoord( int i ) const;
	mstudioboneweight_t	*BoneWeights( int i ) const;
	mstudiovertex_t		*Vertex( int i ) const;
	bool				HasTangentData( void ) const;
	int					GetGlobalVertexIndex( int i ) const;
	int					GetGlobalTangentIndex( int i ) const;
*/
	// base of external vertex data stores
	const void			*pVertexData;
	const void			*pTangentData;
};

struct mstudio_meshvertexdata_t
{
//	DECLARE_BYTESWAP_DATADESC();
/*	Vector				*Position( int i ) const;
	Vector				*Normal( int i ) const;
	Vector4D			*TangentS( int i ) const;
	Vector2D			*Texcoord( int i ) const;
	mstudioboneweight_t *BoneWeights( int i ) const;
	mstudiovertex_t		*Vertex( int i ) const;
	bool				HasTangentData( void ) const;
	int					GetModelVertexIndex( int i ) const;
	int					GetGlobalVertexIndex( int i ) const;
*/
	// indirection to this mesh's model's vertex data
	const mstudio_modelvertexdata_t	*modelvertexdata;

	// used for fixup calcs when culling top level lods
	// expected number of mesh verts at desired lod
	int					numLODVertexes[MAX_NUM_LODS];
};

struct mstudiomesh_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int					material;

	int					modelindex;
//	mstudiomodel_t *pModel() const; 

	int					numvertices;		// number of unique vertices/normals/texcoords
	int					vertexoffset;		// vertex mstudiovertex_t

	// Access thin/fat mesh vertex data (only one will return a non-NULL result)
//	const mstudio_meshvertexdata_t	*GetVertexData(		void *pModelData = NULL );
//	const thinModelVertices_t		*GetThinVertexData(	void *pModelData = NULL );
	
	int					numflexes;			// vertex animation
	int					flexindex;
//	inline mstudioflex_t *pFlex( int i ) const { return (mstudioflex_t *)(((byte *)this) + flexindex) + i; };

	// special codes for material operations
	int					materialtype;
	int					materialparam;

	// a unique ordinal for this mesh
	int					meshid;

	Vector				center;

	mstudio_meshvertexdata_t vertexdata;

	int					unused[8]; // remove as appropriate

	mstudiomesh_t(){}
private:
	// No copy constructors allowed
	mstudiomesh_t(const mstudiomesh_t& vOther);
};

// studio models
struct mstudiomodel_t
{
//	DECLARE_BYTESWAP_DATADESC();
	inline const char * pszName( void ) const { return name; }
	char				name[64];

	int					type;

	float				boundingradius;

	int					nummeshes;	
	int					meshindex;
//	inline mstudiomesh_t *pMesh( int i ) const { return (mstudiomesh_t *)(((byte *)this) + meshindex) + i; };

	// cache purposes
	int					numvertices;		// number of unique vertices/normals/texcoords
	int					vertexindex;		// vertex Vector
	int					tangentsindex;		// tangents Vector

	// These functions are defined in application-specific code:
/*	const vertexFileHeader_t			*CacheVertexData(			void *pModelData );

	// Access thin/fat mesh vertex data (only one will return a non-NULL result)
	const mstudio_modelvertexdata_t		*GetVertexData(		void *pModelData = NULL );
	const thinModelVertices_t			*GetThinVertexData(	void *pModelData = NULL );
*/
	int					numattachments;
	int					attachmentindex;

	int					numeyeballs;
	int					eyeballindex;
//	inline  mstudioeyeball_t *pEyeball( int i ) { return (mstudioeyeball_t *)(((byte *)this) + eyeballindex) + i; };

	mstudio_modelvertexdata_t vertexdata;

	int					unused[8];		// remove as appropriate
};

struct mstudiobodyparts_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int					sznameindex;
//	inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
	int					nummodels;
	int					base;
	int					modelindex; // index into models array
//	inline mstudiomodel_t *pModel( int i ) const { return (mstudiomodel_t *)(((byte *)this) + modelindex) + i; };
};

//typedef int IMaterial;

// skin info
struct mstudiotexture_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int						sznameindex;
//	inline char * const		pszName( void ) const { return ((char *)this) + sznameindex; }
	int						flags;
	int						used;
    int						unused1;
	//mutable IMaterial		*material;  // fixme: this needs to go away . .isn't used by the engine, but is used by studiomdl
	int *mat;
	mutable void			*clientmaterial;	// gary, replace with client material pointer if used
	
	int						unused[10];
};

// bones
struct mstudiobone_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int					sznameindex;
//	inline char * const pszName( void ) const { return ((char *)this) + sznameindex; }
	int		 			parent;		// parent bone
	int					bonecontroller[6];	// bone controller index, -1 == none

	// default values
	Vector				pos;
	Quaternion			quat;
	RadianEuler			rot;
	// compression scale
	Vector				posscale;
	Vector				rotscale;
//96
	matrix3x4_t			poseToBone;
//144
	Quaternion			qAlignment;
	int					flags;
	int					proctype;
	int					procindex;		// procedural rule
	mutable int			physicsbone;	// index into physically simulated bone
//	inline void *pProcedure( ) const { if (procindex == 0) return NULL; else return  (void *)(((byte *)this) + procindex); };
	int					surfacepropidx;	// index into string tablefor property name
//	inline char * const pszSurfaceProp( void ) const { return ((char *)this) + surfacepropidx; }
	int					contents;		// See BSPFlags.h for the contents flags
//204
	int					unused[8];		// remove as appropriate

	mstudiobone_t(){}
private:
	// No copy constructors allowed
	mstudiobone_t(const mstudiobone_t& vOther);
};
//216 bytes

// sequence descriptions
struct mstudioseqdesc_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int					baseptr;
//	inline studiohdr_t	*pStudiohdr( void ) const { return (studiohdr_t *)(((byte *)this) + baseptr); }

	int					szlabelindex;
//	inline char * const pszLabel( void ) const { return ((char *)this) + szlabelindex; }

	int					szactivitynameindex;
//	inline char * const pszActivityName( void ) const { return ((char *)this) + szactivitynameindex; }

	int					flags;		// looping/non-looping flags

	int					activity;	// initialized at loadtime to game DLL values
	int					actweight;

	int					numevents;
	int					eventindex;
//	inline mstudioevent_t *pEvent( int i ) const { Assert( i >= 0 && i < numevents); return (mstudioevent_t *)(((byte *)this) + eventindex) + i; };

	Vector				bbmin;		// per sequence bounding box
	Vector				bbmax;

	int					numblends;

	// Index into array of shorts which is groupsize[0] x groupsize[1] in length
	int					animindexindex;

/*	inline int			anim( int x, int y ) const
	{
		if ( x >= groupsize[0] )
		{
			x = groupsize[0] - 1;
		}

		if ( y >= groupsize[1] )
		{
			y = groupsize[ 1 ] - 1;
		}

		int offset = y * groupsize[0] + x;
		short *blends = (short *)(((byte *)this) + animindexindex);
		int value = (int)blends[ offset ];
		return value;
	}
*/
	int					movementindex;	// [blend] float array for blended movement
	int					groupsize[2];
	int					paramindex[2];	// X, Y, Z, XR, YR, ZR
	float				paramstart[2];	// local (0..1) starting value
	float				paramend[2];	// local (0..1) ending value
	int					paramparent;

	float				fadeintime;		// ideal cross fate in time (0.2 default)
	float				fadeouttime;	// ideal cross fade out time (0.2 default)

	int					localentrynode;		// transition node at entry
	int					localexitnode;		// transition node at exit
	int					nodeflags;		// transition rules

	float				entryphase;		// used to match entry gait
	float				exitphase;		// used to match exit gait

	float				lastframe;		// frame that should generation EndOfSequence

	int					nextseq;		// auto advancing sequences
	int					pose;			// index of delta animation between end and nextseq

	int					numikrules;

	int					numautolayers;	//
	int					autolayerindex;
//	inline mstudioautolayer_t *pAutolayer( int i ) const { Assert( i >= 0 && i < numautolayers); return (mstudioautolayer_t *)(((byte *)this) + autolayerindex) + i; };

	int					weightlistindex;
//	inline float		*pBoneweight( int i ) const { return ((float *)(((byte *)this) + weightlistindex) + i); };
//	inline float		weight( int i ) const { return *(pBoneweight( i)); };

	// FIXME: make this 2D instead of 2x1D arrays
	int					posekeyindex;
//	float				*pPoseKey( int iParam, int iAnim ) const { return (float *)(((byte *)this) + posekeyindex) + iParam * groupsize[0] + iAnim; }
//	float				poseKey( int iParam, int iAnim ) const { return *(pPoseKey( iParam, iAnim )); }

	int					numiklocks;
	int					iklockindex;
//	inline mstudioiklock_t *pIKLock( int i ) const { Assert( i >= 0 && i < numiklocks); return (mstudioiklock_t *)(((byte *)this) + iklockindex) + i; };

	// Key values
	int					keyvalueindex;
	int					keyvaluesize;
//	inline const char * KeyValueText( void ) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

	int					cycleposeindex;		// index of pose parameter to use as cycle index

	int					unused[7];		// remove/add as appropriate (grow back to 8 ints on version change!)

	mstudioseqdesc_t(){}
private:
	// No copy constructors allowed
	mstudioseqdesc_t(const mstudioseqdesc_t& vOther);
};

struct studiohdr_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int					id;
	int					version;

	long				checksum;		// this has to be the same in the phy and vtx files to load!
	
	inline const char *	pszName( void ) const { return name; }
	char				name[64];
	int					length;


	Vector				eyeposition;	// ideal eye position

	Vector				illumposition;	// illumination center
	
	Vector				hull_min;		// ideal movement hull size
	Vector				hull_max;			

	Vector				view_bbmin;		// clipping bounding box
	Vector				view_bbmax;		

	int					flags;

	int					numbones;			// bones
	int					boneindex;
/*	inline mstudiobone_t *pBone( int i ) const { Assert( i >= 0 && i < numbones); return (mstudiobone_t *)(((byte *)this) + boneindex) + i; };
	int					RemapSeqBone( int iSequence, int iLocalBone ) const;	// maps local sequence bone to global bone
	int					RemapAnimBone( int iAnim, int iLocalBone ) const;		// maps local animations bone to global bone
*/
	int					numbonecontrollers;		// bone controllers
	int					bonecontrollerindex;
//	inline mstudiobonecontroller_t *pBonecontroller( int i ) const { Assert( i >= 0 && i < numbonecontrollers); return (mstudiobonecontroller_t *)(((byte *)this) + bonecontrollerindex) + i; };

	int					numhitboxsets;
	int					hitboxsetindex;

	// Look up hitbox set by index
/*	mstudiohitboxset_t	*pHitboxSet( int i ) const 
	{ 
		Assert( i >= 0 && i < numhitboxsets); 
		return (mstudiohitboxset_t *)(((byte *)this) + hitboxsetindex ) + i; 
	};

	// Calls through to hitbox to determine size of specified set
	inline mstudiobbox_t *pHitbox( int i, int set ) const 
	{ 
		mstudiohitboxset_t const *s = pHitboxSet( set );
		if ( !s )
			return NULL;

		return s->pHitbox( i );
	};

	// Calls through to set to get hitbox count for set
	inline int			iHitboxCount( int set ) const
	{
		mstudiohitboxset_t const *s = pHitboxSet( set );
		if ( !s )
			return 0;

		return s->numhitboxes;
	};
*/
	// file local animations? and sequences
//private:
	int					numlocalanim;			// animations/poses
	int					localanimindex;		// animation descriptions
//  	inline mstudioanimdesc_t *pLocalAnimdesc( int i ) const { if (i < 0 || i >= numlocalanim) i = 0; return (mstudioanimdesc_t *)(((byte *)this) + localanimindex) + i; };

	int					numlocalseq;				// sequences
	int					localseqindex;
/*  	inline mstudioseqdesc_t *pLocalSeqdesc( int i ) const { if (i < 0 || i >= numlocalseq) i = 0; return (mstudioseqdesc_t *)(((byte *)this) + localseqindex) + i; };

//public:
	bool				SequencesAvailable() const;
	int					GetNumSeq() const;
	mstudioanimdesc_t	&pAnimdesc( int i ) const;
	mstudioseqdesc_t	&pSeqdesc( int i ) const;
	int					iRelativeAnim( int baseseq, int relanim ) const;	// maps seq local anim reference to global anim index
	int					iRelativeSeq( int baseseq, int relseq ) const;		// maps seq local seq reference to global seq index
*/
//private:
	mutable int			activitylistversion;	// initialization flag - have the sequences been indexed?
	mutable int			eventsindexed;
//public:
/*	int					GetSequenceActivity( int iSequence );
	void				SetSequenceActivity( int iSequence, int iActivity );
	int					GetActivityListVersion( void ) const;
	void				SetActivityListVersion( int version ) const;
	int					GetEventListVersion( void ) const;
	void				SetEventListVersion( int version ) const;
*/	
	// raw textures
	int					numtextures;
	int					textureindex;
//	inline mstudiotexture_t *pTexture( int i ) const { Assert( i >= 0 && i < numtextures ); return (mstudiotexture_t *)(((byte *)this) + textureindex) + i; }; 


	// raw textures search paths
	int					numcdtextures;
	int					cdtextureindex;
//	inline char			*pCdtexture( int i ) const { return (((char *)this) + *((int *)(((byte *)this) + cdtextureindex) + i)); };

	// replaceable textures tables
	int					numskinref;
	int					numskinfamilies;
	int					skinindex;
//	inline short		*pSkinref( int i ) const { return (short *)(((byte *)this) + skinindex) + i; };

	int					numbodyparts;		
	int					bodypartindex;
//	inline mstudiobodyparts_t	*pBodypart( int i ) const { return (mstudiobodyparts_t *)(((byte *)this) + bodypartindex) + i; };

	// queryable attachable points
//private:
	int					numlocalattachments;
	int					localattachmentindex;
/*	inline mstudioattachment_t	*pLocalAttachment( int i ) const { Assert( i >= 0 && i < numlocalattachments); return (mstudioattachment_t *)(((byte *)this) + localattachmentindex) + i; };
//public:
	int					GetNumAttachments( void ) const;
	const mstudioattachment_t &pAttachment( int i ) const;
	int					GetAttachmentBone( int i ) const;
	// used on my tools in hlmv, not persistant
	void				SetAttachmentBone( int iAttachment, int iBone );
*/
	// animation node to animation node transition graph
//private:
	int					numlocalnodes;
	int					localnodeindex;
	int					localnodenameindex;
//	inline char			*pszLocalNodeName( int iNode ) const { Assert( iNode >= 0 && iNode < numlocalnodes); return (((char *)this) + *((int *)(((byte *)this) + localnodenameindex) + iNode)); }
//	inline byte			*pLocalTransition( int i ) const { Assert( i >= 0 && i < (numlocalnodes * numlocalnodes)); return (byte *)(((byte *)this) + localnodeindex) + i; };

//public:
/*	int					EntryNode( int iSequence ) const;
	int					ExitNode( int iSequence ) const;
	char				*pszNodeName( int iNode ) const;
	int					GetTransition( int iFrom, int iTo ) const;
*/
	int					numflexdesc;
	int					flexdescindex;
//	inline mstudioflexdesc_t *pFlexdesc( int i ) const { Assert( i >= 0 && i < numflexdesc); return (mstudioflexdesc_t *)(((byte *)this) + flexdescindex) + i; };

	int					numflexcontrollers;
	int					flexcontrollerindex;
//	inline mstudioflexcontroller_t *pFlexcontroller( LocalFlexController_t i ) const { Assert( i >= 0 && i < numflexcontrollers); return (mstudioflexcontroller_t *)(((byte *)this) + flexcontrollerindex) + i; };

	int					numflexrules;
	int					flexruleindex;
//	inline mstudioflexrule_t *pFlexRule( int i ) const { Assert( i >= 0 && i < numflexrules); return (mstudioflexrule_t *)(((byte *)this) + flexruleindex) + i; };

	int					numikchains;
	int					ikchainindex;
//	inline mstudioikchain_t *pIKChain( int i ) const { Assert( i >= 0 && i < numikchains); return (mstudioikchain_t *)(((byte *)this) + ikchainindex) + i; };

	int					nummouths;
	int					mouthindex;
//	inline mstudiomouth_t *pMouth( int i ) const { Assert( i >= 0 && i < nummouths); return (mstudiomouth_t *)(((byte *)this) + mouthindex) + i; };

//private:
	int					numlocalposeparameters;
	int					localposeparamindex;
/*	inline mstudioposeparamdesc_t *pLocalPoseParameter( int i ) const { Assert( i >= 0 && i < numlocalposeparameters); return (mstudioposeparamdesc_t *)(((byte *)this) + localposeparamindex) + i; };
//public:
	int					GetNumPoseParameters( void ) const;
	const mstudioposeparamdesc_t &pPoseParameter( int i ) const;
	int					GetSharedPoseParameter( int iSequence, int iLocalPose ) const;
*/
	int					surfacepropindex;
//	inline char * const pszSurfaceProp( void ) const { return ((char *)this) + surfacepropindex; }

	// Key values
	int					keyvalueindex;
	int					keyvaluesize;
//	inline const char * KeyValueText( void ) const { return keyvaluesize != 0 ? ((char *)this) + keyvalueindex : NULL; }

	int					numlocalikautoplaylocks;
	int					localikautoplaylockindex;
//	inline mstudioiklock_t *pLocalIKAutoplayLock( int i ) const { Assert( i >= 0 && i < numlocalikautoplaylocks); return (mstudioiklock_t *)(((byte *)this) + localikautoplaylockindex) + i; };

/*	int					GetNumIKAutoplayLocks( void ) const;
	const mstudioiklock_t &pIKAutoplayLock( int i ) const;
	int					CountAutoplaySequences() const;
	int					CopyAutoplaySequences( unsigned short *pOut, int outCount ) const;
	int					GetAutoplayList( unsigned short **pOut ) const;
*/
	// The collision model mass that jay wanted
	float				mass;
	int					contents;

	// external animations, models, etc.
	int					numincludemodels;
	int					includemodelindex;
/*	inline mstudiomodelgroup_t *pModelGroup( int i ) const { Assert( i >= 0 && i < numincludemodels); return (mstudiomodelgroup_t *)(((byte *)this) + includemodelindex) + i; };
	// implementation specific call to get a named model
	const studiohdr_t	*FindModel( void **cache, char const *modelname ) const;

*/	// implementation specific back pointer to virtual data
	mutable void		*virtualModel;
//	virtualmodel_t		*GetVirtualModel( void ) const;

	// for demand loaded animation blocks
	int					szanimblocknameindex;	
//	inline char * const pszAnimBlockName( void ) const { return ((char *)this) + szanimblocknameindex; }
	int					numanimblocks;
	int					animblockindex;
//	inline mstudioanimblock_t *pAnimBlock( int i ) const { Assert( i > 0 && i < numanimblocks); return (mstudioanimblock_t *)(((byte *)this) + animblockindex) + i; };
	mutable void		*animblockModel;
//	byte *				GetAnimBlock( int i ) const;

	int					bonetablebynameindex;
//	inline const byte	*GetBoneTableSortedByName() const { return (byte *)this + bonetablebynameindex; }

	// used by tools only that don't cache, but persist mdl's peer data
	// engine uses virtualModel to back link to cache pointers
	void				*pVertexBase;
	void				*pIndexBase;

	// if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
	// this value is used to calculate directional components of lighting 
	// on static props
	byte				constdirectionallightdot;

	// set during load of mdl data to track *desired* lod configuration (not actual)
	// the *actual* clamped root lod is found in studiohwdata
	// this is stored here as a global store to ensure the staged loading matches the rendering
	byte				rootLOD;
	
	// set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
	// to be set as root LOD:
	//	numAllowedRootLODs = 0	means no restriction, any lod can be set as root lod.
	//	numAllowedRootLODs = N	means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
	byte				numAllowedRootLODs;

	byte				unused[1];

	int					unused4; // zero out if version < 47

	int					numflexcontrollerui;
	int					flexcontrolleruiindex;
//	mstudioflexcontrollerui_t *pFlexControllerUI( int i ) const { Assert( i >= 0 && i < numflexcontrollerui); return (mstudioflexcontrollerui_t *)(((byte *)this) + flexcontrolleruiindex) + i; }

	int					unused3[2];

	// FIXME: Remove when we up the model version. Move all fields of studiohdr2_t into studiohdr_t.
	int					studiohdr2index;
/*	studiohdr2_t*		pStudioHdr2() const { return (studiohdr2_t *)( ( (byte *)this ) + studiohdr2index ); }

	// Src bone transforms are transformations that will convert .dmx or .smd-based animations into .mdl-based animations
	int					NumSrcBoneTransforms() const { return studiohdr2index ? pStudioHdr2()->numsrcbonetransform : 0; }
	const mstudiosrcbonetransform_t* SrcBoneTransform( int i ) const { Assert( i >= 0 && i < NumSrcBoneTransforms()); return (mstudiosrcbonetransform_t *)(((byte *)this) + pStudioHdr2()->srcbonetransformindex) + i; }

	inline int			IllumPositionAttachmentIndex() const { return studiohdr2index ? pStudioHdr2()->IllumPositionAttachmentIndex() : 0; }

	inline float		MaxEyeDeflection() const { return studiohdr2index ? pStudioHdr2()->MaxEyeDeflection() : 0.866f; } // default to cos(30) if not set

	inline mstudiolinearbone_t *pLinearBones() const { return studiohdr2index ? pStudioHdr2()->pLinearBones() : NULL; }
*/
	// NOTE: No room to add stuff? Up the .mdl file format version 
	// [and move all fields in studiohdr2_t into studiohdr_t and kill studiohdr2_t],
	// or add your stuff to studiohdr2_t. See NumSrcBoneTransforms/SrcBoneTransform for the pattern to use.
	int					unused2[1];

	studiohdr_t() {}

private:
	// No copy constructors allowed
	studiohdr_t(const studiohdr_t& vOther);

//	friend struct virtualmodel_t;
};


#pragma pack(1)

struct vtxFileHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	// file version as defined by OPTIMIZED_MODEL_FILE_VERSION
	int version;

	// hardware params that affect how the model is to be optimized.
	int vertCacheSize;
	unsigned short maxBonesPerStrip;
	unsigned short maxBonesPerTri;
	int maxBonesPerVert;

	// must match checkSum in the .mdl
	long checkSum;

	int numLODs; // garymcthack - this is also specified in ModelHeader_t and should match

	// one of these for each LOD
	int materialReplacementListOffset;
/*	MaterialReplacementListHeader_t *pMaterialReplacementList( int lodID ) const
	{
		MaterialReplacementListHeader_t *pDebug =
			(MaterialReplacementListHeader_t *)(((byte *)this) + materialReplacementListOffset) + lodID;
		return pDebug;
	}
*/
	int numBodyParts;
	int bodyPartOffset;
/*	inline BodyPartHeader_t *pBodyPart( int i ) const
	{
		BodyPartHeader_t *pDebug = (BodyPartHeader_t *)(((byte *)this) + bodyPartOffset) + i;
		return pDebug;
	};*/
};

struct vtxBodyPartHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int numModels;
	int modelOffset;
/*	inline ModelHeader_t *pModel( int i ) const
	{
		ModelHeader_t *pDebug = (ModelHeader_t *)(((byte *)this) + modelOffset) + i;
		return pDebug;
	};*/
};

// This maps one to one with models in the mdl file.
// There are a bunch of model LODs stored inside potentially due to the qc $lod command
struct vtxModelHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int numLODs; // garymcthack - this is also specified in FileHeader_t
	int lodOffset;
/*	inline ModelLODHeader_t *pLOD( int i ) const
	{
		ModelLODHeader_t *pDebug = ( ModelLODHeader_t *)(((byte *)this) + lodOffset) + i;
		return pDebug;
	};*/
};

struct vtxModelLODHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int numMeshes;
	int meshOffset;
	float switchPoint;
/*	inline MeshHeader_t *pMesh( int i ) const
	{
		MeshHeader_t *pDebug = (MeshHeader_t *)(((byte *)this) + meshOffset) + i;
		return pDebug;
	};*/
};

// a collection of locking groups:
// up to 4:
// non-flexed, hardware skinned
// flexed, hardware skinned
// non-flexed, software skinned
// flexed, software skinned
//
// A mesh has a material associated with it.
struct vtxMeshHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	int numStripGroups;
	int stripGroupHeaderOffset;
	/*inline StripGroupHeader_t *pStripGroup( int i ) const
	{
		StripGroupHeader_t *pDebug = (StripGroupHeader_t *)(((byte *)this) + stripGroupHeaderOffset) + i;
		return pDebug;
	};*/
	unsigned char flags;
};

// a locking group
// a single vertex buffer
// a single index buffer
struct vtxStripGroupHeader_t
{
//	DECLARE_BYTESWAP_DATADESC();
	// These are the arrays of all verts and indices for this mesh.  strips index into this.
	int numVerts;
	int vertOffset;
/*	inline Vertex_t *pVertex( int i ) const 
	{ 
		return (Vertex_t *)(((byte *)this) + vertOffset) + i; 
	};
*/
	int numIndices;
	int indexOffset;
/*	inline unsigned short *pIndex( int i ) const 
	{ 
		return (unsigned short *)(((byte *)this) + indexOffset) + i; 
	};
*/
	int numStrips;
	int stripOffset;
/*	inline StripHeader_t *pStrip( int i ) const 
	{ 
		return (StripHeader_t *)(((byte *)this) + stripOffset) + i; 
	};
*/
	unsigned char flags;
};

struct vtxVertex_t
{
//	DECLARE_BYTESWAP_DATADESC();
	// these index into the mesh's vert[origMeshVertID]'s bones
	unsigned char boneWeightIndex[MAX_NUM_BONES_PER_VERT];
	unsigned char numBones;

	unsigned short origMeshVertID;

	// for sw skinned verts, these are indices into the global list of bones
	// for hw skinned verts, these are hardware bone indices
	char boneID[MAX_NUM_BONES_PER_VERT];
};
#pragma pack()

enum eRenderPass
{
	eRenderPass_NULL,
	eRenderPass_Opaque,
	eRenderPass_Transparent,
	eRenderPass_Refract
};

#include "graphics/ArrayBuffer.h"
#include "cull/BoundingBox.h"
#include "renderer/vbsp_mesh.h"
class IFile;

struct model_t
{
	studiohdr_t mdlheader;
	GLuint numVerts;
	mstudiovertex_t *verts;
	GLuint numMeshes;
	submesh_t *indsOfs;
	GLuint numInds;
	GLuint *inds;
	Material **materials;
	glm::vec3 rootbonerot;
	float dist;
	BoundingBox bbox;
	VertexBufferObject vbo;
	IndexBufferObject ibo;
	bool loaded;

	model_t()
	{
		numVerts = 0;
		verts = NULL;
		numMeshes = 0;
		numInds = 0;
		inds = NULL;
		materials = NULL;
		rootbonerot = glm::vec3(0.0f);
		loaded = false;
	}
	
//mdl_loader.cpp
	bool Load(const char *fileName, int lod=0);
	
//mdl_renderer.cpp
	void Draw();
	
	void Bind();
	bool DrawInst(eRenderPass pass);
	void Unbind();

private:
	void SetLOD(int l);
	vtxBodyPartHeader_t *ReadVTXBodyparts(IFile *vtxFile, vtxFileHeader_t &vtxHeader, int lod);
	void ClearVTXBodyparts(vtxBodyPartHeader_t *vtxbpheaders, int lod);
	int CalcIndsCount(vtxBodyPartHeader_t *vtxbpheaders, int lod);
	Material *FindMaterial(const char *name, std::string *paths, int numPaths);
};

