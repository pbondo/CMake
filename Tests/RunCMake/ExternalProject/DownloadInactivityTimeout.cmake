include(ExternalProject)
ExternalProject_Add(MyProj URL ${SERVER_URL} INACTIVITY_TIMEOUT 2 DOWNLOAD_NO_EXTRACT TRUE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "")
