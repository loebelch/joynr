/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "gtest/gtest.h"
#include "joynr/JoynrMessageFactory.h"
#include "utils/TestQString.h"
#include "utils/QThreadSleep.h"
#include "joynr/Request.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "common/rpc/RpcMetaTypes.h"
#include "joynr/Reply.h"
#include "joynr/MessagingQos.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/QtOnChangeSubscriptionQos.h"
#include "joynr/joynrlogging.h"
#include "joynr/DispatcherUtils.h"
#include <chrono>
#include <stdint.h>

using namespace joynr;
using namespace std::chrono;

class JoynrMessageFactoryTest : public ::testing::Test {
public:
    JoynrMessageFactoryTest()
        : logger(joynr_logging::Logging::getInstance()->getLogger(QString("TEST"), QString("JoynrMessageFactoryTest"))),
          messageFactory(),
          senderID(),
          receiverID(),
          requestReplyID(),
          qos(),
          request(),
          reply(),
          subscriptionPublication()
    {
        registerRpcMetaTypes();
    }

    void SetUp(){
        senderID = QString("senderId");
        receiverID = QString("receiverID");
        requestReplyID = "requestReplyID";
        qos = MessagingQos(456000);
        request.setMethodName("methodName");
        request.setRequestReplyId(requestReplyID);
        ;
        request.addParam(Variant::make<int>(42), "java.lang.Integer");
        request.addParam(Variant::make<std::string>("value"), "java.lang.String");
        reply.setRequestReplyId(requestReplyID);
        std::vector<Variant> response;
        response.push_back(Variant::make<std::string>("response"));
        reply.setResponse(response);

        std::string subscriptionId("subscriptionTestId");
        subscriptionPublication.setSubscriptionId(subscriptionId);
        response.clear();
        response.push_back(Variant::make<std::string>("publication"));
        subscriptionPublication.setResponse(response);
    }
    void TearDown(){

    }
    void checkHeaderCreatorFromTo(const JoynrMessage& joynrMessage){
        EXPECT_TRUE(joynrMessage.containsHeaderCreatorUserId());
        EXPECT_STREQ(senderID.toStdString().c_str(), joynrMessage.getHeaderFrom().c_str());
        EXPECT_STREQ(receiverID.toStdString().c_str(), joynrMessage.getHeaderTo().c_str());
    }

    void checkRequest(const JoynrMessage& joynrMessage){
        //TODO create expected string from params and methodName
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.Request\","
                    "\"methodName\":\"methodName\","
                    "\"paramDatatypes\":[\"java.lang.Integer\",\"java.lang.String\"],"
                    "\"params\":[42,\"value\"],"
                    "\"requestReplyId\":\"%1\"}"
        );
        expectedPayload = expectedPayload.arg(TypeUtil::toQt(request.getRequestReplyId()));
        EXPECT_EQ(expectedPayload, QString::fromStdString(joynrMessage.getPayload()));
    }

    void checkReply(const JoynrMessage& joynrMessage){
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.Reply\","
                    "\"requestReplyId\":\"%1\","
                    "\"response\":[\"response\"]}"
        );
        expectedPayload = expectedPayload.arg(QString::fromStdString(reply.getRequestReplyId()));
        EXPECT_EQ(expectedPayload, QString::fromStdString(joynrMessage.getPayload()));
    }

    void checkSubscriptionPublication(const JoynrMessage& joynrMessage){
        QString expectedPayload = QString(
                    "{\"_typeName\":\"joynr.SubscriptionPublication\","
                    "\"response\":[\"publication\"],"
                    "\"subscriptionId\":\"%1\"}"
        );
        expectedPayload = expectedPayload.arg(QString::fromStdString(subscriptionPublication.getSubscriptionId()));
        EXPECT_EQ(expectedPayload, QString::fromStdString(joynrMessage.getPayload()));
    }

protected:
    joynr_logging::Logger* logger;
    JoynrMessageFactory messageFactory;
    QString senderID;
    QString receiverID;
    std::string requestReplyID;
    MessagingQos qos;
    Request request;
    Reply reply;
    SubscriptionPublication subscriptionPublication;
};

TEST_F(JoynrMessageFactoryTest, createRequest_withContentType) {
    JoynrMessage joynrMessage = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    checkHeaderCreatorFromTo(joynrMessage);
}

TEST_F(JoynrMessageFactoryTest, createRequest){
    JoynrMessage joynrMessage = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    //warning if prepareRequest needs to long this assert will fail as it compares absolute timestamps
    JoynrTimePoint now = time_point_cast<milliseconds>(system_clock::now());
    JoynrTimePoint expectedExpiryDate = now + milliseconds(qos.getTtl());
    JoynrTimePoint expiryDate = joynrMessage.getHeaderExpiryDate();
    EXPECT_NEAR(expectedExpiryDate.time_since_epoch().count(), expiryDate.time_since_epoch().count(), 100.);
    LOG_DEBUG(logger,
              QString("expiryDate: %1 [%2]")
              .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(expiryDate)))
              .arg(duration_cast<milliseconds>(expiryDate.time_since_epoch()).count()));
    LOG_DEBUG(logger,
              QString("expectedExpiryDate: %1 [%2]")
              .arg(QString::fromStdString(DispatcherUtils::convertAbsoluteTimeToTtlString(expectedExpiryDate)))
              .arg(duration_cast<milliseconds>(expectedExpiryDate.time_since_epoch()).count()));

    checkHeaderCreatorFromTo(joynrMessage);
    checkRequest(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST.c_str(), joynrMessage.getType().c_str());
}

TEST_F(JoynrMessageFactoryTest, createReply){
    JoynrMessage joynrMessage = messageFactory.createReply(
                senderID,
                receiverID,
                qos,
                reply
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkReply(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_REPLY.c_str(), joynrMessage.getType().c_str());
}

TEST_F(JoynrMessageFactoryTest, createOneWay){
    JoynrMessage joynrMessage = messageFactory.createOneWay(
                senderID,
                receiverID,
                qos,
                reply
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkReply(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY.c_str(), joynrMessage.getType().c_str());
}

//TEST_F(JoynrMessageFactoryTest, createSubscriptionReply){
//    QString subscriptionId("subscriptionTestId");
//    JoynrMessage joynrMessage = JoynrMessageFactory::prepareSubscriptionReply(senderID, receiverID, payload, subscriptionId);
//    checkHeaderCreatorFromTo(joynrMessage);
//    checkPayload(joynrMessage);
//    EXPECT_QSTREQ(subscriptionId, joynrMessage.getHeader<QString>(JoynrMessage::HEADER_NAME_SUBSCRIPTION_ID));
//    EXPECT_QSTREQ(JoynrMessage::MESSAGE_TYPE_SUBSCRIPTION_REPLY, joynrMessage.getType());
//}

TEST_F(JoynrMessageFactoryTest, createPublication){
    JoynrMessage joynrMessage = messageFactory.createSubscriptionPublication(
                senderID,
                receiverID,
                qos,
                subscriptionPublication
    );
    checkHeaderCreatorFromTo(joynrMessage);
    checkSubscriptionPublication(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION.c_str(), joynrMessage.getType().c_str());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionRequest){
    auto subscriptionQos = std::shared_ptr<QtSubscriptionQos>(new QtOnChangeSubscriptionQos());
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId("subscriptionId");
    subscriptionRequest.setSubscribeToName("attributeName");
    subscriptionRequest.setQos(subscriptionQos);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionRequest(
                senderID,
                receiverID,
                qos,
                subscriptionRequest
    );
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST.c_str(), joynrMessage.getType().c_str());
}

TEST_F(JoynrMessageFactoryTest, createSubscriptionStop){
    std::string subscriptionId("TEST-SubscriptionId");
    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionId);
    JoynrMessage joynrMessage = messageFactory.createSubscriptionStop(
                senderID,
                receiverID,
                qos,
                subscriptionStop
    );
    checkHeaderCreatorFromTo(joynrMessage);
    EXPECT_STREQ(JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP.c_str(), joynrMessage.getType().c_str());
}

TEST_F(JoynrMessageFactoryTest, testRequestContentType){
    Request request;
    std::vector<Variant> params;
    params.push_back(Variant::make<std::string>("test"));
    request.setMethodName("methodName");
    request.setParams(params);

    JoynrMessage message = messageFactory.createRequest(
                senderID,
                receiverID,
                qos,
                request
    );
    EXPECT_STREQ(JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON.c_str(), message.getHeaderContentType().c_str());
}
