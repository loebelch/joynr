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
#ifndef TESTS_MOCK_MOCKJOYNRRUNTIME_H
#define TESTS_MOCK_MOCKJOYNRRUNTIME_H

#include <gmock/gmock.h>

#include "joynr/JoynrRuntime.h"

class MockJoynrRuntime : public joynr::JoynrRuntime
{
public:
    MockJoynrRuntime(joynr::Settings& settings) : joynr::JoynrRuntime(settings) {}
    MockJoynrRuntime(std::unique_ptr<joynr::Settings> settings) : joynr::JoynrRuntime(*settings) {}
    MOCK_METHOD0(getMessageRouter, std::shared_ptr<joynr::IMessageRouter>());
};

#endif // TESTS_MOCK_MOCKJOYNRRUNTIME_H
