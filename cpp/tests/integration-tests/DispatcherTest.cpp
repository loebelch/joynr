/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include <string>
#include <vector>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/MessageSender.h"
#include "joynr/MulticastPublication.h"
#include "joynr/Dispatcher.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/Semaphore.h"
#include "joynr/tests/Itest.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockCallback.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockSubscriptionManager.h"
#include "tests/mock/MockSubscriptionCallback.h"
#include "tests/mock/MockTestRequestCaller.h"
#include "tests/mock/MockReplyCaller.h"
#include "tests/mock/MockSubscriptionListener.h"

using namespace ::testing;
using namespace joynr;

class DispatcherTest : public ::testing::Test {
public:
    DispatcherTest() :
        singleThreadIOService(),
        mockMessageRouter(new MockMessageRouter(singleThreadIOService.getIOService())),
        mockCallback(new MockCallbackWithJoynrException<types::Localisation::GpsLocation>()),
        mockRequestCaller(new MockTestRequestCaller()),
        mockReplyCaller(new MockReplyCaller<types::Localisation::GpsLocation>(
                [this] (const joynr::types::Localisation::GpsLocation& location) {
                    mockCallback->onSuccess(location);
                },
                [] (const std::shared_ptr<exceptions::JoynrException>&) {
                })),
        mockSubscriptionListener(new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        qos(2000),
        providerParticipantId("TEST-providerParticipantId"),
        proxyParticipantId("TEST-proxyParticipantId"),
        requestReplyId("TEST-requestReplyId"),
        messageFactory(),
        messageSender(std::make_shared<MessageSender>(mockMessageRouter, nullptr)),
        dispatcher(messageSender, singleThreadIOService.getIOService()),
        callContext(),
        getLocationCalledSemaphore(0),
        isLocalMessage(true)
    {
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
        singleThreadIOService.start();
    }

    void invokeOnSuccessWithGpsLocation(
            std::function<void(const joynr::types::Localisation::GpsLocation& location)> onSuccess,
            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        std::ignore = onError;
        onSuccess(gpsLocation1);
    }

    void invokeLocationAndSaveCallContext(
            std::function<void(const joynr::types::Localisation::GpsLocation& location)> onSuccess,
            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        std::ignore = onError;
        callContext = joynr::CallContextStorage::get();
        onSuccess(types::Localisation::GpsLocation());
        getLocationCalledSemaphore.notify();
    }

protected:
    ADD_LOGGER(DispatcherTest)
    SingleThreadedIOService singleThreadIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockCallbackWithJoynrException<types::Localisation::GpsLocation> > mockCallback;

    std::shared_ptr<MockTestRequestCaller> mockRequestCaller;
    std::shared_ptr<MockReplyCaller<types::Localisation::GpsLocation> > mockReplyCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockSubscriptionListener;

    types::Localisation::GpsLocation gpsLocation1;

    // create test data
    MessagingQos qos;
    std::string providerParticipantId;
    std::string proxyParticipantId;
    std::string requestReplyId;

    MutableMessageFactory messageFactory;
    std::shared_ptr<MessageSender> messageSender;
    Dispatcher dispatcher;
    joynr::CallContext callContext;
    joynr::Semaphore getLocationCalledSemaphore;
    const bool isLocalMessage;
};

// from JoynrDispatcher.receive(Request) to IRequestCaller.operation(params)
// this test goes a step further and makes sure that the response is visible in Messaging
TEST_F(DispatcherTest, receive_interpreteRequestAndCallOperation) {

    // Expect the mock object MockTestRequestCaller to be called.
    // The OUT param Gpslocation is set with gpsLocation1
    EXPECT_CALL(
                *mockRequestCaller,
                getLocationMock(
                    A<std::function<void(const joynr::types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>>()
                )
    ).WillOnce(Invoke(this, &DispatcherTest::invokeOnSuccessWithGpsLocation));

    qos.setTtl(1000);
    // build request for location from mock Gps Provider
    Request request;
    request.setRequestReplyId(requestReplyId);
    request.setMethodName("getLocation");
    request.setParams();
    request.setParamDatatypes(std::vector<std::string>());


    MutableMessage mutableMessage = messageFactory.createRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                request,
                isLocalMessage
    );

    // construct the result we expect in messaging.transmit. The JoynrMessage
    // contains a serialized version of the response with the gps location.
    Reply reply;
    reply.setResponse(gpsLocation1);
    reply.setRequestReplyId(requestReplyId);
    MutableMessage expectedReply = messageFactory.createReply(
                proxyParticipantId,
                providerParticipantId,
                qos,
                {},
                reply
    );

    JOYNR_LOG_DEBUG(logger(), "expectedReply.payload()={}",expectedReply.getPayload());

    // setup MockMessaging to expect the response
    EXPECT_CALL(
                *mockMessageRouter,
                route(
                    AllOf(
                        MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_REPLY()),
                        ImmutableMessageHasPayload(expectedReply.getPayload())
                    ),
                    _
                )
    ).WillOnce(ReleaseSemaphore(&getLocationCalledSemaphore));

    // test code: send the request through the dispatcher.
    // This should cause our mock messaging to receive a reply from the mock provider
    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);

    dispatcher.receive(mutableMessage.getImmutableMessage());
    EXPECT_TRUE(getLocationCalledSemaphore.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(DispatcherTest, receive_customHeadersCopied) {
    const std::string customHeaderKey = "custom-header-key";
    const std::string customHeaderValue = "custom-value";

    EXPECT_CALL(
                *mockRequestCaller,
                getLocationMock(
                    A<std::function<void(const joynr::types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>>()
                )
    ).WillOnce(Invoke(this, &DispatcherTest::invokeOnSuccessWithGpsLocation));

    Request request;
    request.setRequestReplyId(requestReplyId);
    request.setMethodName("getLocation");

    MutableMessage mutableMessage = messageFactory.createRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                request,
                isLocalMessage
    );

    mutableMessage.setCustomHeader(customHeaderKey, customHeaderValue);

    std::unordered_map<std::string, std::string> prefixedCustomHeaders {{customHeaderKey, customHeaderValue}};

    EXPECT_CALL(
                *mockMessageRouter,
                route(
                    AllOf(
                        MessageHasType(joynr::Message::VALUE_MESSAGE_TYPE_REPLY()),
                        ImmutableMessageHasPrefixedCustomHeaders(prefixedCustomHeaders)
                    ),
                    _
                )
    ).WillOnce(ReleaseSemaphore(&getLocationCalledSemaphore));

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    dispatcher.receive(mutableMessage.getImmutableMessage());

    EXPECT_TRUE(getLocationCalledSemaphore.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(DispatcherTest, receive_interpreteReplyAndCallReplyCaller) {
    joynr::Semaphore semaphore(0);

    // Expect the mock callback's onSuccess method to be called with the reply (a gps location)
    EXPECT_CALL(*mockCallback, onSuccess(Eq(gpsLocation1))).WillOnce(ReleaseSemaphore(&semaphore));

    // getType is used by the ReplyInterpreterFactory to create an interpreter for the reply
    // so this has to match with the type being passed to the dispatcher in the reply
    ON_CALL(*mockReplyCaller, getType()).WillByDefault(Return(std::string("types::Localisation::GpsLocation")));

    //construct a reply containing a GpsLocation
    Reply reply;
    reply.setRequestReplyId(requestReplyId);
    reply.setResponse(gpsLocation1);

    MutableMessage mutableMessage = messageFactory.createReply(
                proxyParticipantId,
                providerParticipantId,
                qos,
                {},
                reply
    );

    // test code: send the reply through the dispatcher.
    // This should cause our reply caller to be called
    dispatcher.addReplyCaller(requestReplyId, mockReplyCaller, qos);
    dispatcher.receive(mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(5000)));
}

TEST_F(DispatcherTest, receive_interpreteSubscriptionReplyAndCallSubscriptionCallback) {
    joynr::Semaphore semaphore(0);
    std::string subscriptionId = "testSubscriptionId";

    //construct a subscription reply
    SubscriptionReply reply;
    reply.setSubscriptionId(subscriptionId);

    MutableMessage mutableMessage = messageFactory.createSubscriptionReply(
                proxyParticipantId,
                providerParticipantId,
                qos,
                reply
    );

    auto mockSubscriptionManager = std::make_shared<MockSubscriptionManager>(singleThreadIOService.getIOService(), mockMessageRouter);
    auto mockSubscriptionCallback = std::make_shared<MockSubscriptionCallback>();
    EXPECT_CALL(*mockSubscriptionManager, getSubscriptionCallback(Eq(subscriptionId))).WillOnce(Return(mockSubscriptionCallback));

    EXPECT_CALL(*mockSubscriptionCallback, execute(Eq(reply))).WillOnce(ReleaseSemaphore(&semaphore));

    // test code: send the subscription reply through the dispatcher.
    // This should cause our subscription callback to be called
    dispatcher.registerSubscriptionManager(mockSubscriptionManager);
    dispatcher.receive(mutableMessage.getImmutableMessage());

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(5000)));
    dispatcher.registerSubscriptionManager(nullptr);
}

TEST_F(DispatcherTest, receiveMulticastPublication_callSubscriptionCallback) {
    auto mockSubscriptionManager = std::make_shared<MockSubscriptionManager>(
                singleThreadIOService.getIOService(), mockMessageRouter);
    dispatcher.registerSubscriptionManager(mockSubscriptionManager);

    const std::string senderParticipantId("senderParticipantId");
    const std::string multicastId = joynr::util::createMulticastId(
        senderParticipantId,
        "multicastName",
        { "partition0", "partition1"}
    );

    joynr::MulticastPublication payload;
    payload.setMulticastId(multicastId);

    MutableMessage message = messageFactory.createMulticastPublication(
                senderParticipantId, qos, payload);

    auto mockSubscriptionCallback = std::make_shared<MockSubscriptionCallback>();

    EXPECT_CALL(*mockSubscriptionManager, getMulticastSubscriptionCallback(multicastId))
        .Times(1)
        .WillOnce(Return(mockSubscriptionCallback));
    EXPECT_CALL(*mockSubscriptionCallback, executePublication(_))
        .WillOnce(ReleaseSemaphore(&getLocationCalledSemaphore));

    dispatcher.receive(message.getImmutableMessage());

    EXPECT_TRUE(getLocationCalledSemaphore.waitFor(std::chrono::milliseconds(5000)));
    dispatcher.registerSubscriptionManager(nullptr);
}


TEST_F(DispatcherTest, receive_setCallContext) {
    const std::string expectedPrincipal("creatorUserId");

    Request request;
    request.setRequestReplyId(requestReplyId);
    request.setMethodName("getLocation");
    request.setParams();
    request.setParamDatatypes(std::vector<std::string>());

    MutableMessage mutableMessage = messageFactory.createRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                request,
                isLocalMessage
    );
    std::shared_ptr<ImmutableMessage> immutableMessage = mutableMessage.getImmutableMessage();
    immutableMessage->setCreator(expectedPrincipal);
    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);

    EXPECT_CALL(
                *mockRequestCaller,
                getLocationMock(
                    A<std::function<void(const joynr::types::Localisation::GpsLocation&)>>(),
                    A<std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>>()
                )
    ).WillOnce(Invoke(this, &DispatcherTest::invokeLocationAndSaveCallContext));

    dispatcher.receive(immutableMessage);
    EXPECT_TRUE(getLocationCalledSemaphore.waitFor(std::chrono::milliseconds(5000)));

    EXPECT_EQ(expectedPrincipal, callContext.getPrincipal());
}
