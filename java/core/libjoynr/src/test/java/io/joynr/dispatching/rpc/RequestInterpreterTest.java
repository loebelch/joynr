package io.joynr.dispatching.rpc;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.context.JoynrMessageScope;
import io.joynr.dispatching.RequestCaller;
import joynr.OneWayRequest;

/**
 * Unit tests for {@link RequestInterpreter}.
 */
@RunWith(MockitoJUnitRunner.class)
public class RequestInterpreterTest {

    private static class TestRequestCaller implements RequestCaller {
        @SuppressWarnings("unused")
        public void test() {
        }
    }

    @Mock
    private JoynrMessageScope joynrMessageScope;

    private RequestInterpreter subject;

    @Before
    public void setup() {
        subject = new RequestInterpreter(joynrMessageScope);
    }

    @Test
    public void testJoynrMessageScopeActivated() {
        RequestCaller requestCaller = new TestRequestCaller();
        OneWayRequest request = new OneWayRequest("test", new Object[0], new Class[0]);
        subject.invokeMethod(requestCaller, request);
        verify(joynrMessageScope).activate();
        verify(joynrMessageScope).deactivate();
    }

}
