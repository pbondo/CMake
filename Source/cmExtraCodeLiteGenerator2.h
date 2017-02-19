/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2004 Alexander Neundorf (neundorf@kde.org)
  Copyright 2012-2016 Poul Bondo (poul.bondo@gmail.com)

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef cmGlobalCodeLiteGenerator2_h
#define cmGlobalCodeLiteGenerator2_h

#include "cmExternalMakefileProjectGenerator.h"
#include "cmGeneratorTarget.h"
#include "cmGeneratedFileStream.h"

class cmLocalGenerator;


class cmExtraCodeLiteGenerator2 : public cmExternalMakefileProjectGenerator
{
public:

  cmExtraCodeLiteGenerator2();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  std::string CreateWorkspaceHeader();
  std::string CreateWorkspaceFooter( const std::string& config );

  std::string CreateProjectHeader(const std::string& name);
  std::string CreateProjectFooter(const std::string& projectType, const std::string& make_cmd,
    const std::string& generalTag, const std::string& projectName);

  void Generate() CM_OVERRIDE;

  bool CreateProjectFile(const cmGeneratorTarget &_genTarget,
    const cmLocalGenerator& _local, std::string &_projectname, std::string &_projectfilename);

  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs, const std::string& filename);

  void AddFolder( const std::vector<std::string>& folder, const std::string& foldername, cmGeneratedFileStream& fout );

private:

  std::string workspaceProjectName;

};

#endif
