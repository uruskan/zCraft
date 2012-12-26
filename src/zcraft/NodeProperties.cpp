#include "zcraft/NodeProperties.hpp"
#include "zcraft/Node.hpp"

namespace zcraft
{
	/* Static */

	NodeProperties s_list[node::TYPE_COUNT];

	void NodeProperties::initList()
	{
		NodeProperties np;

		// AIR
		np.opaque = false;
		np.cube = true;
		np.name = "air";
		np.color.set(0, 0, 0, 0);
		s_list[node::AIR] = np;

		// STONE
		np.opaque = true;
		np.cube = true;
		np.name = "stone";
		np.color.set(127, 127, 127);
		s_list[node::STONE] = np;

		// DIRT
		np.opaque = true;
		np.cube = true;
		np.name = "dirt";
		np.color.set(128, 64, 0);
		s_list[node::DIRT] = np;

		// GRASS
		np.opaque = true;
		np.cube = true;
		np.name = "grass";
		np.color.set(128, 192, 0);
		s_list[node::GRASS] = np;
	}

	void NodeProperties::freeList()
	{
	}

	const NodeProperties & NodeProperties::get(u8 type)
	{
		return s_list[type];
	}

} // namespace zcraft

