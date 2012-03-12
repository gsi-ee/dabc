/*
 * Simple Readout application for Epics ezca readout
 * V-0.8 1-Nov-2010 J.Adamczewski-Musch. GSI
 */



#include "ezca/ReadoutApplication.h"


#include "dabc/Manager.h"






ezca::ReadoutApplication::ReadoutApplication() :
	dabc::Application(ezca::nameReadoutAppClass)
{

	CreatePar(ezca::xmlEpicsName).DfltStr("IOC-Transport-0");
	CreatePar(ezca::xmlUpdateFlagRecord).DfltStr("dummyflag");
	CreatePar(ezca::xmlEventIDRecord).DfltStr("dummyid");
	CreatePar(ezca::xmlEpicsSubeventId).DfltInt(5);
	CreatePar(ezca::xmlNumLongRecords).DfltInt(0);
	for (int n = 0; n < NumLongRecs(); n++)
		CreatePar(dabc::format("%s%d", ezca::xmlNameLongRecords, n));
	CreatePar(ezca::xmlNumDoubleRecords).DfltInt(0);
	for (int n = 0; n < NumDoubleRecs(); n++)
		{
			CreatePar(dabc::format("%s%d", ezca::xmlNameDoubleRecords, n));
		}


	CreatePar(dabc::xmlBufferSize).DfltInt(65536);
	CreatePar(dabc::xmlNumBuffers).DfltInt(20);
//	CreatePar(ezca::xmlEventsPerBuffer).DfltInt(1);
//	CreatePar(ezca::xmlHitDelay).DfltInt(100);
	CreatePar(ezca::xmlTimeout).DfltDouble(5.);

	CreatePar(dabc::xmlPoolName).DfltStr("EpicsPool");
	CreatePar(ezca::xmlModuleName).DfltStr("EpicsMoni");
	CreatePar(ezca::xmlModuleThread).DfltStr("EpicsThread");

	CreatePar(mbs::xmlServerKind).DfltStr(mbs::ServerKindToStr(mbs::StreamServer));
	CreatePar(mbs::xmlFileName).DfltStr("");
	CreatePar(mbs::xmlSizeLimit).DfltInt(0);

	CreatePar(ezca::xmlCommandReceiver).DfltStr("");

	DOUT1(("!!!! Data server plugin created %s value %d !!!!", GetName(), Par(ezca::xmlEpicsSubeventId).AsInt()));
}

int ezca::ReadoutApplication::DataServerKind() const
{
	return mbs::StrToServerKind(Par(mbs::xmlServerKind).AsStr());
}


bool ezca::ReadoutApplication::CreateAppModules()
{
	DOUT1(("CreateAppModules starts..."));
	bool res = false;

	dabc::Command cmd;

	dabc::lgr()->SetLogLimit(10000000);

	dabc::mgr.CreateMemoryPool(
	      Par(dabc::xmlPoolName).AsStr("dummy pool"),
	      Par(dabc::xmlBufferSize).AsInt(8192),
			Par(dabc::xmlNumBuffers).AsInt(100));

	// Readout module with memory pool:
	cmd = dabc::CmdCreateModule(ezca::nameReadoutModuleClass,
			Par(ezca::xmlModuleName).AsStr(), Par(ezca::xmlModuleThread).AsStr());

	cmd.SetStr(dabc::xmlPoolName, Par(dabc::xmlPoolName).AsStdStr("dummy pool"));
	cmd.SetInt(dabc::xmlNumInputs, 1);
   cmd.SetBool(mbs::xmlFileOutput, true);
   cmd.SetBool(mbs::xmlServerOutput, true);

	res = dabc::mgr.Execute(cmd);
	DOUT1(("Create Epics Readout module = %s", DBOOL(res)));
	if (!res) return false;

	cmd = dabc::CmdCreateTransport(FORMAT(("%s/Input0", Par(ezca::xmlModuleName).AsStr())), ezca::typeEpicsInput);
	cmd.SetStr(ezca::xmlEpicsName, Par(ezca::xmlEpicsName).AsStdStr());
	cmd.SetStr(ezca::xmlUpdateFlagRecord, Par(ezca::xmlUpdateFlagRecord).AsStdStr());
	cmd.SetStr(ezca::xmlCommandReceiver, Par(ezca::xmlCommandReceiver).AsStdStr());
	cmd.SetStr(ezca::xmlEventIDRecord, Par(ezca::xmlEventIDRecord).AsStdStr());
	cmd.SetInt(ezca::xmlEpicsSubeventId, Par(ezca::xmlEpicsSubeventId).AsInt(8));
	cmd.SetInt(ezca::xmlNumLongRecords,NumLongRecs());
	for (int n = 0; n < NumLongRecs(); n++)
   	cmd.SetStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, n)), Par(FORMAT(("%s%d", ezca::xmlNameLongRecords, n))).AsStdStr());
	cmd.SetInt(ezca::xmlNumDoubleRecords,NumDoubleRecs());

	for (int n = 0; n < NumDoubleRecs(); n++)
		{
			cmd.SetStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, n)), Par(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, n))).AsStdStr());
		}
	cmd.Field(ezca::xmlTimeout).SetDouble(Par(ezca::xmlTimeout).AsDouble(0.1));
	//cmd.SetInt(ezca::xmlFormatMbs,true);
	res = dabc::mgr.Execute(cmd);
	if (!res) return false;


	// connect file and mbs server outputs:
	if (OutputFileName().length() > 0)
	{
		cmd = dabc::CmdCreateTransport(
					FORMAT(("%s/%s", Par(xmlModuleName).AsStr(), mbs::portFileOutput)),
					mbs::typeLmdOutput);
		cmd.SetStr(mbs::xmlFileName, OutputFileName());
		res = dabc::mgr.Execute(cmd);
		DOUT1(("Create raw lmd output file %s , result = %s", OutputFileName().c_str(), DBOOL(res)));
		if (!res)
			return false;
	}

	if (DataServerKind() != mbs::NoServer)
	{
		///// connect module to mbs server:
		cmd = dabc::CmdCreateTransport(
							FORMAT(("%s/%s", Par(xmlModuleName).AsStr(), mbs::portServerOutput)),
							mbs::typeServerTransport,
							"MbsServerThrd");

		// no need to set extra parameters - they will be taken from application !!!
		//      cmd.Field(mbs::xmlServerKind).SetStr(mbs::ServerKindToStr(DataServerKind())); //mbs::StreamServer ,mbs::TransportServer
		//      cmd.Field(dabc::xmlBufferSize).SetInt(Par(dabc::xmlBufferSize).AsInt(8192));

		res = dabc::mgr.Execute(cmd);
		DOUT1(("Connected readout module output to Mbs server = %s", DBOOL(res)));
		if (!res)
			return false;
	}

	return true;
}

int ezca::ReadoutApplication::ExecuteCommand(dabc::Command cmd)
{
	int res = dabc::cmd_false;
	// insert optional command treatment here
	res = dabc::Application::ExecuteCommand(cmd);

	return res;
}

