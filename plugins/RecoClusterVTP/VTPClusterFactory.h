

#ifndef _VTPClusterFactory_h_
#define _VTPClusterFactory_h_

#include <JANA/JObject.h>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct VTPClusterFactory : public JObject {
	JOBJECT_PUBLIC(VTPClusterFactory)

	std::vector<int> block_ids;		 // Block IDs (0-1079) that contributed to this cluster
	std::vector<int> x_coords;		 // x coordinates of the blocks in the calorimeter (col id)
	std::vector<int> y_coords;		 // y coordinates of the blocks in the calorimeter (row id)
	std::vector<double> pulse_times; // rise time of the pulse in each block (leading edge)
	std::vector<double> energies;	 // energy deposited in each block

	/// Make it convenient to construct one of these things
	VTPClusterFactory() {}

	VTPClusterFactory(
		const std::vector<int> &block_ids, const std::vector<int> &x_coords, const std::vector<int> &y_coords,
		const std::vector<double> &pulse_times, const std::vector<double> &energies
	) :
		block_ids(block_ids),
		x_coords(x_coords),
		y_coords(y_coords),
		pulse_times(pulse_times),
		energies(energies) {}

	void AddHit(int block_id, int x_coord, int y_coord, double pulse_time, double energy) {
		block_ids.push_back(block_id);
		x_coords.push_back(x_coord);
		y_coords.push_back(y_coord);
		pulse_times.push_back(pulse_time);
		energies.push_back(energy);
	}

	void GetSeedInfo(int &seed_block_id, double &seed_time, double &seed_energy) const {
		if (block_ids.empty()) {
			seed_block_id = -1; // or some invalid value
			seed_time = 0.0;
			seed_energy = 0.0;
			return;
		}
		seed_block_id = block_ids[0];
		seed_time = pulse_times[0];
		seed_energy = energies[0];
	}

	/// Override Summarize to tell JANA how to produce a convenient string representation for our JObject.
	/// This can be called from user code, but also lets JANA automatically inspect its own data. See the
	/// CsvWriter example. Warning: Because this is slow, it should be used for debugging and monitoring
	/// but not inside the performance critical code paths.

	void Summarize(JObjectSummary &summary) const override {
		summary.add(block_ids.size(), "n_blocks", "%zu", "Number of blocks in the cluster");
		summary.add(pulse_times.size(), "n_pulse_times", "%zu", "Number of pulse times in the cluster");
		summary.add(energies.size(), "n_energies", "%zu", "Number of energies in the cluster");
	}
};

#endif // _VTPClusterFactory_h_
