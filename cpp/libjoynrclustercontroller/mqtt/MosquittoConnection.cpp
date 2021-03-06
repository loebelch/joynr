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

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/MessagingSettings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

MosquittoConnection::MosquittoConnection(const MessagingSettings& messagingSettings,
                                         const ClusterControllerSettings& ccSettings,
                                         const std::string& clientId)
        : mosquittopp(clientId.c_str(), false),
          messagingSettings(messagingSettings),
          host(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost()),
          port(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort()),
          channelId(),
          subscribeChannelMid(),
          topic(),
          additionalTopics(),
          additionalTopicsMutex(),
          isConnected(false),
          isRunning(false),
          isChannelIdRegistered(false),
          subscribedToChannelTopic(false),
          readyToSend(false),
          onMessageReceived(),
          onReadyToSendChangedMutex(),
          onReadyToSendChanged()
{
    JOYNR_LOG_INFO(logger(), "Init mosquitto connection using MQTT client ID: {}", clientId);
    mosqpp::lib_init();

    if (ccSettings.isMqttTlsEnabled()) {
        const std::string mqttCertificateAuthorityPemFilename =
                ccSettings.getMqttCertificateAuthorityPemFilename();
        const std::string mqttCertificateAuthorityCertificateFolderPath =
                ccSettings.getMqttCertificateAuthorityCertificateFolderPath();

        const char* mqttCertificateAuthorityPemFilename_cstr = nullptr;
        if (!mqttCertificateAuthorityPemFilename.empty()) {
            mqttCertificateAuthorityPemFilename_cstr = mqttCertificateAuthorityPemFilename.c_str();
        }

        const char* mqttCertificateAuthorityCertificateFolderPath_cstr = nullptr;
        if (!mqttCertificateAuthorityCertificateFolderPath.empty()) {
            mqttCertificateAuthorityCertificateFolderPath_cstr =
                    mqttCertificateAuthorityCertificateFolderPath.c_str();
        }

        int rc = tls_set(mqttCertificateAuthorityPemFilename_cstr,
                         mqttCertificateAuthorityCertificateFolderPath_cstr,
                         ccSettings.getMqttCertificatePemFilename().c_str(),
                         ccSettings.getMqttPrivateKeyPemFilename().c_str());

        if (rc != MOSQ_ERR_SUCCESS) {
            mosqpp::lib_cleanup();
            JOYNR_LOG_FATAL(logger(),
                            "fatal failure to initialize TLS connection - {}",
                            mosqpp::strerror(rc));
        }
    } else {
        JOYNR_LOG_DEBUG(logger(), "MQTT connection not encrypted");
    }
}

MosquittoConnection::~MosquittoConnection()
{
    stop();
    stopLoop(true);

    mosqpp::lib_cleanup();
}

void MosquittoConnection::on_disconnect(int rc)
{
    setReadyToSend(false);
    if (!isConnected) {
        // In case we didn't connect yet
        JOYNR_LOG_ERROR(logger(),
                        "Not yet connected to tcp://{}:{}, error: {}",
                        host,
                        port,
                        mosqpp::strerror(rc));
        return;
    }

    // There was indeed a disconnect...set connect to false
    isConnected = false;

    if (rc == 0) {
        JOYNR_LOG_DEBUG(logger(), "Disconnected from tcp://{}:{}", host, port);
        stopLoop();
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Unexpectedly disconnected from tcp://{}:{}, error: {}",
                        host,
                        port,
                        mosqpp::strerror(rc));
        reconnect();
    }
}

void MosquittoConnection::on_log(int level, const char* str)
{
    if (level == MOSQ_LOG_ERR) {
        JOYNR_LOG_ERROR(logger(), "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_WARNING) {
        JOYNR_LOG_WARN(logger(), "Mosquitto Log: {}", str);
    } else if (level == MOSQ_LOG_INFO) {
        JOYNR_LOG_INFO(logger(), "Mosquitto Log: {}", str);
    } else {
        // MOSQ_LOG_NOTICE || MOSQ_LOG_DEBUG || any other log level
        JOYNR_LOG_DEBUG(logger(), "Mosquitto Log: {}", str);
    }
}

std::uint16_t MosquittoConnection::getMqttQos() const
{
    return mqttQos;
}

std::string MosquittoConnection::getMqttPrio() const
{
    static const std::string value("low");
    return value;
}

bool MosquittoConnection::isMqttRetain() const
{
    return mqttRetain;
}

void MosquittoConnection::start()
{
    JOYNR_LOG_TRACE(
            logger(), "Start called with isRunning: {}, isConnected: {}", isRunning, isConnected);

    JOYNR_LOG_DEBUG(logger(), "Try to connect to tcp://{}:{}", host, port);
    connect_async(host.c_str(), port, messagingSettings.getMqttKeepAliveTimeSeconds().count());

    reconnect_delay_set(messagingSettings.getMqttReconnectDelayTimeSeconds().count(),
                        messagingSettings.getMqttReconnectMaxDelayTimeSeconds().count(),
                        messagingSettings.getMqttExponentialBackoffEnabled());

    startLoop();
}

void MosquittoConnection::startLoop()
{
    int rc = loop_start();
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_TRACE(logger(), "Mosquitto loop started");
        isRunning = true;
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Mosquitto loop start failed: error: {} ({})",
                        std::to_string(rc),
                        mosqpp::strerror(rc));
    }
}

void MosquittoConnection::stop()
{
    if (isConnected) {
        int rc = disconnect();

        if (rc == MOSQ_ERR_SUCCESS) {
            JOYNR_LOG_TRACE(logger(), "Mosquitto Connection disconnected");
        } else {
            JOYNR_LOG_ERROR(logger(),
                            "Mosquitto disconnect failed: error: {} ({})",
                            std::to_string(rc),
                            mosqpp::strerror(rc));
            stopLoop(true);
            mosqpp::lib_cleanup();
        }
    } else if (isRunning) {
        stopLoop(true);
        mosqpp::lib_cleanup();
    }
    setReadyToSend(false);
}

void MosquittoConnection::stopLoop(bool force)
{
    int rc = loop_stop(force);

    if (rc == MOSQ_ERR_SUCCESS) {
        isRunning = false;
        JOYNR_LOG_TRACE(logger(), "Mosquitto loop stopped");
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Mosquitto loop stop failed: error: {} ({})",
                        std::to_string(rc),
                        mosqpp::strerror(rc));
    }
}

void MosquittoConnection::on_connect(int rc)
{
    if (rc > 0) {
        JOYNR_LOG_ERROR(logger(),
                        "Mosquitto Connection Error: {} ({})",
                        std::to_string(rc),
                        mosqpp::strerror(rc));
    } else {
        JOYNR_LOG_DEBUG(logger(), "Mosquitto Connection established");
        isConnected = true;

        createSubscriptions();
    }
}

void MosquittoConnection::createSubscriptions()
{
    while (!isChannelIdRegistered && isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    try {
        subscribeToTopicInternal(topic, true);
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        for (const std::string& additionalTopic : additionalTopics) {
            subscribeToTopicInternal(additionalTopic);
        }
    } catch (const exceptions::JoynrRuntimeException& error) {
        JOYNR_LOG_ERROR(logger(), "Error subscribing to Mqtt topic, error: ", error.getMessage());
    }
}

void MosquittoConnection::subscribeToTopicInternal(const std::string& topic,
                                                   const bool isChannelTopic)
{
    int* mid = nullptr;
    if (isChannelTopic) {
        mid = &subscribeChannelMid;
    }
    int rc = subscribe(mid, topic.c_str(), getMqttQos());
    switch (rc) {
    case (MOSQ_ERR_SUCCESS):
        JOYNR_LOG_DEBUG(logger(), "Subscribed to {}", topic);
        break;
    case (MOSQ_ERR_NO_CONN):
        JOYNR_LOG_DEBUG(logger(),
                        "Subscription to {} failed: error: {} (not connected to a broker). "
                        "Subscription will be restored on connect.",
                        topic,
                        std::to_string(rc));
        break;
    default:
        // MOSQ_ERR_INVAL, MOSQ_ERR_NOMEM
        std::string errorMsg = "Subscription to " + topic + " failed: error: " +
                               std::to_string(rc) + " (" + mosqpp::strerror(rc) + ")";
        throw exceptions::JoynrRuntimeException(errorMsg);
    }
}

void MosquittoConnection::subscribeToTopic(const std::string& topic)
{
    if (!isChannelIdRegistered) {
        std::string errorMsg = "No channelId registered, cannot subscribe to topic " + topic;
        throw exceptions::JoynrRuntimeException(errorMsg);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        if (additionalTopics.find(topic) != additionalTopics.end()) {
            JOYNR_LOG_DEBUG(logger(), "Already subscribed to topic {}", topic);
            return;
        }

        subscribeToTopicInternal(topic);

        additionalTopics.insert(topic);
    }
}

void MosquittoConnection::unsubscribeFromTopic(const std::string& topic)
{
    if (isChannelIdRegistered) {
        std::lock_guard<std::recursive_mutex> lock(additionalTopicsMutex);
        if (additionalTopics.find(topic) == additionalTopics.end()) {
            JOYNR_LOG_DEBUG(logger(), "Unsubscribe called for non existing topic {}", topic);
            return;
        }
        additionalTopics.erase(topic);
        if (isConnected && isRunning) {
            int rc = unsubscribe(nullptr, topic.c_str());
            if (rc == MOSQ_ERR_SUCCESS) {
                JOYNR_LOG_DEBUG(logger(), "Unsubscribed from {}", topic);
            } else {
                // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN
                JOYNR_LOG_ERROR(logger(),
                                "Unsubscribe from {} failed: error: {} ({})",
                                topic,
                                std::to_string(rc),
                                mosqpp::strerror(rc));
            }
        }
    }
}

void MosquittoConnection::publishMessage(
        const std::string& topic,
        const int qosLevel,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
        uint32_t payloadlen = 0,
        const void* payload = nullptr)
{
    JOYNR_LOG_DEBUG(logger(), "Publish message of length {} to {}", payloadlen, topic);

    int mid;
    int rc = publish(&mid, topic.c_str(), payloadlen, payload, qosLevel, isMqttRetain());
    if (!(rc == MOSQ_ERR_SUCCESS)) {
        if (rc == MOSQ_ERR_INVAL || rc == MOSQ_ERR_PAYLOAD_SIZE) {
            onFailure(exceptions::JoynrMessageNotSentException(
                    "message could not be sent: mid (mqtt message id): " + std::to_string(mid) +
                    ", error: " + std::to_string(rc) + " (" + mosqpp::strerror(rc) + ")"));
        }
        // MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN || MOSQ_ERR_PROTOCOL ||| unexpected errors
        onFailure(exceptions::JoynrDelayMessageException(
                "error sending message: mid (mqtt message id): " + std::to_string(mid) +
                ", error: " + std::to_string(rc) + " (" + mosqpp::strerror(rc) + ")"));
        return;
    }
    JOYNR_LOG_TRACE(logger(), "published message with mqtt message id {}", std::to_string(mid));
}

void MosquittoConnection::registerChannelId(const std::string& channelId)
{
    this->channelId = channelId;
    topic = channelId + "/" + getMqttPrio() + "/" + "#";
    isChannelIdRegistered = true;
}

void MosquittoConnection::registerReceiveCallback(
        std::function<void(smrf::ByteVector&&)> onMessageReceived)
{
    this->onMessageReceived = onMessageReceived;
}

void MosquittoConnection::registerReadyToSendChangedCallback(
        std::function<void(bool)> onReadyToSendChanged)
{
    std::lock_guard<std::mutex> lock(onReadyToSendChangedMutex);
    this->onReadyToSendChanged = std::move(onReadyToSendChanged);
}

bool MosquittoConnection::isSubscribedToChannelTopic() const
{
    return subscribedToChannelTopic;
}

bool MosquittoConnection::isReadyToSend() const
{
    return readyToSend;
}

void MosquittoConnection::on_subscribe(int mid, int qos_count, const int* granted_qos)
{
    JOYNR_LOG_DEBUG(logger(), "Subscribed (mid: {} with granted QOS {}", mid, granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(logger(), "QOS: {} granted {}", i, granted_qos[i]);
    }

    if (mid == subscribeChannelMid) {
        subscribedToChannelTopic = true;
        setReadyToSend(isConnected);
    }
}

void MosquittoConnection::on_message(const mosquitto_message* message)
{
    if (!onMessageReceived) {
        JOYNR_LOG_ERROR(logger(),
                        "Discarding received message, since onMessageReceived callback is empty.");
        return;
    }

    if (!message || message->payloadlen <= 0) {
        JOYNR_LOG_ERROR(
                logger(),
                "Discarding received message: invalid message or non-positive payload's length.");
        return;
    }

    std::uint8_t* data = static_cast<std::uint8_t*>(message->payload);

    // convert address of data into integral type
    std::uintptr_t integralAddress = reinterpret_cast<std::uintptr_t>(data);
    const bool overflow =
            joynr::util::isAdditionOnPointerSafe(integralAddress, message->payloadlen);
    if (overflow) {
        JOYNR_LOG_ERROR(logger(), "Discarding received message, since there is an overflow.");
        return;
    }

    smrf::ByteVector rawMessage(data, data + message->payloadlen);
    onMessageReceived(std::move(rawMessage));
}

void MosquittoConnection::on_publish(int mid)
{
    JOYNR_LOG_TRACE(logger(), "published message with mid {}", std::to_string(mid));
}

void MosquittoConnection::setReadyToSend(bool readyToSend)
{
    if (this->readyToSend != readyToSend) {
        this->readyToSend = readyToSend;

        std::lock_guard<std::mutex> lock(onReadyToSendChangedMutex);
        if (onReadyToSendChanged) {
            onReadyToSendChanged(readyToSend);
        }
    }
}

} // namespace joynr
