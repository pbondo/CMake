/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2004 Alexander Neundorf (neundorf@kde.org)
  Copyright 2012-2020 Poul Bondo (poul.bondo@gmail.com)

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
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
#include "cmSystemTools.h"

#include <cmsys/SystemTools.hxx>
#include <cmsys/Directory.hxx>

#include <fstream>
using namespace std;

static std::ofstream logf2("codelite2.log");

//----------------------------------------------------------------------------
cmExtraCodeLiteGenerator2::cmExtraCodeLiteGenerator2()
:cmExternalMakefileProjectGenerator()
{
}

cmExternalMakefileProjectGeneratorFactory*
cmExtraCodeLiteGenerator2::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmExtraCodeLiteGenerator2>
    factory("CodeLite2", "Generates CodeLite project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
    factory.AddSupportedGlobalGenerator("Ninja");
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
  }

  return &factory;
}

std::string cmExtraCodeLiteGenerator2::CreateWorkspaceHeader()
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
   std::string workspaceFileName;
   std::string configCMake;
   cmGeneratedFileStream fout;

   // loop projects and locate the root project.
   for (auto& p : this->GlobalGenerator->GetProjectMap())
   {
      auto mf = p.second[0]->GetMakefile();
      this->generator = mf->GetSafeDefinition("CMAKE_GENERATOR");
      if (mf->GetCurrentBinaryDirectory() == mf->GetHomeOutputDirectory())
      {
         sourcePath = std::string(mf->GetCurrentSourceDirectory()) + "/";
         workspaceProjectName = p.second[0]->GetProjectName();
         workspacePath = std::string(mf->GetCurrentBinaryDirectory()) + "/";
         workspaceFileName = workspacePath + workspaceProjectName + ".workspace";
         break;
      }
   }
   this->LoadSettingFile(sourcePath + "settings.codelite");
   logf2 << "workspace: " << workspaceProjectName << " => " << workspaceFileName << " source " << sourcePath << std::endl;

   // The following section attempts to find and remember the current active project. Will be restored at the end.
   std::string activeProject;
   {
      std::ifstream ifs(workspaceFileName.c_str());
		if (ifs.good())
		{
			std::string tmp;
			while (cmSystemTools::GetLineFromStream(ifs, tmp))
			{
            char sz1[1000] = "", sz2[1000] = "", sz3[1000] = "", sz4[1000] = "";
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
   fout << this->CreateWorkspaceHeader();

  // For all the projects, for all the generators, find all the targets
  for (auto& p : this->GlobalGenerator->GetProjectMap())
  {
    // Local Generators
    for (auto& lg : p.second)
    {
      // Generator Targets
      for (auto& gt : lg->GetGeneratorTargets())
      {
        logf2 << "\tTarget: " << " : " << gt->GetType() << " : " << gt->GetName() << endl;
        switch(gt->GetType())
        {
        case cmStateEnums::EXECUTABLE: // 0
        case cmStateEnums::STATIC_LIBRARY: // 1
        case cmStateEnums::SHARED_LIBRARY: // 2
        //case cmTarget::MODULE_LIBRARY:
        //case cmTarget::OBJECT_LIBRARY:
        {
          std::string projectName;
          std::string filename;
          if (this->CreateProjectFile(*gt, *lg, projectName, filename))
          {
              std::string activeValue = "No";
              if (projectName == activeProject)
              {
                activeValue = "Yes";
              }
              fout << "  <Project Name=\"" << projectName << "\" Path=\"" << filename << "\" Active=\"" << activeValue << "\"/>\n";
              configCMake += "      <Project Name=\"" + projectName + "\" ConfigName=\"CMake\"/>\n";
            }
          }
          break;
        case cmStateEnums::UTILITY:             // 5
        case cmStateEnums::GLOBAL_TARGET:       // 6
        case cmStateEnums::INTERFACE_LIBRARY:   // 7
        case cmStateEnums::UNKNOWN_LIBRARY:     // 8
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

std::string cmExtraCodeLiteGenerator2::CreateProjectFooter(const std::string& projectType, const std::string& make_cmd,
   const std::string& generalTag, const std::string& projectName)
{
   std::string make_single_target = make_cmd + " -f$(ProjectPath)/Makefile $(CurrentFileName).o";
   std::string make_project = make_cmd + " " + projectName;

   logf2 << "gen " << generator << " " << make_cmd << " " << endl;

   if (generator == "Ninja")
   {  // ninja -C /home/pba/tmp/cl2/ /home/pba/user/gh/ais-r138/impl/atonmonitor/aisconf.cpp^
      make_single_target = make_cmd + " -C " + workspacePath + " $(CurrentFilePath)/$(CurrentFileFullName)^";
      //make_single_target = make_cmd + " -C " + workspacePath + " $(CurrentFilePath)/$(CurrentFileName).cpp^"; // This trick will match header files with .cpp files, but fail with other types of files?
      //make_single_target = make_cmd + " -C " + workspacePath + " $(ProjectName)/CMakeFiles/$(ProjectName).dir/$(CurrentFileFullName).o^";
      make_project       = make_cmd + " -C " + workspacePath + " $(ProjectName)";
   }
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
      //"        <BuildCommand>" << make_cmd << " " << projectName << "</BuildCommand>\n"
      "        <BuildCommand>" << make_project << "</BuildCommand>\n"
      "        <PreprocessFileCommand>" << make_cmd << " -f$(ProjectPath)/Makefile $(CurrentFileName).i</PreprocessFileCommand>\n"
      //"        <SingleFileCommand>" << make_cmd << " -f$(ProjectPath)/Makefile $(CurrentFileName).o</SingleFileCommand>\n"
      "        <SingleFileCommand>" << make_single_target << "</SingleFileCommand>\n"
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
   std::string outputDir=mf->GetCurrentBinaryDirectory();
   _projectName= _target.GetName();
   std::string targettype; // Defaults to empty if no recognized target type is detected.
   std::string make_cmd = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");

   _projectFilename = outputDir + "/" + _projectName + ".project";

   //logf2 << "Project: " << _projectName << " " << _projectFilename << " " << mf->GetHomeDirectory() << " | " << mf->GetHomeOutputDirectory() << " | " << mf->GetCurrentSourceDirectory() << " | " << mf->GetCurrentBinaryDirectory() << "|" << outputDir << std::endl;
   std::string workingDirectory = "$(IntermediateDirectory)";
   if (auto wd = this->mapping[_projectName]["Working Directory"]; !wd.empty())
   {
      workingDirectory = wd;
   }
   std::string commandArguments = this->mapping[_projectName]["Program Arguments"]; // Using the names from the codelite visual project settings

  // We try to merge some of the usable information from the project file.
  std::string generalTag = "      <General OutputFile=\"$(IntermediateDirectory)/$(ProjectName)\" IntermediateDirectory=\"" + outputDir + "\" Command=\"$(IntermediateDirectory)/$(ProjectName)\" CommandArguments=\"" + commandArguments + "\" WorkingDirectory=\"" + workingDirectory + "\" PauseExecWhenProcTerminates=\"yes\"/>\n";
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

   //logf2 << "\tTarget: " << _target.GetType() << " : " << _target.GetName() << endl;

   std::vector<cmSourceFile*> sources;
   _target.GetSourceFiles(sources, mf->GetSafeDefinition("CMAKE_BUILD_TYPE"));

  
  std::vector<std::string> filenames;
  //for (std::vector<cmSourceFile*>::const_iterator si=sources.begin(); si!=sources.end(); si++)
  for (auto& si : sources)
  {
    //logf2 << "Target: " << _projectName << " => " << (si)->GetFullPath() << std::endl;
    filenames.push_back((si)->GetFullPath());
  }

  { // Check if we have a local CMakeLists.txt we want to add.
    std::string cmakelist_txt = std::string(mf->GetCurrentSourceDirectory()) + "/CMakeLists.txt";
    std::ifstream ifs(cmakelist_txt);
    if (ifs.good())
    {
      filenames.push_back(cmakelist_txt);
    }
  }
  // For each file check if we have a matching source group.
  for (auto& file : filenames)
  {
     for (auto& sg : sogr)
     {
       if (sg.sogr_.MatchesFiles(file.c_str()) || sg.sogr_.MatchesRegex(file.c_str()))
       {
         sg.filenames_.push_back(file);
         break;
       }
     }
  }
  for (auto& sg : sogr)
  {
    this->AddFolder(sg.filenames_, sg.sogr_.GetName(), fout);
  }
  std::string project_build = "$(ProjectName)";
  if (_projectName == workspaceProjectName)
  {
     project_build.clear();
  }
  std::string j;
  if (cmsys::SystemTools::GetEnv("NUMCPUS", j) && !j.empty())
  {
     project_build = " -j " + j + " " + project_build;
  }
  fout << this->CreateProjectFooter(targettype, make_cmd, generalTag, project_build);
  return true;
}

void cmExtraCodeLiteGenerator2::AddFolder( const std::vector<std::string>& folder, std::string foldername, cmGeneratedFileStream & fout )
{
   if ( folder.size() > 0 )
   {
      if (foldername.empty())
      {
         foldername = "Other";
      }
      fout << "<VirtualDirectory Name=\"" << foldername << "\">\n";
      for ( std::vector<std::string>::const_iterator iter = folder.begin(); iter != folder.end(); iter++)
      {
         fout << "<File Name=\"" << (*iter) << "\"/>\n";
      }
      fout << "</VirtualDirectory>\n";
   }
}

void cmExtraCodeLiteGenerator2::LoadSettingFile(std::string filename)
{
   std::ifstream ifs(filename);
   if (ifs.good())
   {
      std::string line;
      while (std::getline(ifs, line))
      {
         auto v = cmSystemTools::SplitString(line, '|');
         if (v.size() > 1) // Not really interesting unless we have some data
         {
            for (size_t index = 1; index < v.size(); index++)
            {
               auto kv = cmSystemTools::SplitString(v[index], ':');
               if (kv.size() == 2)
               {
                  this->mapping[v[0]][kv[0]] = kv[1];
               }
            }
         }
      }
      for (auto m : this->mapping)
      {
         for (auto e : m.second)
         {
            logf2 << "Setting: " << m.first << " " << e.first << "=" << e.second << endl;
         }
      }
   }
}
