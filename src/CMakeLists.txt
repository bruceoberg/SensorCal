set(SRCS gui.cpp gui.h portlist.cpp portscanner.cpp portscanner.h serialdata.cpp serialdata.h visualize.cpp visualize.h)
set(PNGS checkempty.png checkemptygray.png checkgreen.png)

find_package(OpenGL REQUIRED)
find_package(wxWidgets REQUIRED COMPONENTS core base gl)
include(${wxWidgets_USE_FILE})

include(${image2wx_USE_FILE})
generate_image2wx_sources(PNG_SRCS "${PNGS}")
list(APPEND SRCS ${PNG_SRCS})

add_executable(SensorCal WIN32 MACOSX_BUNDLE ${SRCS})

if(APPLE)
	message("[SensorCal] building for mac")

	# Add icon as a resource to be copied into the bundle

	set(MACOS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
	set_source_files_properties(${MACOS_ICON} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources")
	target_sources(SensorCal PRIVATE ${MACOS_ICON})

	# Set a custom plist file for the app bundle - needed for Mac OS Retina display

	set_target_properties(SensorCal
		PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
		MACOSX_BUNDLE_ICON_FILE icon.icns
		)
	
	target_compile_definitions(SensorCal PUBLIC "MACOSX")
	target_compile_definitions(SensorCal PUBLIC "GL_SILENCE_DEPRECATION")

	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/entitlements-release.plist)
	else()
		set(ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/entitlements-debug.plist)
	endif()

	add_custom_command(TARGET SensorCal POST_BUILD
		COMMAND codesign -s - --entitlements ${ENTITLEMENTS} --force $<TARGET_BUNDLE_DIR:SensorCal>
		COMMENT "Ad-hoc signing SensorCal with ${ENTITLEMENTS}"
	)
elseif(WIN32)
	message("[SensorCal] building for windows")
    target_sources(SensorCal PRIVATE main.exe.manifest)
	target_compile_definitions(SensorCal PUBLIC "WINDOWS")
else()
	target_compile_definitions(SensorCal PUBLIC "LINUX")
endif()

target_link_libraries(SensorCal PUBLIC ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARY} libcalib image2wx serial_cpp glm::glm)
