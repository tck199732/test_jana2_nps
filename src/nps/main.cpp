#include <JANA/Components/JOmniFactoryGeneratorT.h>
#include <JANA/JApplication.h>
#include <JANA/Services/JParameterManager.h>

#include <utility>

#include "CLI/CLI.hpp"

#include "calibration/VtpService.hpp"
#include "calibration/fAdc250Service.hpp"
#include "clustering/VtpClusterFactory.hpp"
#include "geometry/NpsGeometryService.hpp"
#include "io/CsvWriterProcessor.hpp"
#include "io/ReplaySource.hpp"

struct ProgramArguments {
	std::map<std::string, std::string> params;
	std::vector<std::string> filePaths;
	std::string outputPrefix = "nps_output";
};

static inline ProgramArguments parseArguments(int argc, char **argv) {
	CLI::App app{"nps ONNX clustering analysis"};

	bool showHelp = false;
	bool showVersion = false;
	std::string outputPrefix = "";

	app.add_flag("--version,-v", showVersion, "Show version information");
	app.add_flag("--output,-o", outputPrefix, "Output files prefix (no extensions)");

	// Define parameters starting with -p or -P (case-insensitive)
	std::vector<std::string> params;
	app.allow_extras(); // Allow unrecognized options

	// Collect file paths (positional arguments)
	std::vector<std::string> filePaths;
	app.add_option("files", filePaths, "Input files");

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
	std::map<std::string, std::string> paramMap;
	auto extras = app.remaining();
	for (size_t i = 0; i < extras.size(); ++i) {
		std::string arg = extras[i];

		// Check if argument starts with -p or -P
		if (arg.size() >= 2 && (arg[0] == '-' || arg[0] == '/') && (arg[1] == 'p' || arg[1] == 'P')) {

			// Remove the prefix
			arg = arg.substr(2);

			// Handle -pParam=value
			auto equalPos = arg.find('=');

			std::string key, value;
			if (equalPos != std::string::npos) {
				key = arg.substr(0, equalPos);
				value = arg.substr(equalPos + 1);
			} else if ((i + 1) < extras.size() && extras[i + 1][0] != '-') {
				// Handle -pParam value
				key = arg;
				value = extras[++i];
			} else {
				key = arg;
				value = "";
			}

			// Case-insensitive keys
			std::ranges::transform(key, key.begin(), ::tolower);
			paramMap[key] = value;
		} else if (arg[0] != '-') {
			// Assume positional argument (file path)
			filePaths.push_back(arg);
		}
	}

	return ProgramArguments{paramMap, filePaths};
}

int main(int argc, char *argv[]) {

	auto parsedArgs = parseArguments(argc, argv);

	auto parameterManager = new JParameterManager();
	parameterManager->SetDefaultParameter(
		"nps:output", parsedArgs.outputPrefix, "Output prefix for created files (no extension, alias to -o,--output)"
	);

	for (const auto &[name, value] : parsedArgs.params) {
		parameterManager->SetParameter(name, value);
	}

	JApplication app(parameterManager);

	app.ProvideService(std::make_shared<nps::geo::NpsGeometryService>());
	app.ProvideService(std::make_shared<nps::calib::fAdc250Service>());
	app.ProvideService(std::make_shared<nps::calib::VtpService>());

	auto vtpClusterGenerator = new JOmniFactoryGeneratorT<nps::clustering::VtpClusterFactory>();
	vtpClusterGenerator->AddWiring(
		"VtpClusterFactory",	 // tag
		{"RawHits", "VtpSeeds"}, // inputs
		{"VtpClusters"}			 // outputs
	);
	app.Add(vtpClusterGenerator);

	app.Add(new JEventSourceGeneratorT<nps::io::ReplaySource>);
	app.Add(new nps::io::CsvWriterProcessor());

	if (parsedArgs.filePaths.empty()) {
		std::cerr << "No input files specified" << std::endl;
		return 1;
	}

	for (auto &filePath : parsedArgs.filePaths) {
		app.Add(filePath);
	}

	app.Initialize();

	app.Run();

	return 0;
}
