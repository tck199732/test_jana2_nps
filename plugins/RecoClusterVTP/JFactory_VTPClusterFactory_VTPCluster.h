
#ifndef _JFactory_VTPClusterFactory_VTPCluster_h_
#define _JFactory_VTPClusterFactory_VTPCluster_h_

#include "Hit.h"
#include "NPSGeometryService.h"
#include "VTPClusterFactory.h"
#include "VTPClusterSeed.h"

#include <JANA/JEvent.h>
#include <JANA/JFactoryT.h>
#include <JANA/JService.h>

#include <NPS.hh>
#include <VTP.hh>
#include <fADC250.hh>

#include <cassert>

class JFactory_VTPClusterFactory_VTPCluster : public JFactoryT<VTPClusterFactory> {

	std::string m_vmeConfigFile = "config_fadc250.txt";
	std::string m_vtpConfigFile = "config_vtp.txt";

	std::shared_ptr<NPS::Geometry> m_geometry;
	std::unique_ptr<fADC250> m_fadcDevice;
	std::unique_ptr<VTP> m_vtpDevice;

public:
	JFactory_VTPClusterFactory_VTPCluster();
	void Init() override;
	void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
	void Process(const std::shared_ptr<const JEvent> &event) override;
};

#endif // _JFactory_VTPClusterFactory_VTPCluster_h_
