

#ifndef _VTPClusterSeed_h_
#define _VTPClusterSeed_h_

#include <JANA/JObject.h>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct VTPClusterSeed : public JObject {
	JOBJECT_PUBLIC(VTPClusterSeed)

	unsigned int col; // column id of the calorimeter
	unsigned int row; // row id of the calorimeter
	unsigned int
		size; // size of the cluster (number of blocks in 3x3 grid around the seed that have energy above threshold)
	double E; // total energy of the vtp registered energy in 3x3 grid around the seed, in GeV
	double t; // rise time of the seed pulse in ns

	/// Make it convenient to construct one of these things
	VTPClusterSeed(unsigned int col, unsigned int row, unsigned int size, double E, double t) :
		col(col),
		row(row),
		size(size),
		E(E),
		t(t) {}

	void Summarize(JObjectSummary &summary) const override {
		summary.add(col, NAME_OF(col), "%d", "Column id of the calorimeter");
		summary.add(row, NAME_OF(row), "%d", "Row id of the calorimeter");
		summary.add(
			size, NAME_OF(size), "%d",
			"Size of the cluster (number of blocks in 3x3 grid around the seed that have energy above threshold)"
		);
		summary.add(
			E, NAME_OF(E), "%f", "Total energy of the VTP registered energy in 3x3 grid around the seed, in GeV"
		);
		summary.add(t, NAME_OF(t), "%f", "Rise time of the seed pulse in ns");
	}
};

#endif // _VTPClusterSeed_h_
