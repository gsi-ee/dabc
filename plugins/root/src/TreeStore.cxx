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

#include "root/TreeStore.h"

#include "TTree.h"
#include "TFile.h"

root::TreeStore::TreeStore(const std::string& name) :
   dabc::LocalWorker(name),
   fFile(0),
   fTree(0)
{
}

root::TreeStore::~TreeStore()
{
   CloseTree();
}

void root::TreeStore::CloseTree()
{
   if (fTree && fFile) {
      fFile->cd();
      fTree->Write();
      delete fTree;
      fTree = 0;
      delete fFile;
      fFile = 0;
   }
}

int root::TreeStore::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("Fill")) {
      if (fTree) fTree->Fill();
      return dabc::cmd_true;
   } else
   if (cmd.IsName("Close")) {
      CloseTree();
      return dabc::cmd_true;
   } else
   if (cmd.IsName("CreateBranch")) {
      if (fTree==0) return dabc::cmd_false;

      if (cmd.GetPtr("member") != 0)
         fTree->Branch(cmd.GetStr("name").c_str(), cmd.GetPtr("member"), cmd.GetStr("kind").c_str());
      else
         fTree->Branch(cmd.GetStr("name").c_str(), cmd.GetStr("class_name").c_str(), (void**) cmd.GetPtr("obj"));
      return dabc::cmd_true;
   } else
   if (cmd.IsName("Create")) {
      if (fTree || fFile) CloseTree();
      fFile = TFile::Open(cmd.GetStr("fname","f.root").c_str(), "RECREATE", cmd.GetStr("ftitle","ROOT file store").c_str());
      if (fFile==0) return dabc::cmd_false;
      fTree = new TTree(cmd.GetStr("tname","T").c_str(), cmd.GetStr("ttitle","ROOT Tree").c_str());
      return dabc::cmd_true;
   }

   return dabc::cmd_false;
}
