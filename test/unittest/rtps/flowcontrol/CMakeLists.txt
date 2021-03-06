# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(NOT ((MSVC OR MSVC_IDE) AND EPROSIMA_INSTALLER))
    include(${PROJECT_SOURCE_DIR}/cmake/dev/gtest.cmake)
    check_gtest()

    if(GTEST_FOUND)
        if(WIN32)
            add_definitions(-D_WIN32_WINNT=0x0601)
        endif()

        include_directories(mock/)
        include_directories(${ASIO_INCLUDE_DIR})

        set(THROUGHPUTCONTROLLERTESTS_SOURCE
            ThroughputControllerTests.cpp
            mock/mock.RTPSParticipantImpl.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/AsyncWriterThread.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/resources/AsyncInterestTree.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/FlowController.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputController.cpp
            ${PROJECT_SOURCE_DIR}/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp)

        add_executable(ThroughputControllerTests ${THROUGHPUTCONTROLLERTESTS_SOURCE})
        add_gtest(ThroughputControllerTests ${THROUGHPUTCONTROLLERTESTS_SOURCE})
        target_compile_definitions(ThroughputControllerTests PRIVATE FASTRTPS_NO_LIB)
        target_include_directories(ThroughputControllerTests PRIVATE ${GTEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME})
        target_link_libraries(ThroughputControllerTests ${GTEST_LIBRARIES} ${MOCKS})
        if(MSVC OR MSVC_IDE)
            target_link_libraries(ThroughputControllerTests ${PRIVACY}
                iphlpapi Shlwapi
                )
        endif()
    endif()
endif()
