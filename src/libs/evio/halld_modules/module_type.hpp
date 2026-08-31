#pragma once
namespace halld::evio {
enum module_type {
	TID = 0,			   // =0
	FADC250 = 1,		   // =1
	FADC125 = 2,		   // =2
	F1TDC32 = 3,		   // =3
	F1TDC48 = 4,		   // =4
	JLAB_TS = 5,		   // =5
	TD = 6,				   // =6
	SSP = 7,			   // =7
	JLAB_DISC = 8,		   // =8
	MODULE_TYPE_RES1 = 9,  // =9
	MODULE_TYPE_RES2 = 10, // =10
	MODULE_TYPE_RES3 = 11, // =11
	MODULE_TYPE_RES4 = 12, // =12
	MODULE_TYPE_RES5 = 13, // =13
	MODULE_TYPE_RES6 = 14, // =14
	MODULE_TYPE_RES7 = 15, // =15

	// The following are not defined by the DAQ group (i.e.
	// they don't control the data format so can't encode
	// the module type in it)
	UNKNOWN = 16,  // =16
	VMECPU = 17,   // =17
	CAEN1190 = 18, // =18
	CAEN1290 = 19, // =19

	CDAQTSG = 22, // = 22

	N_MODULE_TYPES // Make sure this is the last thing in the enum!
};
} // namespace halld::evio