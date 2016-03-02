/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2004 Alexander Neundorf (neundorf@kde.org)
  Copyright 2012-2015 Poul Bondo (poul.bondo@gmail.com)

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.


  http://www.cmake.org/Bug/print_bug_page.php?bug_id=9901
   * Currently compile file only works for files in local directory.
   * Currently we can only partially merge workspace and project files.
============================================================================*/
#include "cmExtraCodeLiteGenerator2.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmake.h"
#include "cmSourceFile.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmState.h"


#include <cmsys/SystemTools.hxx>
#include <cmsys/Directory.hxx>

#include <fstream>
using namespace std;

std::ofstream logf2("dummy.log");


//----------------------------------------------------------------------------
cmExtraCodeLiteGenerator2::cmExtraCodeLiteGenerator2()
:cmExternalMakefileProjectGenerator()
{
   this->SupportedGlobalGenerators.push_back("Unix Makefiles");
}


void cmExtraCodeLiteGenerator2::GetDocumentation(cmDocumentationEntry& entry, const std::string &) const
{
  entry.Name = this->GetName();
  entry.Brief = "Generates CodeLite project files.";
//  entry.Full = "Project files for CodeLite will be created in the top directory ...";
}


void cmExtraCodeLiteGenerator2::Generate2()
{
  for (auto &proj : this->GlobalGenerator->GetProjectMap())
  {
    assert(!proj.second.empty());
    //cmLocalGenerator    
    //auto &lg = *proj.second.front();
    
  }
}



std::string cmExtraCodeLiteGenerator2::CreateWorkspaceHeader( const std::string& workspaceProjectName )
{
   std::ostringstream result;
   result <<    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
             "<CodeLite_Workspace Name=\"" << workspaceProjectName << "\" Database=\"./" << workspaceProjectName << ".tags\">\n";

   return result.str();
   
}


std::string cmExtraCodeLiteGenerator2::CreateWorkspaceFooter( const std::string& configCMake )
{
  std::ostringstream result;

  result << "  <BuildMatrix>\n"
            "    <WorkspaceConfiguration Name=\"CMake\" Selected=\"yes\">\n"
            << configCMake <<
            "    </WorkspaceConfiguration>\n"
            "  </BuildMatrix>\n"
            "</CodeLite_Workspace>\n";
  return result.str();
}


void cmExtraCodeLiteGenerator2::Generate()
{
  std::string workspaceProjectName;
  std::string workspaceOutputDir;
  std::string workspaceFileName;
  std::string workspaceSourcePath;
  std::string configCMake;
  cmGeneratedFileStream fout; 

  // loop projects and locate the root project.
  // and extract the information for creating the workspace. Do we know if this is always the first ?
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
    it = this->GlobalGenerator->GetProjectMap().begin();
    it!= this->GlobalGenerator->GetProjectMap().end();
    ++it)
  {
    const cmMakefile* mf = it->second[0]->GetMakefile(); // Is this safe with index 0 ?
    std::string projectName = it->second[0]->GetProjectName();

    if (strcmp(mf->GetCurrentBinaryDirectory(), mf->GetHomeOutputDirectory()) == 0)
    {
      workspaceOutputDir = mf->GetCurrentBinaryDirectory();
      workspaceProjectName = projectName; //mf->GetProjectName();
      workspaceSourcePath = mf->GetHomeDirectory();
      workspaceFileName = workspaceOutputDir+"/";
      workspaceFileName+= workspaceProjectName + ".workspace";
      break;
    }
  }

logf2 << "ws dir: " << workspaceOutputDir << " => " << workspaceFileName << std::endl;

  // The following section attempts to find and remember the current active project. Will be restored at the end.
	std::string activeProject;
	{
		std::ifstream ifs(workspaceFileName.c_str());
		if (ifs.good())
		{
			std::string tmp;
			while (cmSystemTools::GetLineFromStream(ifs, tmp))
			{
				char sz1[200],sz2[200],sz3[200] = "", sz4[200] = "";
				// Looking for:  <Project Name="myproject" Path="/home/user/somewhere/cmake/build/myproject/myproject.project" Active="Yes"/>
				if (sscanf(tmp.c_str(),"%s Name=\"%s Path=\"%s Active=\"%s\"", sz1, sz2, sz3, sz4 ) >= 4)
				{
          logf2 << "Found project: " << sz1 << " : " << sz2 << " : " << sz3 << " : " << sz4 << std::endl;
					if (std::string(sz4).substr(0,3) == "Yes")
					{
						size_t pos = std::string(sz2).find("\"");
						if ( pos != std::string::npos )
						{
							activeProject = std::string(sz2).substr(0,pos);
						}
					}
				}
      }
    }
  }

  fout.Open(workspaceFileName.c_str(),false,false);
  fout << this->CreateWorkspaceHeader(workspaceProjectName);

  // For all the projects, for all the generators, find all the targets
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
    it = this->GlobalGenerator->GetProjectMap().begin();
    it != this->GlobalGenerator->GetProjectMap().end();
    ++it)
  {
    for (std::vector<cmLocalGenerator*>::const_iterator lg= it->second.begin(); lg!=it->second.end(); lg++)
    {
      //cmMakefile* makefile=(*lg)->GetMakefile();
      //cmTargets& targets=makefile->GetTargets();
      //for (cmTargets::iterator ti = targets.begin(); ti != targets.end(); ti++)
      std::vector<cmGeneratorTarget*> targets=(*lg)->GetGeneratorTargets();
      for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin(); ti != targets.end(); ti++)
      {
        auto & target = *(*ti);

        logf2 << "\tTarget: " << " : " << target.GetType() << " : " << target.GetName() << endl;
        switch(target.GetType())
        {
        case cmState::EXECUTABLE: // 0
        case cmState::STATIC_LIBRARY: // 1
        case cmState::SHARED_LIBRARY: // 2
        //case cmTarget::MODULE_LIBRARY:
        //case cmTarget::OBJECT_LIBRARY:
//NB!!        
        case cmState::UTILITY: // 5
        {
          std::string projectName;
          std::string filename;

         std::vector<cmGeneratorTarget*> targetGenerators=(*lg)->GetGeneratorTargets();
          
          //cmGeneratorTarget* gt = *targetGenerators.begin();
          if ( this->CreateProjectFile(target, *(*lg), projectName, filename) )
          {
              std::string activeValue = "No";
              if ( projectName == activeProject ) // || activeProject.empty() )
              {
                activeValue = "Yes";
              }
              fout << "  <Project Name=\"" << projectName << "\" Path=\"" << filename << "\" Active=\"" << activeValue << "\"/>\n";
              configCMake += "      <Project Name=\"" + projectName + "\" ConfigName=\"CMake\"/>\n";
            }
          }
          break;
        default:
          break;
        }
      }
    }
  }
  fout << this->CreateWorkspaceFooter(configCMake);
}


std::string cmExtraCodeLiteGenerator2::CreateProjectHeader( const std::string& projectName )
{
  std::ostringstream result;
  result << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<CodeLite_Project Name=\"" << projectName << "\" InternalType=\"\">\n\n";
  return result.str();
}


std::string cmExtraCodeLiteGenerator2::CreateProjectFooter( const std::string& projectType, const std::string& make_cmd, const std::string& generalTag )
{
   std::ostringstream result;

   result << "\n"
      "  <Settings Type=\"" << projectType << "\">\n"
      "    <Configuration Name=\"CMake\" CompilerType=\"gnu g++\" DebuggerType=\"GNU gdb debugger\" Type=\"" << projectType << "\" BuildCmpWithGlobalSettings=\"append\" BuildLnkWithGlobalSettings=\"append\" BuildResWithGlobalSettings=\"append\">\n"
      "      <Compiler Options=\"-g\" Required=\"yes\" PreCompiledHeader=\"\">\n"
      "        <IncludePath Value=\".\"/>\n"
      "      </Compiler>\n"
      "      <Linker Options=\"\" Required=\"yes\"/>\n"
      "      <ResourceCompiler Options=\"\" Required=\"no\"/>\n"
		<< generalTag <<
      "      <Debugger IsRemote=\"no\" RemoteHostName=\"\" RemoteHostPort=\"\" DebuggerPath=\"\">\n"
      "        <PostConnectCommands/>\n"
      "        <StartupCommands/>\n"
      "      </Debugger>\n"
      "      <CustomBuild Enabled=\"yes\">\n"
      "        <RebuildCommand/>\n"
      "        <CleanCommand>make clean</CleanCommand>\n"
      "        <BuildCommand>" << make_cmd << " $(ProjectName)</BuildCommand>\n"
      "        <PreprocessFileCommand>" << make_cmd << " -f$(ProjectPath)/Makefile $(CurrentFileName).i</PreprocessFileCommand>\n"
//      "        <SingleFileCommand>" << make_cmd << " -f$(ProjectPath)/Makefile $(ProjectPath)A/$(WorkspacePath)B/$(ProjectName)C/$(IntermediateDirectory)D/$(ConfigurationName)E/$(OutDir)F/$(CurrentFilePath)G/$(CurrentFileFullPath)H/$(CodeLitePath)I/$(OutputFile)J/$(ObjectName)K/$(ObjectSuffix)L/$(FileName)M/$(FileFullName)N/$(FileFullPath)O $(CurrentFileName).o</SingleFileCommand>\n"
      "        <SingleFileCommand>" << make_cmd << " -f$(ProjectPath)/Makefile $(CurrentFileName).o</SingleFileCommand>\n"
//      "        <MakefileGenerationCommand>cmake-gui " << workspaceOutputDir << "</MakefileGenerationCommand>\n"
      "        <ThirdPartyToolName>Other</ThirdPartyToolName>\n"
      "        <WorkingDirectory>$(IntermediateDirectory)</WorkingDirectory>\n"
      "      </CustomBuild>\n"
      "     <Completion EnableCpp11=\"yes\">\n"
      "        <SearchPaths/>\n"
      "     </Completion>\n"
      "    </Configuration>\n"
      "  </Settings>\n"
      "  <Dependencies Name=\"CMake\"/>\n"
      "</CodeLite_Project>\n";

   return result.str();
}

class source_group
{
public:

  source_group(std::string name, std::string regex)
  : sogr_(name.c_str(),regex.c_str())
  {
  }

  source_group(cmSourceGroup g)
  : sogr_(g){}

  cmSourceGroup sogr_;
  std::vector<std::string> filenames_;
};



bool cmExtraCodeLiteGenerator2::CreateProjectFile(const cmGeneratorTarget &_target, const cmLocalGenerator& _local, 
  std::string &_projectName, std::string &_projectFilename)
{
  // Retrieve information from the cmake makefile
  const cmMakefile* mf = _local.GetMakefile();
  const std::vector<cmSourceGroup> &sogr1 = mf->GetSourceGroups();
  std::vector<source_group> sogr;
  for (std::vector<cmSourceGroup>::const_iterator it = sogr1.begin(); it != sogr1.end(); it++)
  {
    sogr.insert(sogr.begin(),source_group(*it));
  }
//  sogr.push_back(source_group("source","(.*)\\.cpp"));
//  sogr.push_back(source_group("include","(.*)\\.h"));
//  sogr.push_back(source_group("other","(.*)"));


  std::string outputDir=mf->GetCurrentBinaryDirectory();

  _projectName= _target.GetName();
  std::string targettype; // Defaults to empty if no recognized target type is detected.
  std::string make_cmd = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string workspaceSourcePath = mf->GetHomeDirectory();
  //std::string cmakelist = mf->GetCurrentListFile();

  _projectFilename = outputDir + "/" + _projectName + ".project";
  std::string cmakelist_txt = std::string(mf->GetCurrentSourceDirectory()) + "/CMakeLists.txt";

logf2 << "Project: " << _projectName << " " << _projectFilename << " " << mf->GetHomeDirectory() << " | " << mf->GetHomeOutputDirectory() << " | " << mf->GetCurrentSourceDirectory() << " | " << mf->GetCurrentBinaryDirectory() << "|" << outputDir << std::endl;

  // We try to merge some of the usable information from the project file.
  std::string generalTag = "      <General OutputFile=\"$(IntermediateDirectory)/$(ProjectName)\" IntermediateDirectory=\"" + outputDir + "\" Command=\"$(IntermediateDirectory)/$(ProjectName)\" CommandArguments=\"\" WorkingDirectory=\"$(IntermediateDirectory)\" PauseExecWhenProcTerminates=\"yes\"/>\n";
  {
    // Look in the existing project file (if it exists) for the <General OutputFile tag.
    // It contains the debug settings i.e: OutputFile, IntermediateDirectory, Command, CommandArguments 
    std::ifstream ifs(_projectFilename.c_str());
    if (ifs.good())
    {
      std::string tmp;
      while (cmSystemTools::GetLineFromStream(ifs, tmp))
      {
        if (tmp.find("<General ") != std::string::npos && tmp.find("OutputFile") != std::string::npos)
        {
          generalTag = tmp + "\n";
        }
      }
    }
  }

  cmGeneratedFileStream fout(_projectFilename.c_str());
  fout << this->CreateProjectHeader(_projectName);

  // The following file lists are generated in each own virtual folder
  // This would be nice to make configurable.
  //std::vector<std::string> source_files, include_files, other_files, rule_files, qt_files, web_files;

  //other_files.push_back( cmakelist ); // Add the CMakeLists.txt file.

  logf2 << "\tTarget: " << _target.GetType() << " : " << _target.GetName() << endl;

  // The following casting is only required until the const issue is fixed in cmTarget.h
  //const std::vector<cmSourceFile*> &sources = ((cmTarget&)_target).GetSourceFiles();
  std::vector<cmSourceFile*> sources;
  //_target.GetSourceFiles(sources,mf->GetSafeDefinition("CMAKE_BUILD_TYPE"));
  //sources.push_back(new cmSourceFile(mf,cmakelist));
  //cmGeneratorTarget* gt = &_target;
  _target.GetSourceFiles(sources, mf->GetSafeDefinition("CMAKE_BUILD_TYPE"));

  
  std::vector<std::string> filenames;
  for (std::vector<cmSourceFile*>::const_iterator si=sources.begin(); si!=sources.end(); si++)
  {
logf2 << "Target: " << _projectName << " => " << (*si)->GetFullPath() << std::endl;
    filenames.push_back((*si)->GetFullPath());
  }

  {
    std::ifstream ifs(cmakelist_txt);
    if (ifs.good())
    {
logf2 << "Found: " << cmakelist_txt << std::endl; 
      filenames.push_back(cmakelist_txt);
    }
    else
    {
logf2 << "Failed: " << cmakelist_txt << std::endl;
    }
  }
//NB!!!  filenames.push_back(cmakelist);
  for (std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); it++)
  {
     for (std::vector<source_group>::iterator sgi = sogr.begin(); sgi != sogr.end(); sgi++)
     {
       //logf2 << "sg: " << sgi->sogr_.GetName() << std::endl;
       if (sgi->sogr_.MatchesFiles((*it).c_str()) || sgi->sogr_.MatchesRegex((*it).c_str()))
       {
         sgi->filenames_.push_back((*it));
         break;
       }
     }
  }
  
/*  
  
  for (std::vector<cmSourceFile*>::const_iterator si=sources.begin(); si!=sources.end(); si++)
  {
     logf2 << "\tCFile: " << (*si)->GetFullPath() << endl;
     std::string lang;
     cmSourceFile &source = *(*si);
     if ( !source.GetLanguage().empty() )
     {
        lang = source.GetLanguage();
     }
     std::string ext = source.GetExtension(); // Should be lowercase.
     std::string fullname = source.GetFullPath(); // Should be lowercase.
     if (!cmSystemTools::FileExists(fullname.c_str()) )
     {
       logf << "File: " << fullname << " does not exist. It is not being added" << endl;
       continue;
     }
     
     for (std::vector<source_group>::iterator sgi = sogr.begin(); sgi != sogr.end(); sgi++)
     {
       logf << "sg: " << sgi->sogr_.GetName() << std::endl;
       
       const std::vector<const cmSourceFile*>& sgf = sgi->sogr_.GetSourceFiles();
       for (std::vector<const cmSourceFile*>::const_iterator sgfit = sgf.begin(); sgfit != sgf.end(); sgfit++)
       {
         const cmSourceFile *p = (*sgfit);
         logf << " File: " << p->GetFullPath() << std::endl;
       }
       logf << "--" << std::endl;

        if (sgi->sogr_.MatchesFiles(fullname.c_str()) || sgi->sogr_.MatchesRegex(fullname.c_str()))
        {
          sgi->filenames_.push_back(fullname);
          break;
        }
     }

     // The following filter is not very good. It is considered a hack.
     if ( lang == "C" || lang == "CXX" )
     {
        if ( fullname.find("_moc.cpp") != std::string::npos || fullname.find("_uic.cpp") != std::string::npos )
        {
           qt_files.push_back(source.GetFullPath());
        }
        else
        {
           source_files.push_back(source.GetFullPath());
        }
     }
     else if ( ext == "h" || ext == "hpp" )
     {
        include_files.push_back(source.GetFullPath());
     }
     else if ( ext == "ui" )
     {
        qt_files.push_back(source.GetFullPath());
     }
     else if ( ext == "js" || ext == "html" || ext == "css" )
     {
        web_files.push_back(source.GetFullPath());
     }
     else if ( ext == "rule" )
     {
        rule_files.push_back(source.GetFullPath());
     }
     else
     {
        other_files.push_back(source.GetFullPath());
     }

  }

  this->AddFolder(include_files, "include",fout);
  this->AddFolder(source_files, "source",fout);
  this->AddFolder(other_files, "other",fout);
  this->AddFolder(rule_files, "rule",fout);
  this->AddFolder(qt_files, "qt",fout);
  this->AddFolder(web_files, "web",fout);
*/
  for (std::vector<source_group>::iterator sgi = sogr.begin(); sgi != sogr.end(); sgi++)
  {
    this->AddFolder(sgi->filenames_,sgi->sogr_.GetName(),fout);
  }
  fout << this->CreateProjectFooter(targettype,make_cmd,generalTag);

  return true; //source_files.size() > 0 || include_files.size() > 0  || web_files.size() > 0 || other_files.size() > 1;
}


void cmExtraCodeLiteGenerator2::AddFolder( const std::vector<std::string>& folder, const std::string& foldername, cmGeneratedFileStream & fout )
{
   if ( folder.size() > 0 )
   {
      fout << "<VirtualDirectory Name=\"" << foldername << "\">\n";
      for ( std::vector<std::string>::const_iterator iter = folder.begin(); iter != folder.end(); iter++)
      {
         fout << "<File Name=\"" << (*iter) << "\"/>\n";
      }
      fout << "</VirtualDirectory>\n";
   }
}
