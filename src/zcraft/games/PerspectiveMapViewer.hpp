/*
PerspectiveMapViewer.hpp
Copyright (C) 2010-2012 Marc GILLERON
This file is part of the zCraft project.
*/

#ifndef ZCRAFT_PERSPECTIVEMAPVIEWER_HPP_INCLUDED
#define ZCRAFT_PERSPECTIVEMAPVIEWER_HPP_INCLUDED

#include "engine/BasicGame.hpp"
#include "engine/bmfont/Font.hpp"
#include "engine/opengl/VertexColorArray.hpp"
#include "zcraft/zcraft.hpp"
#include "zcraft/BlockMeshMap.hpp"
#include "zcraft/BlockMeshMaker.hpp"
#include "zcraft/MapStreamer.hpp"
#include "zcraft/IMapListener.hpp"
#include "zcraft/FreeCamera.hpp"

namespace zcraft
{
	/*
		This is a basic 3D map viewer (not really a game).
		It only consists on a controllable flying camera and
		an infinite map that keeps itself updated around.
	*/

	class PerspectiveMapViewer :
		public engine::ABasicGame,
		public IMapListener // To be notified of map changes through methods
	{
	private :

		FreeCamera m_camera;	// How to see
		engine::bmfont::Font m_font; // How to draw text
		BlockMap m_map; // Here are stored the voxels
		BlockMeshMap m_meshMap; // Here is the graphical representation of the map
		MapStreamer * m_mapStreamer = nullptr; // How to keep the map updated around us
		BlockMeshMaker m_meshMaker; // How to transform voxel data into visual things

	public :

		PerspectiveMapViewer(u32 width, u32 height);

		~PerspectiveMapViewer();

		void blockAdded(const Vector3i pos) override;

		void blockChanged(const Vector3i pos) override;

		void blockRemoved(const Vector3i pos) override;

	protected :

		bool init() override;

		void update(const engine::Time & delta) override;

		void render(const engine::Time & delta) override;

		void dispose() override;

		void resized(const Vector2i & newSize) override;

		void processEvent(const sf::Event & event) override;

	};

} // namespace zcraft


#endif // ZCRAFT_PERSPECTIVEMAPVIEWER_HPP_INCLUDED
