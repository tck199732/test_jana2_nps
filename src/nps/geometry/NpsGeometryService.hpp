#pragma once

#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nps::geo {

struct block {
	int channel;
	int row;
	int col;
	int crate;
	int slot;
};

struct PairHash {
	size_t operator()(const std::pair<int, int> &p) const noexcept {
		return (static_cast<size_t>(p.first) << 32) ^ static_cast<int>(p.second);
	}
};

class NpsGeometryService : public JService {

public:
	explicit NpsGeometryService() : JService() {}
	~NpsGeometryService() override = default;

	void Init() override;

	int getBlockFromColRow(int col, int row) const;
	std::pair<int, int> getColRowFromBlock(int block) const;
	int getCrateFromBlock(int block) const;
	int getSlotFromBlock(int block) const;

	bool isNeighbour(int ch1, int ch2) const;
	bool isInsideGrid(int seedChannel, int channel, int gridSize) const;

private:
	std::vector<block> m_blocks;				// index-based lookup
	std::unordered_map<int, block> m_index_map; // block -> info
	std::unordered_map<std::pair<int, int>, int, PairHash> m_rc_to_index;
	void Load(const std::string &config_file);

	Parameter<std::string> m_geo_file{
		this, "geo:config_file", "geo_config.json", "Path to NPS geometry configuration file"
	};
};
} // namespace nps::geo
