mitkFunctionCheckCompilerFlags("-Wno-deprecated-declarations" CMAKE_CXX_FLAGS)

MITK_CREATE_MODULE( IpPicSupport
  DEPENDS Mitk LegacyAdaptors mitkIpPic
  DEPRECATED_SINCE 2013.12
  )

add_subdirectory(Testing)
