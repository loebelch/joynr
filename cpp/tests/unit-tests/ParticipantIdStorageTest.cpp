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

#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#include "joynr/ParticipantIdStorage.h"

using namespace joynr;

static const std::string storageFile("test-participantIdStorageTest.persist");

class ParticipantIdStorageTest : public ::testing::Test {
public:
    ParticipantIdStorageTest(){}

    void SetUp()
    {
        std::remove(storageFile.c_str());
    }
};

// Test that the default participant id is used when no provider exists"
TEST_F(ParticipantIdStorageTest, defaultProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);

    std::string participantId = store.getProviderParticipantId("domain.myDomain",
                                                           "interface.mytest",
                                                           "defaultParticipantId");
    ASSERT_EQ(std::string("defaultParticipantId"), participantId);
}

// Test that a participant id is created when no provider exists and
// no default value is given
TEST_F(ParticipantIdStorageTest, newProviderParticipantId)
{
    ParticipantIdStorage store(storageFile);
    std::string participantId = store.getProviderParticipantId("domain.myDomain",
                                                           "interface.mytest",
                                                           std::string());
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 32);

    // also check get function without default value
    participantId = store.getProviderParticipantId("domain.myDomain",
                                                   "interface.mytest");
    // Check that the id is long enough to be a UUID
    ASSERT_TRUE(participantId.size() > 32);
}

// Test that a persisted participant id is used
TEST_F(ParticipantIdStorageTest, persistedProviderParticipantId)
{
    std::string expectedParticipantId;
    {
        ParticipantIdStorage store(storageFile);
        expectedParticipantId = store.getProviderParticipantId("domain.myDomain",
                                                               "interface.mytest");
        store.setProviderParticipantId("domain.myDomain", "interface.mytest", expectedParticipantId);
    }

    // create a new store
    ParticipantIdStorage store(storageFile);

    // Check that the setting was persisted
    std::string participantId = store.getProviderParticipantId("domain.myDomain",
                                                               "interface.mytest");

    ASSERT_EQ(expectedParticipantId, participantId);
}

TEST_F(ParticipantIdStorageTest, settingsAreNotAutomaticallySyncedToFile)
{
    const std::string participantID = "participantID-should-not-be-saved-to-file";
    {
        ParticipantIdStorage store(storageFile);
        store.getProviderParticipantId("domain.myDomain",
                                       "interface.mytest");
    }
    {
        ParticipantIdStorage store(storageFile);
        std::string queriedParticipantID = store.getProviderParticipantId("domain.myDomain",
                                                                          "interface.mytest");
        //participantID does not exist
        EXPECT_NE(queriedParticipantID, participantID);
    }
}

