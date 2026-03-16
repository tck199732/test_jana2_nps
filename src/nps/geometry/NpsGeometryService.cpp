#include "NpsGeometryService.hpp"

namespace nps::geo {
void NpsGeometryService::Init() { Load(m_geo_file()); }

void NpsGeometryService::Load(const std::string &config_file) {
	std::ifstream fin(config_file);
	if (!fin.is_open()) {
		throw std::runtime_error("Cannot open NpsGeometryService config: " + config_file);
	}

	std::string line;
	while (std::getline(fin, line)) {
		if (line.empty() || line[0] == '#') {
			continue;
		}

		block info;
		std::istringstream iss(line);
		iss >> info.channel >> info.row >> info.col >> info.crate >> info.slot;
		if (!iss) {
			continue;
		}
		m_blocks.push_back(info);
		m_index_map[info.channel] = info;
		m_rc_to_index[{info.row, info.col}] = info.channel;
	}
}

int NpsGeometryService::getBlockFromColRow(int col, int row) const {
	auto it = m_rc_to_index.find({row, col});
	if (it == m_rc_to_index.end())
		throw std::out_of_range("Invalid (row, col)");
	return it->second;
}

std::pair<int, int> NpsGeometryService::getColRowFromBlock(int block) const {
	auto it = m_index_map.find(block);
	if (it == m_index_map.end())
		throw std::out_of_range("Invalid block index");
	return {it->second.col, it->second.row};
}

int NpsGeometryService::getCrateFromBlock(int block) const {
	auto it = m_index_map.find(block);
	if (it == m_index_map.end())
		throw std::out_of_range("Invalid block index");
	return it->second.crate;
}

int NpsGeometryService::getSlotFromBlock(int block) const {
	auto it = m_index_map.find(block);
	if (it == m_index_map.end())
		throw std::out_of_range("Invalid block index");
	return it->second.slot;
}

bool NpsGeometryService::isNeighbour(int ch1, int ch2) const {
	auto [col1, row1] = getColRowFromBlock(ch1);
	auto [col2, row2] = getColRowFromBlock(ch2);
	return (std::abs(col1 - col2) <= 1) && (std::abs(row1 - row2) <= 1) && !(col1 == col2 && row1 == row2);
}

bool NpsGeometryService::isInsideGrid(int seedChannel, int channel, int gridSize) const {
	auto [seedCol, seedRow] = getColRowFromBlock(seedChannel);
	auto [chCol, chRow] = getColRowFromBlock(channel);
	int halfGrid = gridSize / 2;
	return (std::abs(seedCol - chCol) <= halfGrid) && (std::abs(seedRow - chRow) <= halfGrid);
}

} // namespace nps::geo
