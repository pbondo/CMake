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

class cmLocalGenerator;


class cmExtraCodeLiteGenerator2 : public cmExternalMakefileProjectGenerator
{
public:

  cmExtraCodeLiteGenerator2();

  std::string CreateWorkspaceHeader( const std::string& name );
  std::string CreateWorkspaceFooter( const std::string& config );

  std::string CreateProjectHeader( const std::string& name );
  std::string CreateProjectFooter( const std::string& projectType, const std::string& make_cmd, const std::string& generalTag );

  virtual std::string GetName() const override
  { return cmExtraCodeLiteGenerator2::GetActualName();}

  static const char* GetActualName()                     
  { return "CodeLite2";}

  static cmExternalMakefileProjectGenerator* New()
  { return new cmExtraCodeLiteGenerator2; }

  // Get the documentation entry for this generator.
  virtual void GetDocumentation(cmDocumentationEntry& entry, 
                                const std::string& fullName) const override;

  virtual void Generate() override;

  bool CreateProjectFile(const cmGeneratorTarget &_genTarget, //const cmTarget& lgs, 
    const cmLocalGenerator& _local, 
    std::string &_projectname, std::string &_projectfilename );

  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                                const std::string& filename);

  void AddFolder( const std::vector<std::string>& folder, const std::string& foldername, cmGeneratedFileStream& fout );

private:
};

#endif
