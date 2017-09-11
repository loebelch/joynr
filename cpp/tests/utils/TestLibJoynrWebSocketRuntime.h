/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTLIBJOYNRWEBSOCKETRUNTIME_H
#define TESTLIBJOYNRWEBSOCKETRUNTIME_H

#include <memory>
#include <chrono>

#include <gtest/gtest.h>

#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"
#include "joynr/Settings.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/IKeychain.h"

namespace joynr
{

class TestLibJoynrWebSocketRuntime : public LibJoynrWebSocketRuntime
{
public:

    TestLibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings, std::shared_ptr<IKeychain> keyChain) : LibJoynrWebSocketRuntime(std::move(settings), std::move(keyChain))
    {
    }

    bool connect(std::chrono::milliseconds timeoutMs)
    {
        Semaphore semaphore;

        LibJoynrWebSocketRuntime::connect([&semaphore]()
        {
            semaphore.notify();
        }, [](const exceptions::JoynrRuntimeException&){ FAIL(); });

        return semaphore.waitFor(timeoutMs);
    }
};

} // namespace joynr

#endif // TESTLIBJOYNRWEBSOCKETRUNTIME_H
