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

#include "dabc/Url.h"

#include "TTree.h"
#include "TFile.h"

root::TreeStore::TreeStore(const std::string &name) :
   dabc::LocalWorker(name),
   fTree(nullptr)
{
}

root::TreeStore::~TreeStore()
{
   CloseTree();
}

void root::TreeStore::CloseTree()
{
   if (fTree) {
      TFile* f = fTree->GetCurrentFile();
      f->cd();
      fTree->Write();
      delete fTree;
      fTree = nullptr;
      delete f;
   }
}

int root::TreeStore::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("Fill")) {
      if (fTree) {
         fTree->Fill();
         cmd.SetStr("StoreInfo", dabc::format("File:%s Tree:%s Size:%s", fTree->GetCurrentFile()->GetName(), fTree->GetName(), dabc::size_to_str(fTree->GetTotBytes()).c_str()));
      }
      return dabc::cmd_true;
   } else
   if (cmd.IsName("Close")) {
      CloseTree();
      return dabc::cmd_true;
   } else
   if (cmd.IsName("CreateBranch")) {
      if (fTree==0) return dabc::cmd_false;

      if (cmd.GetPtr("member"))
         fTree->Branch(cmd.GetStr("name").c_str(), cmd.GetPtr("member"), cmd.GetStr("kind").c_str());
      else
         fTree->Branch(cmd.GetStr("name").c_str(), cmd.GetStr("class_name").c_str(), (void**) cmd.GetPtr("obj"));
      return dabc::cmd_true;
   } else
   if (cmd.IsName("Create")) {
      if (fTree) CloseTree();

      dabc::Url url(cmd.GetStr("fname","f.root"));

      int64_t maxsize = url.GetOptionInt("maxsize", cmd.GetInt("maxsize",0));

      TFile *f = TFile::Open(url.GetFullName().c_str(), "RECREATE", cmd.GetStr("ftitle","ROOT file store").c_str());
      if (!f) return dabc::cmd_false;
      if (maxsize > 0) TTree::SetMaxTreeSize(maxsize*1024*1024);
      fTree = new TTree(cmd.GetStr("tname","T").c_str(), cmd.GetStr("ttitle","ROOT Tree").c_str());
      cmd.SetPtr("tree_ptr", fTree);
      return dabc::cmd_true;
   }

   return dabc::cmd_false;
}
