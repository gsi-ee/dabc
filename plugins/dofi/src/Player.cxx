// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dofi/Player.h"

#include "dabc/Publisher.h"

#include <cstdio>

dofi::Player::Player(const std::string &name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd)
{
   //~ fWorkerHierarchy.Create("FESA", true);

   //~ dabc::CommandDefinition cmddef = CreateCmdDef("CmdGosip");
   //~ //cmddef.SetField(dabc::prop_auth, true); // require authentication
   //~ cmddef.AddArg("cmd", "string", true, "-");

   //~ dabc::Hierarchy ui = fWorkerHierarchy.CreateHChild("UI");
   //~ ui.SetField(dabc::prop_kind, "DABC.HTML");
   //~ ui.SetField("_UserFilePath", "${DABCSYS}/plugins/dofi/htm/");
   //~ ui.SetField("_UserFileMain", "main.htm");

   //~ CreateTimer("update", 1.);

   //~ PublishPars("GOSIP/Test");
}

dofi::Player::~Player()
{
}

void dofi::Player::ProcessTimerEvent(unsigned )
{
   //dabc::Hierarchy ui = fWorkerHierarchy.FindChild("UserInterface");
   //DOUT0("Process timer '%s'", ui.GetField("FilePath").AsStr().c_str());
}


int dofi::Player::ExecuteCommand(dabc::Command cmd)
{
   //if (cmd.IsName("CmdGosip")) {

      //int sfp = cmd.GetInt("sfp", 0);
      //int dev = cmd.GetInt("dev", 0);

      //bool log_output = cmd.GetInt("log") > 0;

      //std::string addr;
      //if ((sfp<0) || (dev<0))
         //addr = "-- -1 -1";
      //else
         //addr = dabc::format("%d %d", sfp, dev);

      //std::vector<std::string> doficmd = cmd.GetField("cmd").AsStrVect();

      //std::vector<std::string> dofires;
      //std::vector<std::string> dofilog;

      //DOUT2("*** CmdGosip len %u ****", (unsigned) doficmd.size());
      //for (unsigned n=0;n<doficmd.size();n++) {

         //std::string currcmd = doficmd[n];

         //bool isreading = (currcmd.find("-r") == 0);
         //bool iswriting = (currcmd.find("-w") == 0);

         //if (!isreading && !iswriting && (currcmd[0]!='-')) {
            //isreading = true;
            //currcmd = std::string("-r adr ") + currcmd;
         //}

         //size_t ppp = currcmd.find("adr");
         //if (ppp!=std::string::npos) currcmd.replace(ppp,3,addr);

         //currcmd = "doficmd " + currcmd;

         //if (log_output) dofilog.emplace_back(currcmd);

         //// DOUT0("CMD %s", currcmd.c_str());

         //FILE* pipe = popen(currcmd.c_str(), "r");

         //if (!pipe) {
            //dofires.emplace_back("<err>");
            //break;
         //}

         //char buf[50000];
         //memset(buf, 0, sizeof(buf));
         //int totalsize = 0;

         //while(!feof(pipe) && (totalsize<200000)) {
            //int size = (int)fread(buf,1, sizeof(buf)-1, pipe); //cout<<buffer<<" size="<<size<<endl;
            //if (size<=0) break;
            //while ((size>0) && ((buf[size-1]==' ') || (buf[size-1]=='\n'))) size--;
            //buf[size] = 0;
            //totalsize+=size;
            //if (log_output && (size>0)) dofilog.emplace_back(buf);
            //// DOUT0("Get %s", buf);
         //}

         //pclose(pipe);

         //if (iswriting) {

            //// DOUT0("Writing res:%s len %d", buf, strlen(buf));

            //if (strlen(buf) > 2) {
               //dofires.emplace_back("<err>");
               //break;
            //}

            //dofires.emplace_back("<ok>");
            //continue;
         //}

         //if (isreading) {
            //long value = 0;
            //if (!dabc::str_to_lint(buf,&value)) {
               //dofires.emplace_back("<err>");
               //break;
            //}
            //// DOUT0("Reading ok %ld", value);
            //dofires.emplace_back(dabc::format("%ld", value));
            //continue;
         //}

         //// no idea that should be returned by other commands
         //dofires.emplace_back("<undef>");
      //}

      //while (dofires.size() < doficmd.size()) dofires.emplace_back("<skip>");

      //cmd.SetField("res", dofires);
      //if (log_output)
         //cmd.SetField("log", dofilog);

      //DOUT2("*** CmdGosip finished ****");


      //return dabc::cmd_true;
   //}

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}
