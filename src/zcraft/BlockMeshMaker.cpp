/*
BlockMeshMaker.cpp
Copyright (C) 2010-2012 Marc GILLERON
This file is part of the zCraft project.
*/

#include <list>
#include "zcraft/BlockMeshMaker.hpp"
#include "zcraft/Block.hpp"
#include "zcraft/NodeProperties.hpp"
#include "zcraft/face.hpp"

using namespace engine;

namespace zcraft
{
	/*
		Static
	*/

	gl::VertexColorArray * BlockMeshMaker::makeMesh(
			const Vector3i bpos, VoxelBuffer & vb)
	{
		const Vector3i min = bpos * Block::SIZE;
		//const Vector3i max = min + Vector3i(1,1,1) * (Block::SIZE - 1);

		// Note : we don't know at advance how many faces we will find
		std::list<FastFace> faces;

		/*
			Compute fast faces

			o----o----o----o----o          o----o--------------o
			|\ A  \ B  \ B  \ B  \         |\ A  \      B       \
			o o----o----o----o----o  ===>  o o----o--------------o
			 \| A  | B  | B  | B  |         \| A  |      B       |
			  o----o----o----o----o          o----o--------------o

			y z
			 \|
			  o--x
		*/

		try
		{
			u32 x,y,z;

			// Go through every y,z and get north(y+) faces in rows of x+
			for(y = 0; y < Block::SIZE; y++)
			{
				for(z = 0; z < Block::SIZE; z++)
				{
					updateFastFacesRow(
						Vector3i(min.x, min.y+y, min.z+z),
						Vector3i(1,0,0),
						Vector3i(0,1,0),
						face::NORTH,
						Block::SIZE,
						vb, faces);
				}
			}

			// Go through every x,y and get east(x+) faces in rows of z+
			for(x = 0; x < Block::SIZE; x++)
			{
				for(y = 0; y < Block::SIZE; y++)
				{
					updateFastFacesRow(
						Vector3i(min.x+x, min.y+y, min.z),
						Vector3i(0,0,1),
						Vector3i(1,0,0),
						face::EAST,
						Block::SIZE,
						vb, faces);
				}
			}

			// Go through every y,z and get top(z+) faces in rows of x+
			for(z = 0; z < Block::SIZE; z++)
			{
				for(y = 0; y < Block::SIZE; y++)
				{
					updateFastFacesRow(
						Vector3i(min.x, min.y+y, min.z+z),
						Vector3i(1,0,0),
						Vector3i(0,0,1),
						face::TOP,
						Block::SIZE,
						vb, faces);
				}
			}
		}
		catch(std::exception & e)
		{
			std::cout << "EXCEPTION: BlockMeshMaker::makeMesh: "
				<< e.what() << std::endl;
			return nullptr;
		}

		// Make the mesh with collected faces

		gl::VertexColorArray * vbo = nullptr;

		if(!faces.empty())
		{
			// Convert faces into a mesh

			Array<f32> vertices(faces.size() * 4 * 3); // XYZ
			Array<u8> colors(faces.size() * 4 * 3); // RGB only

			u32 vi = 0, ci = 0, i = 0;
			for(auto & ff : faces)
			{
				for(i = 0; i < 4; i++)
				{
					vertices[vi] = ff.vertices[i].x;
					vertices[vi + 1] = ff.vertices[i].y;
					vertices[vi + 2] = ff.vertices[i].z;
					vi += 3;
				}

				for(i = 0; i < 16; i += 4)
				{
					colors[ci] = ff.colors[i];
					colors[ci + 1] = ff.colors[i + 1];
					colors[ci + 2] = ff.colors[i + 2];
					ci += 3;
				}
			}

			vbo = new gl::VertexColorArray();
			vbo->setPrimitiveType(GL_QUADS);
			vbo->moveVertices(vertices, 3);
			vbo->moveColors(colors, 3);
		}

		return vbo; // Remainder: Can be null
	}

	// fetch a row of blocks on a voxel vb to convert them into fastfaces
	// Note : tessellation algorithm is here
	void BlockMeshMaker::updateFastFacesRow(
			Vector3i startPos, // absolute start position
			Vector3i transDir, // iteration translation
			Vector3i vFaceDir, // face direction vector, ex : (0,0,1) for back face (z+)
			const u8 dir, // direction constant
			const u16 length, // usually Block::SIZE
			VoxelBuffer & vb, // voxels to analyse (may contain neighboring)
			std::list<FastFace> & faces)
	{
		// get textures
		//TextureManager* textures = TextureManager::getInstance();

		// relative positions
		Vector3i curPos;
		Vector3i nextPos0;
		Vector3i nextPos1;

		// current block
		Node nCur0 = vb.get(startPos);
		// block above the face of current block
		Node nCur1 = vb.get(startPos + vFaceDir);
		// next block
		Node nNext0;
		// block above the face of next block
		Node nNext1;

		u8 continuousFacesCount = 0;

		// for each voxels in the row
		for(u16 i = 0; i < length; i++)
		{
			bool nextIsDifferent = true;

			// at last position, the fastface is cut,
			// then no need to compare next blocks
			if(i != length - 1)
			{
				// getting next blocks
				nextPos0 = curPos + transDir;
				nextPos1 = nextPos0 + vFaceDir;
				nNext0 = vb.get(startPos + nextPos0);
				nNext1 = vb.get(startPos + nextPos1);

				// determining if the next block appearance is the same
				if( nCur0.type == nNext0.type &&
					nCur0.meta == nNext0.meta &&
					nCur1.type == nNext1.type &&
					nCur1.meta == nNext1.meta)
				{
					nextIsDifferent = false;
				}
			}

			++continuousFacesCount;

			if(nextIsDifferent)
			{
				// determining FastFace orientation
				u8 fc = compareFaces(nCur0, nCur1);

				// if a FastFace must be drawn
				if(fc != 0)
				{
					FastFace ff;

					// getting textures
					/*
					video::ITexture* texture0 =
						textures->getBlockTexture(nCur0, dir);
					video::ITexture* texture1 =
						textures->getBlockTexture(nCur1, game::block::oppositeFace(dir));
					*/
					sf::Texture * texture0 = nullptr;
					sf::Texture * texture1 = nullptr;

					// getting light
					const u8 light0 = getFaceLight(vFaceDir, nCur1.getLight());
					const u8 light1 = getFaceLight(-vFaceDir, nCur0.getLight());

					// fastface absolute start position
					Vector3i ffStart =
						startPos + curPos - transDir * (continuousFacesCount - 1);
					// to float position
					Vector3f fpos(ffStart.x, ffStart.y, ffStart.z);

					// scale
					Vector3f scale(1,1,1);
					if(transDir.x != 0)
						scale.x = continuousFacesCount;
					if(transDir.y != 0)
						scale.y = continuousFacesCount;
					if(transDir.z != 0)
						scale.z = continuousFacesCount;

					// updating fastface
					if(fc == 1)
					{
						updateFastFace(ff, texture0, light0, fpos,
							   vFaceDir, false, scale);
						faces.push_back(ff);
					}
					else if(fc == 2)
					{
						updateFastFace(ff, texture1, light1, fpos,
							   -vFaceDir, true, scale);
						faces.push_back(ff);
					}
				}
				continuousFacesCount = 0;

			} // if(nextIsDifferent)

			// shift next to current
			nCur0 = nNext0;
			nCur1 = nNext1;
			curPos += transDir;

		} // for each voxel [i] in the row
	}

	/*
		sets parameters of a FastFace.
		light in [0,255] (not a lightCode)
		shift : if true, the face will be shifted of (-vFaceDir).
	*/
	void BlockMeshMaker::updateFastFace(
			FastFace & ff, // the fastface to set
			sf::Texture * tex,
			u8 light, // light shade (for differenciating sides)
			Vector3f pos, // face absolute origin
			Vector3i vFaceDir, // direction vector
			bool shift,
			Vector3f scale) // scale of the face (integer)
	{
        // creating face's vertices
        Vector3f vertexPos[4];
        getFaceVerticesFromDirVector(vFaceDir, vertexPos);

        // normal
        // TODO fix normal in some cases (when the face is shifted)
        //Vector3f normal(vFaceDir.X, vFaceDir.Y, vFaceDir.Z);

        if(shift)
        {
            pos.x -= vFaceDir.x;
            pos.y -= vFaceDir.y;
            pos.z -= vFaceDir.z;
        }

        // apply scale and position
        for(u8 i = 0; i < 4; i++)
        {
            vertexPos[i].x *= scale.x;
            vertexPos[i].y *= scale.y;
            vertexPos[i].z *= scale.z;
            vertexPos[i] += pos;
        }

        // computing texture scale
        /*
        float tscale = 1;
        // Note : never use '==' on a float
        if     (scale.X < 0.999 || scale.X > 1.001) tscale = scale.X;
        else if(scale.Y < 0.999 || scale.Y > 1.001) tscale = scale.Y;
        else if(scale.Z < 0.999 || scale.Z > 1.001) tscale = scale.Z;
        */

        // color
        //video::SColor color = video::SColor(255, light, light, light);

		// TODO BlockMeshMaker: fix colors
		u8 ci = 0;
		for(u8 i = 0; i < 4; i++)
		{
			ff.vertices[i] = vertexPos[i];

			ff.colors[ci] = light;
			ff.colors[ci + 1] = light;
			ff.colors[ci + 2] = light;
			ff.colors[ci + 3] = 255;
			ci += 4;
		}

        // update final vertices
        // TODO FEATURE smooth lighting with a lightColor(light) function
        /*
        ff.vertices[0] = video::S3DVertex(vertexPos[0], normal, color,
            Vector2f(0, 1));
        ff.vertices[1] = video::S3DVertex(vertexPos[1], normal, color,
            Vector2f(1*tscale, 1));
        ff.vertices[2] = video::S3DVertex(vertexPos[2], normal, color,
            Vector2f(1*tscale, 0));
        ff.vertices[3] = video::S3DVertex(vertexPos[3], normal, color,
            Vector2f(0, 0));
		*/

        ff.texture = tex;
	}

	// get light shading of a face (to make differences between sides)
	u8 BlockMeshMaker::getFaceLight(const Vector3i & vdir, u8 lightCode)
	{
		float lightf = (float)lightCode / 16.f;

		if(lightf > 1)
			lightf = 1;
		if(lightf < 0)
			lightf = 0;

		if(vdir.x != 0) // EAST and WEST
		{
			lightf *= 0.78f;
		}
		else if(vdir.y != 0) // NORTH and SOUTH
		{
			lightf *= 0.56f;
		}
		else if(vdir.z == -1) // BOTTOM
		{
			lightf *= 0.34f;
		}
		else // TOP
		{
			lightf *= 1.f;
		}

		return lightf * 255.f;
	}

	u8 BlockMeshMaker::compareFaces(const Node a, const Node b)
	{
		if(a.type == node::AIR && b.type == node::AIR)
		{
			return 0; // Both empty
		}
		else if(a.properties().isOpaqueCube() && b.properties().isOpaqueCube())
		{
			return 0; // Both hidden
		}
		else if(a.type != node::AIR && b.type == node::AIR)
		{
			return 1; // first is solid
		}
		else
		{
			return 2; // second is solid
		}
	}

	// TODO BlockMeshMaker: fix vertex order (backface culling is wrong)
    // Note : vDir must ever have two zero coordinates. If not, it will fail.
    void BlockMeshMaker::getFaceVerticesFromDirVector(
			Vector3i vDir, Vector3f vertices[4])
    {
		u8 dir;

		if(vDir.x == -1)		dir = face::WEST;
		else if(vDir.x == 1)	dir = face::EAST;
		else if(vDir.y == -1)	dir = face::SOUTH;
		else if(vDir.y == 1)	dir = face::NORTH;
		else if(vDir.z == -1)	dir = face::BOTTOM;
		else					dir = face::TOP;

		switch(dir)
		{
		case face::BOTTOM :
			vertices[0] = Vector3f(0,0,0);
			vertices[1] = Vector3f(1,0,0);
			vertices[2] = Vector3f(1,1,0);
			vertices[3] = Vector3f(0,1,0);
			break;
		case face::TOP :
			vertices[0] = Vector3f(1,0,1);
			vertices[1] = Vector3f(0,0,1);
			vertices[2] = Vector3f(0,1,1);
			vertices[3] = Vector3f(1,1,1);
			break;
		case face::WEST :
			vertices[0] = Vector3f(0,0,1);
			vertices[1] = Vector3f(0,0,0);
			vertices[2] = Vector3f(0,1,0);
			vertices[3] = Vector3f(0,1,1);
			break;
		case face::EAST :
			vertices[0] = Vector3f(1,0,0);
			vertices[1] = Vector3f(1,0,1);
			vertices[2] = Vector3f(1,1,1);
			vertices[3] = Vector3f(1,1,0);
			break;
		case face::SOUTH :
			vertices[0] = Vector3f(0,0,1);
			vertices[1] = Vector3f(1,0,1);
			vertices[2] = Vector3f(1,0,0);
			vertices[3] = Vector3f(0,0,0);
			break;
		case face::NORTH :
			vertices[0] = Vector3f(0,1,0);
			vertices[1] = Vector3f(1,1,0);
			vertices[2] = Vector3f(1,1,1);
			vertices[3] = Vector3f(0,1,1);
			break;

		default :
			break;
		}
    }

} // namespace zcraft



