
#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include <JANA/JApplication.h>
#include <JANA/Services/JParameterManager.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "CLI/CLI.hpp"

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "onnx/OnnxRuntimeService.hpp"

#include "clustering/AiVtpClusterFactory.hpp"
#include "clustering/VtpClusterFactory.hpp"

#include "io/CsvWriterProcessor.hpp"
#include "io/RandomSource.hpp"
#include "io/ReplaySource.hpp"

struct ProgramArguments {
	std::map<std::string, std::string> params;
	std::vector<std::string> filePaths;
};

static std::map<std::string, std::string> parseParameterOverrides(const std::vector<std::string> &extras) {
	std::map<std::string, std::string> paramMap;

	for (size_t i = 0; i < extras.size(); ++i) {
		std::string arg = extras[i];

		if (!(arg.size() >= 2 && (arg[0] == '-' || arg[0] == '/') && (arg[1] == 'p' || arg[1] == 'P'))) {
			continue;
		}

		arg = arg.substr(2);
		auto equalPos = arg.find('=');

		std::string key;
		std::string value;
		if (equalPos != std::string::npos) {
			key = arg.substr(0, equalPos);
			value = arg.substr(equalPos + 1);
		} else if ((i + 1) < extras.size() && !extras[i + 1].empty() && extras[i + 1][0] != '-') {
			key = arg;
			value = extras[++i];
		} else {
			key = arg;
			value = "";
		}

		std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		paramMap[key] = value;
	}

	return paramMap;
}

static inline ProgramArguments parseArguments(int argc, char **argv) {
	CLI::App app{"nps ONNX clustering analysis"};

	ProgramArguments args;
	bool showHelp = false;
	bool showVersion = false;

	app.add_flag("--version,-v", showVersion, "Show version information");

	// Define parameters starting with -p or -P (case-insensitive)
	app.allow_extras(); // Allow unrecognized options

	// Collect file paths (positional arguments)
	app.add_option("files", args.filePaths, "Input files");

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		exit(app.exit(e));
	}

	if (showVersion) {
		std::cout << "Version 1.0" << std::endl;
		exit(0);
	}

	// Process extra arguments to handle -p* options
	args.params = parseParameterOverrides(app.remaining());
	return args;
}

int main(int argc, char *argv[]) {

	auto parsedArgs = parseArguments(argc, argv);

	auto parameterManager = new JParameterManager();

	for (const auto &[name, value] : parsedArgs.params) {
		parameterManager->SetParameter(name, value);
	}

	JApplication app(parameterManager);

	app.ProvideService(std::make_shared<nps::geo::NpsGeometryService>());
	app.ProvideService(std::make_shared<nps::calib::fAdc250Service>());
	app.ProvideService(std::make_shared<nps::calib::VtpService>());
	app.ProvideService(std::make_shared<onnx::OnnxRuntimeService>());

	// add all available event sources, use event_source_type to select
	app.Add(new JEventSourceGeneratorT<nps::io::RandomSource>);
	app.Add(new JEventSourceGeneratorT<nps::io::ReplaySource>);

	if (parsedArgs.filePaths.empty()) {
		std::cerr << "Error: No input files provided for replay source." << std::endl;
		exit(1);
	}
	for (auto &filePath : parsedArgs.filePaths) {
		app.Add(filePath);
	}

	// create fpga reco cluster factory and add it to the app
	// auto vtpClusterGenerator = new JOmniFactoryGeneratorT<nps::clustering::VtpClusterFactory>();
	// vtpClusterGenerator->AddWiring(
	// 	"VtpClusterFactory",	 // tag
	// 	{"RawHits", "VtpSeeds"}, // inputs
	// 	{"VtpClusters"}			 // outputs
	// );
	// app.Add(vtpClusterGenerator);

	// create the ai reco cluster factory and add it to the app
	auto aiVtpClusterGenerator = new JOmniFactoryGeneratorT<nps::clustering::AiVtpClusterFactory>();
	aiVtpClusterGenerator->AddWiring(
		"AiVtpClusterFactory", // tag
		{"RawHits"},		   // inputs
		{"RecoClusters"}	   // outputs
	);
	app.Add(aiVtpClusterGenerator);

	app.Add(new nps::io::CsvWriterProcessor());
	app.Initialize();

	app.Run();

	return 0;
}
