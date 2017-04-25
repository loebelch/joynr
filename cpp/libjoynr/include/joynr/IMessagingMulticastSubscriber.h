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
#ifndef IMESSAGINGMULTICASTSUBSCRIBER_H
#define IMESSAGINGMULTICASTSUBSCRIBER_H

#include <string>

#include "joynr/IMessaging.h"
#include "joynr/JoynrExport.h"

namespace joynr
{

class JOYNR_EXPORT IMessagingMulticastSubscriber : public IMessaging
{
public:
    ~IMessagingMulticastSubscriber() override = default;
    virtual void registerMulticastSubscription(const std::string& multicastId) = 0;
    virtual void unregisterMulticastSubscription(const std::string& multicastId) = 0;
};

} // namespace joynr
#endif // IMESSAGINGMULTICASTSUBSCRIBER_H