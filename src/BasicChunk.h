#ifndef	BasicChunk_H
#define BasicChunk_H

#include "OryxMeshData.h"
#include "Chunk.h"
#include "ChunkOptions.h"

class BasicChunkGenerator;

/** A simple chunk */
class BasicChunk : public Chunk
{
public:
	
	BasicChunk(BasicChunkGenerator* gen, InterChunkCoords pos);
	virtual ~BasicChunk();

	/** Does either a full build or lighting only
	 *		@param full Whether or not to do a full build or just 
	 *			do lighting */
	void build(bool full);

	/** Blacks out all lighting, used prior to a lighting update */
	void clearLighting();

	/** Does full lighting, and spreads light into any chunks in the set, if secondaryLight then
	 *		also gather light from neighbors not in the set
	 *	@param chunks The map of chunks that are getting updated (light can spread into
	 *		and out of any of these during this update) 
	 *	@param secondaryLight Whether to gather secondary lighting from surrounding chunks
	 *		(that aren't included in 'chunks') */
	virtual void calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight);

	/** Change the value of a block
	 *	@param chng the change to make (use the ChunkCoords 'data'
	 *		field to specify what to change to)
	 *	@remarks This will not be instantaneous it will get updated when
	 *		the background thread gets to it... */
	void changeBlock(const ChunkCoords& chng);

	/** Attempt to set a light value (it will only do so if it is brighter
	 *		what is already there) and return whether or not it worked.
	 *	@param coords the coordinates at which to set the light
	 *	@param val The light value to try 
	 *	@return If it replaced the light value*/
	inline bool setLight(ChunkCoords coords, byte val)
	{
		//boost::mutex::scoped_lock lock(mLightMutex);

		if(light[coords.x][coords.y][coords.z] >= val)
		{
			return false;
		}
		else
		{
			light[coords.x][coords.y][coords.z] = val;
			lightDirty = true;
			return true;
		}
	}

	/*inline void forceSetLight(ChunkCoords coords, byte val)
	{
		light[coords.x][coords.y][coords.z] = val;
		if(coords.x == 0 && neighbors[0])
			neighbors[0]->lightDirty = true;
		if(coords.x == 15 && neighbors[1])
			neighbors[1]->lightDirty = true;
		if(coords.z == 0 && neighbors[4])
			neighbors[4]->lightDirty = true;
		if(coords.z == 15 && neighbors[5])
			neighbors[5]->lightDirty = true;
	}*/

	inline bool setLight(byte x, byte y, byte z, byte val)
	{
		if(light[x][y][z] >= val)
		{
			return false;
		}
		else
		{
			light[x][y][z] = val;
			return true;
		}
	}

	/** Gets the lighting at a point
	 *	@param x The x coordinate 
	 *	@param y The y coordinate 
	 *	@param z The z coordinate */
	inline byte getLightAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[x][y][z];
	}

	/** Gets the lighting at a point
	 *	@param c The coordinates */
	inline byte getLightAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[c.x][c.y][c.z];
	}

	/** Gets the lighting at a point
	 *	@param x The x coordinate 
	 *	@param y The y coordinate 
	 *	@param z The z coordinate */
	inline byte getBlockAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[x][y][z];
	}

	/** Gets the lighting at a point
	 *	@param c The coordinates */
	inline byte getBlockAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[c.x][c.y][c.z];
	}

	/** Sets this chunk as 'active' (actively being rendered/simulated) */
	bool activate();

	/** Sets this chunk as 'inactive' and halts any existing graphics/physics simulation. */
	void deactivate();

	/** Gets whether or not this is active */
	bool isActive();

	/** Clears extra light emiting blocks */
	void clearLights();

	/** Adds a light emitting block */
	void addLight(ChunkCoords c, byte strength);

	/** Explicity mark for relight (mainly used for portal light updates) */
	void needsRelight();

	/** So the chunk generator can get at this thing's data more easily */
	friend class BasicChunkGenerator;
	friend class TerrainChunkGenerator;
	friend class TerrainChunk;

	// TODO: protect this...
	BasicChunk* neighbors[6];

protected:

	/** Calculates lighting by recursively tracing through the chunks 
	 *	@param chunks The set of chunks being updated
	 *	@param coords The position in the current Chunk
	 *	@param lightVal The lighting to pass along
	 *	@param emitter If this is an emitting block */
	void doLighting(const std::map<BasicChunk*, bool>& chunks, 
		ChunkCoords coords, byte lightVal, bool emitter);

	/** Calculates lighting by recursively tracing through the chunks 
	 *	@param coords The position in the current Chunk
	 *	@param lightVal The lighting to pass along
	 *	@param emitter If this is an emitting block */
	void doLighting(ChunkCoords coords, byte lightVal, bool emitter);

	/** Helper that creates a quad 
	 *	@param chunkPos Which block we're dealing with 
	 *	@param realPos Object space position
	 *	@param direction Which face we're making 
	 *	@param blockType The texture atlas index for this blocktype
	 *	@param lighting Light value at this block
	 *	@param adjacentBlocks Whether or not adjacent blocks are occupied
	 *	@param adjacentLighting Lighting in adjacent blocks (used for smooth lighting)
	 *	@param lightOnly Whether to perform lighting only */
	void makeQuad(
		const ChunkCoords& chunkPos,
		const Vector3& realPos,
		const unsigned int& direction,
		const byte& blockType,
		const Real& lighting,
		const bool* adjacentBlocks,
		const Real* adjacentLighting,
		bool lightOnly);

	//---------------------------------------------------------------------------

	static inline BasicChunk* correctCoords(BasicChunk* c, ChunkCoords& coords)
	{
		if(!c)
			return c;
		for(int i = 0; i < 3; ++i)
		{
			if(coords[i] < 0)
			{
				coords[i] += CHUNKSIZE[i];
				return correctCoords(c->neighbors[i*2],coords);	
			}
			else if(coords[i] > CHUNKSIZE[i]-1) 
			{
				coords[i] -= CHUNKSIZE[i];
				return correctCoords(c->neighbors[i*2+1],coords);
			}
		}
		return c;
	}
	//---------------------------------------------------------------------------

	inline static byte getBlockAt(BasicChunk* c, ChunkCoords coords)
	{
		c = correctCoords(c,coords);
		return c ? c->blocks[coords.x][coords.y][coords.z] : 0;
	}	
	//---------------------------------------------------------------------------

	inline static byte getLightAt(BasicChunk* c, ChunkCoords coords)
	{
		c = correctCoords(c,coords);
		return c ? c->light[coords.x][coords.y][coords.z] : 0;
	}	
	//---------------------------------------------------------------------------

	// mutex to protect lighting data
	boost::mutex mLightMutex;

	// lighting data
	byte light[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	// the block data itself
	byte blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	// pending changes
	std::list<ChunkCoords> mChanges;
	std::map<ChunkCoords, ChunkChange> mConfirmedChanges;

	// light emitting blocks
	boost::mutex mLightListMutex;
	std::set<ChunkCoords> lights;
	bool lightDirty;
	// TODO: something for portals and associated light changes...
	
	bool mHasBeenActive;

	// the generator that created this
	BasicChunkGenerator* mBasicGenerator;

};

#endif
