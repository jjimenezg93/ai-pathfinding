#include <stdafx.h>

#include "pathfinder.h"

const char * gridFilename = "grid.txt";
const uint16_t cellSize = 25; //size of each cell in pixels

Pathfinder::Pathfinder(): MOAIEntity2D(), m_grid{gridFilename} {
	RTTI_BEGIN
		RTTI_EXTEND(MOAIEntity2D)
	RTTI_END

	//fill m_nodes
	uint16_t gridRows = m_grid.GetGridWidth();
	int16_t cost = 0;
	for (uint16_t y = 0; y < gridRows; ++y) {
		for (uint16_t x = 0; x < gridRows; ++x) {
			if (m_grid.IsObstacle(x, y)) {
				cost = -1;
			} else {
				cost = rand() % 3000 + 500;
			}
			USVec2D v(x, y);
			PathNode node(v, cost, 0, nullptr);
			m_nodes.push_back(node);
		}
	}
}

Pathfinder::~Pathfinder() {

}

void Pathfinder::UpdatePath() {
	if (m_StartPosition.mX >= 0 && m_StartPosition.mY >= 0 && m_EndPosition.mX >= 0
	&& m_EndPosition.mY >= 0) { //need to check both Start and End positions are at least 0
		//need to set all parents to null before recalculating the path.
		//if not, there's an infinite loop in BuildPath()
		for (std::vector<PathNode>::iterator itr = m_nodes.begin(); itr != m_nodes.end(); ++itr) {
			itr->SetParent(nullptr);
		}

		//set StartNode and EndNode
		uint8_t pos = m_StartPosition.mX + m_StartPosition.mY * m_grid.GetGridWidth();
		m_startNode = &m_nodes.at(pos);
		pos = m_EndPosition.mX + m_EndPosition.mY  * m_grid.GetGridWidth();
		m_endNode = &m_nodes.at(pos);

		//recalculate all estimated costs
		uint16_t gridRows = m_grid.GetGridWidth();
		int16_t cost = 0;
		PathNode * node;
		for (uint16_t y = 0; y < gridRows; ++y) {
			for (uint16_t x = 0; x < gridRows; ++x) {
				node = &(m_nodes.at(x + y * m_grid.GetGridWidth()));
				if (node->GetCost() != -1) {
					//estimated based on pixels distance
					float cost = node->GetCost();
					float estimated = (sqrt(pow((m_endNode->GetPos().mX - x)* cellSize, 2)
						+ pow((m_endNode->GetPos().mY - y)* cellSize, 2)));
					node->SetEstimatedCost(estimated);
					node->SetCost(cost + estimated);
					node->SetTotalCost(cost + estimated);
				}
			}
		}

		//A*
		m_openNodes.clear();
		m_closedNodes.clear();
		m_startNode->SetCost(0);
		m_startNode->SetTotalCost(0);
		m_openNodes.push_back(m_startNode);
		bool objectiveFound = false;
		while (!m_openNodes.empty() && !objectiveFound) {
			PathNode * node = *(m_openNodes.begin());
			m_openNodes.erase(m_openNodes.begin());
			if (node->GetPos().mX == m_endNode->GetPos().mX
				&& node->GetPos().mY == m_endNode->GetPos().mY) {
				BuildPath(node);
			} else {
				//with 2 for loops from -1 to 1, we iterate over all adjacent nodes
				for (int8_t y = -1; y <= 1; ++y) {
					for (int8_t x = -1; x <= 1; ++x) {
						if (node->GetPos().mX + x >= 0
						&& node->GetPos().mX + x < m_grid.GetGridWidth()
						&& node->GetPos().mY + y >= 0
						&& node->GetPos().mY + y < m_grid.GetGridWidth()) {
							int16_t pos = (node->GetPos().mX + x) +
								(node->GetPos().mY + y) * m_grid.GetGridWidth();
							if (pos >= 0 && pos <= m_nodes.size()) {
								PathNode * nextNode = &(m_nodes.at(pos));
								
								//if nextNode == objective -> buildPath and terminate A*
								if (nextNode->GetPos().mX == m_endNode->GetPos().mX
								&& nextNode->GetPos().mY == m_endNode->GetPos().mY) {
									nextNode->SetParent(node);
									nextNode->SetTotalCost(node->GetTotalCost() + nextNode->GetCost());
									BuildPath(nextNode);
									objectiveFound = true;
									break;
								}
								if (nextNode->GetCost() != -1) {
									//if we check the same node, it will overwrite its parent
									if (node == nextNode) {
										continue;
									} else if (find(m_closedNodes.begin(), m_closedNodes.end(), nextNode)
									!= m_closedNodes.end()) {
										continue;
									} else if (find(m_openNodes.begin(), m_openNodes.end(), nextNode)
									!= m_openNodes.end()) {
										if (nextNode->GetTotalCost() > 0
										&& nextNode->GetTotalCost() >
										node->GetTotalCost() + nextNode->GetCost()) {
											nextNode->SetTotalCost(node->GetTotalCost() + nextNode->GetCost());
											nextNode->SetParent(node);
										}
									} else {
										nextNode->SetParent(node);
										m_openNodes.push_back(nextNode);
									}
								}
							}
						}
					}
				}
				std::vector<PathNode *>::iterator el = find(m_openNodes.begin(),
					m_openNodes.end(), node);
				if (el != m_openNodes.end()) {
					m_openNodes.erase(el);
				}
				m_closedNodes.push_back(node);
			}
		}
	}
}

void Pathfinder::BuildPath(PathNode * lastNode) {
	m_finalPath.clear();
	//iterate over all node parents to get the full path
	PathNode * node = lastNode;
	while (node->GetParent()) {
		m_finalPath.push_back(node);
		node = node->GetParent();
	}
	node = node;
}

void Pathfinder::DrawDebug() {
	MOAIGfxDevice& gfxDevice = MOAIGfxDevice::Get();

	gfxDevice.SetPenColor(1.0f, 1.0f, 1.0f, 1.0f);
	uint16_t gridRows = m_grid.GetGridWidth(); //also cols -> squared
	USRect r;
	r.mXMin = 0.f;
	r.mYMin = 0.f;
	r.mXMax = static_cast<float>(cellSize * gridRows);
	r.mYMax = static_cast<float>(cellSize * gridRows);

	MOAIDraw::DrawGrid(r, gridRows, gridRows);
	USRect fillCell;

	gfxDevice.SetPenColor(1.0f, 1.0f, 1.0f, 1.0f);
	for (uint16_t i = 0; i < m_finalPath.size(); ++i) {
		fillCell.mXMin = static_cast<float>(m_finalPath.at(i)->GetPos().mX * cellSize);
		fillCell.mYMin = static_cast<float>(m_finalPath.at(i)->GetPos().mY * cellSize);
		fillCell.mXMax = static_cast<float>(m_finalPath.at(i)->GetPos().mX * cellSize + cellSize);
		fillCell.mYMax = static_cast<float>(m_finalPath.at(i)->GetPos().mY * cellSize + cellSize);
		MOAIDraw::DrawRectFill(fillCell);
	}

	for (uint16_t i = 0; i < gridRows; ++i) {
		for (uint16_t j = 0; j < gridRows; ++j) {
			if (m_grid.IsObstacle(i, j)) {
				gfxDevice.SetPenColor(1.0f, 0.0f, 0.0f, 1.0f);
				fillCell.mXMin = static_cast<float>(i * cellSize);
				fillCell.mYMin = static_cast<float>(j * cellSize);
				fillCell.mXMax = static_cast<float>(i * cellSize + cellSize);
				fillCell.mYMax = static_cast<float>(j * cellSize + cellSize);
				MOAIDraw::DrawRectFill(fillCell);
			}
		}
	}
	//startPos
	fillCell.mXMin = static_cast<float>(m_StartPosition.mX * cellSize);
	fillCell.mYMin = static_cast<float>(m_StartPosition.mY * cellSize);
	fillCell.mXMax = static_cast<float>(m_StartPosition.mX * cellSize + cellSize);
	fillCell.mYMax = static_cast<float>(m_StartPosition.mY * cellSize + cellSize);
	gfxDevice.SetPenColor(0.0f, 1.0f, 0.0f, 1.0f);
	MOAIDraw::DrawRectFill(fillCell);

	//endPos
	fillCell.mXMin = static_cast<float>(m_EndPosition.mX * cellSize);
	fillCell.mYMin = static_cast<float>(m_EndPosition.mY * cellSize);
	fillCell.mXMax = static_cast<float>(m_EndPosition.mX * cellSize + cellSize);
	fillCell.mYMax = static_cast<float>(m_EndPosition.mY * cellSize + cellSize);
	gfxDevice.SetPenColor(0.0f, 0.0f, 1.0f, 1.0f);
	MOAIDraw::DrawRectFill(fillCell);
}

void Pathfinder::InitStartPosition(float x, float y) {
	m_StartPosition = USVec2D(floor(x), floor(y));
	UpdatePath();
}

void Pathfinder::InitEndPosition(float x, float y) {
	m_EndPosition = USVec2D(floor(x), floor(y));
	UpdatePath();
}

void Pathfinder::SetStartPosition(float x, float y) {
	x /= cellSize;
	y /= cellSize;
	m_StartPosition = USVec2D(floor(x), floor(y));
	UpdatePath();
}
void Pathfinder::SetEndPosition(float x, float y) {
	x /= cellSize;
	y /= cellSize;
	m_EndPosition = USVec2D(floor(x), floor(y));
	UpdatePath();
}

bool Pathfinder::PathfindStep() {
	// returns true if pathfinding process finished
	return true;
}

//lua configuration ----------------------------------------------------------------//
void Pathfinder::RegisterLuaFuncs(MOAILuaState& state) {
	MOAIEntity::RegisterLuaFuncs(state);

	luaL_Reg regTable[] = {
		{"initStartPosition", _initStartPosition},
		{"initEndPosition", _initEndPosition},
		{"setStartPosition", _setStartPosition},
		{"setEndPosition", _setEndPosition},
		{"pathfindStep", _pathfindStep},
		{NULL, NULL}
	};

	luaL_register(state, 0, regTable);
}

int Pathfinder::_initStartPosition(lua_State* L) {
	MOAI_LUA_SETUP(Pathfinder, "U")

	float pX = state.GetValue<float>(2, 0.0f);
	float pY = state.GetValue<float>(3, 0.0f);
	self->InitStartPosition(pX, pY);
	return 0;
}

int Pathfinder::_initEndPosition(lua_State* L) {
	MOAI_LUA_SETUP(Pathfinder, "U")

	float pX = state.GetValue<float>(2, 0.0f);
	float pY = state.GetValue<float>(3, 0.0f);
	self->InitEndPosition(pX, pY);
	return 0;
}

int Pathfinder::_setStartPosition(lua_State* L) {
	MOAI_LUA_SETUP(Pathfinder, "U")

	float pX = state.GetValue<float>(2, 0.0f);
	float pY = state.GetValue<float>(3, 0.0f);
	self->SetStartPosition(pX, pY);
	return 0;
}

int Pathfinder::_setEndPosition(lua_State* L) {
	MOAI_LUA_SETUP(Pathfinder, "U")

	float pX = state.GetValue<float>(2, 0.0f);
	float pY = state.GetValue<float>(3, 0.0f);
	self->SetEndPosition(pX, pY);
	return 0;
}

int Pathfinder::_pathfindStep(lua_State* L) {
	MOAI_LUA_SETUP(Pathfinder, "U")

	self->PathfindStep();
	return 0;
}