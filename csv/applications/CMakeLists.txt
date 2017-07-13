SET( dir ${SOURCE_CODE_BASE_DIR}/csv/applications )
FILE( GLOB source   ${dir}/*.cpp)
FILE( GLOB includes ${dir}/*.h)
FILE( GLOB play_source   ${dir}/play/*.cpp)
FILE( GLOB play_includes ${dir}/play/*.h)
SOURCE_GROUP( ${TARGET_NAME} FILES ${play_source} ${play_includes} ${source} ${includes} )

add_executable( csv-quote csv-quote.cpp )
target_link_libraries( csv-quote ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )
install( TARGETS csv-quote RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( csv-fields ${dir}/csv-fields.cpp )
add_executable( csv-format ${dir}/csv-format.cpp )
add_executable( csv-size ${dir}/csv-size.cpp )
add_executable( csv-select ${dir}/csv-select.cpp )
add_executable( csv-bin-cut ${dir}/csv-bin-cut.cpp )
add_executable( csv-from-columns ${dir}/csv-from-columns.cpp )
add_executable( csv-join ${dir}/csv-join.cpp )
add_executable( csv-sort ${dir}/csv-sort.cpp )
add_executable( csv-paste ${dir}/csv-paste.cpp )
add_executable( csv-split ${dir}/csv-split.cpp ${dir}/split/split.cpp ${dir}/split/split.h )
add_executable( csv-time ${dir}/csv-time.cpp )
add_executable( csv-time-delay ${dir}/csv-time-delay.cpp )
add_executable( csv-time-join ${dir}/csv-time-join.cpp )
add_executable( csv-time-stamp ${dir}/csv-time-stamp.cpp )
add_executable( csv-to-bin ${dir}/csv-to-bin.cpp )
add_executable( csv-from-bin ${dir}/csv-from-bin.cpp )
add_executable( csv-calc ${dir}/csv-calc.cpp )
add_executable( csv-calc-new ${dir}/csv-calc.new.cpp )
add_executable( csv-crc ${dir}/csv-crc.cpp )
add_executable( csv-play ${dir}/csv-play.cpp ${dir}/play/multiplay.cpp ${dir}/play/play.cpp )
add_executable( csv-shuffle ${dir}/csv-shuffle.cpp )
add_executable( csv-thin ${dir}/csv-thin.cpp )
add_executable( csv-analyse ${dir}/csv-analyse.cpp )
add_executable( csv-to-sql ${dir}/csv-to-sql.cpp )

target_link_libraries ( csv-fields ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )
target_link_libraries ( csv-format ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )
target_link_libraries ( csv-size ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )
target_link_libraries ( csv-bin-cut ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv comma_xpath )
target_link_libraries ( csv-split comma_csv comma_application comma_string comma_xpath ${comma_ALL_EXTERNAL_LIBRARIES} )
target_link_libraries ( csv-from-columns ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_io comma_string )
target_link_libraries ( csv-join ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_xpath comma_string )
target_link_libraries ( csv-sort ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_xpath comma_string )
target_link_libraries ( csv-select ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_xpath comma_string )
target_link_libraries ( csv-paste ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv comma_io )
target_link_libraries ( csv-time ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_xpath comma_string )
target_link_libraries ( csv-time-delay ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_string comma_xpath )
target_link_libraries ( csv-time-join ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_string comma_xpath )
target_link_libraries ( csv-time-stamp ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_string )
target_link_libraries ( csv-to-bin ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application ) # profiler )
target_link_libraries ( csv-from-bin ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application )
target_link_libraries ( csv-calc ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application comma_string )
target_link_libraries ( csv-calc-new ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application comma_string )
target_link_libraries ( csv-crc ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application comma_string )
target_link_libraries ( csv-play ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application comma_io )
target_link_libraries ( csv-shuffle ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )
target_link_libraries ( csv-thin ${comma_ALL_EXTERNAL_LIBRARIES} comma_csv comma_xpath comma_application comma_io )
target_link_libraries ( csv-analyse ${comma_ALL_EXTERNAL_LIBRARIES} comma_application )
target_link_libraries ( csv-to-sql ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_csv )

install( TARGETS csv-bin-cut
                 csv-fields
                 csv-format
                 csv-join
                 csv-sort
                 csv-from-columns
                 csv-paste
                 csv-split
                 csv-time
                 csv-time-delay
                 csv-time-join
                 csv-time-stamp
                 csv-to-bin
                 csv-from-bin
                 csv-size
                 csv-calc
                 csv-play
                 csv-shuffle
                 csv-crc
                 csv-select
                 csv-thin
                 csv-analyse
                 csv-to-sql
         RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR}
         COMPONENT Runtime )

add_executable( csv-blocks ${dir}/csv-blocks.cpp )
target_link_libraries ( csv-blocks ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_io comma_string comma_xpath comma_csv )
install( TARGETS csv-blocks RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( csv-cast ${dir}/csv-cast.cpp )
target_link_libraries ( csv-cast ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv )
install( TARGETS csv-cast RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( csv-enumerate ${dir}/csv-enumerate.cpp )
target_link_libraries ( csv-enumerate ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_io comma_string comma_xpath comma_csv )
install( TARGETS csv-enumerate RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( csv-interval ${dir}/csv-interval.cpp )
target_link_libraries ( csv-interval ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_xpath )
install( TARGETS csv-interval RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )
         
add_executable( csv-units ${dir}/csv-units.cpp )
target_link_libraries ( csv-units ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_csv comma_io comma_xpath comma_string )
install( TARGETS csv-units RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

add_executable( csv-update ${dir}/csv-update.cpp )
target_link_libraries ( csv-update ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_io comma_string comma_xpath comma_csv )
install( TARGETS csv-update RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )

if( NOT WIN32 )
    add_executable( csv-repeat ${dir}/csv-repeat.cpp )
    target_link_libraries ( csv-repeat comma_application comma_csv comma_io )
    install( TARGETS csv-repeat RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )
endif()

add_executable( csv-bin-reverse ${dir}/csv-bin-reverse.cpp )
target_link_libraries ( csv-bin-reverse ${comma_ALL_EXTERNAL_LIBRARIES} comma_application comma_string comma_xpath comma_csv )
install( TARGETS csv-bin-reverse RUNTIME DESTINATION ${comma_INSTALL_BIN_DIR} COMPONENT Runtime )
