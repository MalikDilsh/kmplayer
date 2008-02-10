project(kmplayer)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories( ${KDE4_INCLUDES})

if(NOT WIN32)
  OPTION(WITH_CAIRO "Enable Cairo support" ON)
  OPTION(WITH_NPP "Build NPP player" ON)
  INCLUDE(UsePkgConfig)

  if (WITH_CAIRO)

    PKGCONFIG(cairo CAIROIncDir CAIROLinkDir CAIROLinkFlags CAIROCflags)

    if (CAIROCflags)
        set(KMPLAYER_WITH_CAIRO 1)
    endif (CAIROCflags)
  endif (WITH_CAIRO)

  if (WITH_NPP)
    PKGCONFIG(dbus-glib-1 GLibDBusIncDir GLibDBusLinkDir GLibDBusLinkFlags GLibDBusCflags)

    if (GLibDBusCflags)
      set(KMPLAYER_WITH_GDBUS 1)
      PKGCONFIG(gtk+-x11-2.0 GTKIncDir GTKLinkDir GTKLinkFlags GTKCflags)
      PKGCONFIG(gthread-2.0 GThreadIncDir GThreadLinkDir GThreadLinkFlags GThreadCflags)
      if (GTKCflags)
          set(KMPLAYER_WITH_NPP 1)
      endif (GTKCflags)
    endif (GLibDBusCflags)
  endif (WITH_NPP)

endif(NOT WIN32)

configure_file (config-kmplayer.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kmplayer.h )

add_subdirectory(src)
add_subdirectory(icons)
#add_subdirectory(mimetypes)