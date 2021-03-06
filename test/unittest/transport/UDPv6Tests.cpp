// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fastrtps/transport/UDPv6Transport.h>
#include <gtest/gtest.h>
#include <thread>
#include <memory>
#include <fastrtps/log/Log.h>
#include <asio.hpp>


using namespace eprosima::fastrtps::rtps;
using namespace eprosima::fastrtps;

#ifndef __APPLE__
const uint32_t ReceiveBufferCapacity = 65536;
#endif

class UDPv6Tests: public ::testing::Test
{
    public:
        UDPv6Tests()
        {
            HELPER_SetDescriptorDefaults();
        }

        ~UDPv6Tests()
        {
            Log::KillThread();
        }

        void HELPER_SetDescriptorDefaults();

        UDPv6TransportDescriptor descriptor;
        std::unique_ptr<std::thread> senderThread;
        std::unique_ptr<std::thread> receiverThread;
};

TEST_F(UDPv6Tests, conversion_to_ip6_string)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;
    ASSERT_EQ("0:0:0:0:0:0:0:0", locator.to_IP6_string());

    locator.address[0] = 0xff;
    ASSERT_EQ("ff00:0:0:0:0:0:0:0", locator.to_IP6_string());

    locator.address[1] = 0xaa;
    ASSERT_EQ("ffaa:0:0:0:0:0:0:0", locator.to_IP6_string());

    locator.address[2] = 0x0a;
    ASSERT_EQ("ffaa:a00:0:0:0:0:0:0", locator.to_IP6_string());

    locator.address[5] = 0x0c;
    ASSERT_EQ("ffaa:a00:c:0:0:0:0:0", locator.to_IP6_string());
}

TEST_F(UDPv6Tests, setting_ip6_values_on_locators)
{
    Locator_t locator;
    locator.kind = LOCATOR_KIND_UDPv6;

    locator.set_IP6_address(0xffff,0xa, 0xaba, 0, 0, 0, 0, 0);
    ASSERT_EQ("ffff:a:aba:0:0:0:0:0", locator.to_IP6_string());
}

TEST_F(UDPv6Tests, locators_with_kind_2_supported)
{
    // Given
    UDPv6Transport transportUnderTest(descriptor);
    transportUnderTest.init();

    Locator_t supportedLocator;
    supportedLocator.kind = LOCATOR_KIND_UDPv6;
    Locator_t unsupportedLocator;
    unsupportedLocator.kind = LOCATOR_KIND_UDPv4;

    // Then
    ASSERT_TRUE(transportUnderTest.IsLocatorSupported(supportedLocator));
    ASSERT_FALSE(transportUnderTest.IsLocatorSupported(unsupportedLocator));
}

TEST_F(UDPv6Tests, opening_and_closing_output_channel)
{
    // Given
    UDPv6Transport transportUnderTest(descriptor);
    transportUnderTest.init();

    Locator_t genericOutputChannelLocator;
    genericOutputChannelLocator.kind = LOCATOR_KIND_UDPv6;
    genericOutputChannelLocator.port = 7400; // arbitrary

    // Then
    ASSERT_FALSE (transportUnderTest.IsOutputChannelOpen(genericOutputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.OpenOutputChannel(genericOutputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.IsOutputChannelOpen(genericOutputChannelLocator));
    ASSERT_TRUE  (transportUnderTest.CloseOutputChannel(genericOutputChannelLocator));
    ASSERT_FALSE (transportUnderTest.IsOutputChannelOpen(genericOutputChannelLocator));
    ASSERT_FALSE (transportUnderTest.CloseOutputChannel(genericOutputChannelLocator));
}

#ifndef __APPLE__
TEST_F(UDPv6Tests, opening_and_closing_input_channel)
{
    // Given
    UDPv6Transport transportUnderTest(descriptor);
    transportUnderTest.init();

    Locator_t multicastFilterLocator;
    multicastFilterLocator.kind = LOCATOR_KIND_UDPv6;
    multicastFilterLocator.port = 7410; // arbitrary
    multicastFilterLocator.set_IP6_address(0xff31, 0, 0, 0, 0, 0, 0x8000, 0x1234);

    // Then
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(multicastFilterLocator));
    ASSERT_TRUE  (transportUnderTest.OpenInputChannel(multicastFilterLocator));
    ASSERT_TRUE  (transportUnderTest.IsInputChannelOpen(multicastFilterLocator));
    ASSERT_TRUE  (transportUnderTest.CloseInputChannel(multicastFilterLocator));
    ASSERT_FALSE (transportUnderTest.IsInputChannelOpen(multicastFilterLocator));
    ASSERT_FALSE (transportUnderTest.CloseInputChannel(multicastFilterLocator));
}

TEST_F(UDPv6Tests, send_and_receive_between_ports)
{
    UDPv6Transport transportUnderTest(descriptor);
    transportUnderTest.init();

    Locator_t multicastLocator;
    multicastLocator.port = 7410;
    multicastLocator.kind = LOCATOR_KIND_UDPv6;
    multicastLocator.set_IP6_address(0xff31, 0, 0, 0, 0, 0, 0, 0);

    Locator_t outputChannelLocator;
    outputChannelLocator.port = 7400;
    outputChannelLocator.kind = LOCATOR_KIND_UDPv6;
    ASSERT_TRUE(transportUnderTest.OpenOutputChannel(outputChannelLocator)); // Includes loopback
    ASSERT_TRUE(transportUnderTest.OpenInputChannel(multicastLocator));
    octet message[5] = { 'H','e','l','l','o' };

    auto sendThreadFunction = [&]()
    {
        Locator_t destinationLocator;
        destinationLocator.port = 7410;
        destinationLocator.kind = LOCATOR_KIND_UDPv6;
        EXPECT_TRUE(transportUnderTest.Send(message, 5, outputChannelLocator, multicastLocator));
    };

    auto receiveThreadFunction = [&]()
    {
        octet receiveBuffer[ReceiveBufferCapacity];
        uint32_t receiveBufferSize;
        Locator_t remoteLocatorToReceive;
        EXPECT_TRUE(transportUnderTest.Receive(receiveBuffer, ReceiveBufferCapacity, receiveBufferSize, multicastLocator, remoteLocatorToReceive));
        EXPECT_EQ(memcmp(message,receiveBuffer,5), 0);
    };

    receiverThread.reset(new std::thread(receiveThreadFunction));
    senderThread.reset(new std::thread(sendThreadFunction));
    senderThread->join();
    receiverThread->join();
}

/*
TEST_F(UDPv6Tests, send_to_loopback)
{
    UDPv6Transport transportUnderTest(descriptor);
    transportUnderTest.init();

    Locator_t multicastLocator;
    multicastLocator.port = 7410;
    multicastLocator.kind = LOCATOR_KIND_UDPv6;
    multicastLocator.set_IP6_address(0xff31, 0, 0, 0, 0, 0, 0, 0);

    Locator_t outputChannelLocator;
    outputChannelLocator.port = 7400;
    outputChannelLocator.kind = LOCATOR_KIND_UDPv6;
    outputChannelLocator.set_IP6_address(0,0,0,0,0,0,0,1); // Loopback
    ASSERT_TRUE(transportUnderTest.OpenOutputChannel(outputChannelLocator));
    ASSERT_TRUE(transportUnderTest.OpenInputChannel(multicastLocator));
    octet message[5] = { 'H','e','l','l','o' };

    auto sendThreadFunction = [&]()
    {
        Locator_t destinationLocator;
        destinationLocator.port = 7410;
        destinationLocator.kind = LOCATOR_KIND_UDPv6;
        EXPECT_TRUE(transportUnderTest.Send(message, 5, outputChannelLocator, multicastLocator));
    };

    auto receiveThreadFunction = [&]()
    {
        octet receiveBuffer[ReceiveBufferCapacity];
        uint32_t receiveBufferSize;

        Locator_t remoteLocatorToReceive;
        EXPECT_TRUE(transportUnderTest.Receive(receiveBuffer, ReceiveBufferCapacity, receiveBufferSize, multicastLocator, remoteLocatorToReceive));
        EXPECT_EQ(memcmp(message,receiveBuffer,5), 0);
    };

    receiverThread.reset(new std::thread(receiveThreadFunction));
    senderThread.reset(new std::thread(sendThreadFunction));
    senderThread->join();
    receiverThread->join();
}
*/
#endif

void UDPv6Tests::HELPER_SetDescriptorDefaults()
{
    descriptor.maxMessageSize = 5;
    descriptor.sendBufferSize = 5;
    descriptor.receiveBufferSize = 5;
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
