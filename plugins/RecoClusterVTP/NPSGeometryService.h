#ifndef _NPSGeometryService_h_
#define _NPSGeometryService_h_

#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>
#include <NPS.hh>
#include <memory>
#include <string>

class NPSGeometryService : public JService {
	std::shared_ptr<NPS::Geometry> geometry;

public:
	NPSGeometryService() = default;

	void Init() override {

		std::string m_geo_config_file = "geo_config.json";

		auto app = GetApplication();
		app->SetDefaultParameter("nps:geo_config_file", m_geo_config_file, "Path to NPS geometry configuration file");

		geometry = std::make_shared<NPS::Geometry>(m_geo_config_file);
	}

	std::shared_ptr<NPS::Geometry> getGeometry() const { return geometry; }
};

#endif