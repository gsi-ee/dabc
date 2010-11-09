/*
 * Simple Readout application for Epics ezca readout
 * V-0.8 1-Nov-2010 J.Adamczewski-Musch. GSI
 */



#include "ezca/ReadoutApplication.h"


#include "dabc/Manager.h"






ezca::ReadoutApplication::ReadoutApplication() :
	dabc::Application(ezca::nameReadoutAppClass)
{

	CreateParStr(ezca::xmlEpicsName, "IOC-Transport-0");
	CreateParStr(ezca::xmlUpdateFlagRecord, "dummyflag");
	CreateParStr(ezca::xmlEventIDRecord,"dummyid");
	CreateParInt(ezca::xmlNumLongRecords, 0);
	for (int n = 0; n < NumLongRecs(); n++)
	{
		CreateParStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, n)));
	}
	CreateParInt(ezca::xmlNumDoubleRecords, 0);
	for (int n = 0; n < NumDoubleRecs(); n++)
		{
			CreateParStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, n)));
		}


	CreateParInt(dabc::xmlBufferSize, 65536);
	CreateParInt(dabc::xmlNumBuffers, 20);
//	CreateParInt(ezca::xmlEventsPerBuffer, 1);
//	CreateParInt(ezca::xmlHitDelay, 100);
	CreateParDouble(ezca::xmlTimeout, 0.1);

	CreateParStr(dabc::xmlPoolName, "EpicsPool");
	CreateParStr(ezca::xmlModuleName, "EpicsMoni");
	CreateParStr(ezca::xmlModuleThread, "EpicsThread");

	CreateParStr(mbs::xmlServerKind, mbs::ServerKindToStr(mbs::StreamServer));
	CreateParStr(mbs::xmlFileName, "");
	CreateParInt(mbs::xmlSizeLimit, 0);

	DOUT1(("!!!! Data server plugin created %s !!!!", GetName()));
}

int ezca::ReadoutApplication::DataServerKind() const
{
	return mbs::StrToServerKind(GetParStr(mbs::xmlServerKind).c_str());
}


bool ezca::ReadoutApplication::CreateAppModules()
{
	DOUT1(("CreateAppModules starts..."));
	bool res = false;
	dabc::Command* cmd;

	dabc::lgr()->SetLogLimit(10000000);

	dabc::mgr()->CreateMemoryPool(GetParStr(dabc::xmlPoolName,
			"dummy pool").c_str(), GetParInt(dabc::xmlBufferSize, 8192),
			GetParInt(dabc::xmlNumBuffers, 100));

	// Readout module with memory pool:
	cmd = new dabc::CmdCreateModule(ezca::nameReadoutModuleClass,
			GetParStr(ezca::xmlModuleName).c_str(), GetParStr(ezca::xmlModuleThread).c_str());

	cmd->SetStr(dabc::xmlPoolName, GetParStr(dabc::xmlPoolName, "dummy pool"));
	cmd->SetInt(dabc::xmlNumInputs, 1);
    cmd->SetBool(mbs::xmlFileOutput, true);
    cmd->SetBool(mbs::xmlServerOutput, true);

	res = dabc::mgr()->Execute(cmd);
	DOUT1(("Create Epics Readout module = %s", DBOOL(res)));
	if (res != dabc::cmd_true)
		return false;

	cmd = new dabc::CmdCreateTransport(FORMAT(("%s/Input0", GetParStr(ezca::xmlModuleName).c_str())), ezca::typeEpicsInput);
	cmd->SetStr(ezca::xmlEpicsName, GetParStr(ezca::xmlEpicsName).c_str());
	cmd->SetStr(ezca::xmlUpdateFlagRecord, GetParStr(ezca::xmlUpdateFlagRecord).c_str());
	cmd->SetStr(ezca::xmlEventIDRecord, GetParStr(ezca::xmlEventIDRecord).c_str());
	cmd->SetInt(ezca::xmlNumLongRecords,NumLongRecs());
	for (int n = 0; n < NumLongRecs(); n++)
		{

			cmd->SetStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, n)),GetParStr(FORMAT(("%s%d", ezca::xmlNameLongRecords, n))).c_str());
		}
	cmd->SetInt(ezca::xmlNumDoubleRecords,NumDoubleRecs());
	for (int n = 0; n < NumDoubleRecs(); n++)
		{
			cmd->SetStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, n)),GetParStr(FORMAT(("%s%d", ezca::xmlNameDoubleRecords, n))).c_str());
		}
	cmd->SetDouble(ezca::xmlTimeout,GetParDouble(ezca::xmlTimeout, 0.1));
	//cmd->SetInt(ezca::xmlFormatMbs,true);
	res = dabc::mgr()->Execute(cmd);
	if (res != dabc::cmd_true)
		return false;


	// connect file and mbs server outputs:
	if (OutputFileName().length() > 0)
	{
		cmd = new dabc::CmdCreateTransport(
					FORMAT(("%s/%s", GetParStr(xmlModuleName).c_str(), mbs::portFileOutput)),
					mbs::typeLmdOutput);
		cmd->SetStr(mbs::xmlFileName, OutputFileName().c_str());
		res = dabc::mgr()->Execute(cmd);
		DOUT1(
				("Create raw lmd output file %s , result = %s", OutputFileName().c_str(), DBOOL(
						res)));
		if (!res)
			return false;
	}

	if (DataServerKind() != mbs::NoServer)
	{
		///// connect module to mbs server:
		cmd = new dabc::CmdCreateTransport(
							FORMAT(("%s/%s", GetParStr(xmlModuleName).c_str(), mbs::portServerOutput)),
							mbs::typeServerTransport,
							"MbsServerThrd");

		// no need to set extra parameters - they will be taken from application !!!
		//      cmd->SetStr(mbs::xmlServerKind, mbs::ServerKindToStr(DataServerKind())); //mbs::StreamServer ,mbs::TransportServer
		//      cmd->SetInt(dabc::xmlBufferSize, GetParInt(dabc::xmlBufferSize, 8192));

		res = dabc::mgr()->Execute(cmd);
		DOUT1(
				("Connected readout module output to Mbs server = %s", DBOOL(
						res)));
		if (!res)
			return false;
	}

	return true;
}

int ezca::ReadoutApplication::ExecuteCommand(dabc::Command* cmd)
{
	int res = dabc::cmd_false;
	// insert optional command treatment here
	res = dabc::Application::ExecuteCommand(cmd);

	return res;
}

