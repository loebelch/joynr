/*jslint node: true */

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
var defaultMessagingSettings =
        require('../../../classes/joynr/start/settings/defaultMessagingSettings');
var MessagingQos = require('../../../classes/joynr/messaging/MessagingQos');
var MessagingQosEffort = require('../../../classes/joynr/messaging/MessagingQosEffort');

describe("libjoynr-js.joynr.messaging.MessagingQos", function() {
    it("is instantiable", function() {
        expect(new MessagingQos()).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            effort : MessagingQosEffort.BEST_EFFORT
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            encrypt : true
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            effort : MessagingQosEffort.BEST_EFFORT,
            encrypt : true
        })).toBeDefined();
        expect(new MessagingQos({
            effort : MessagingQosEffort.BEST_EFFORT,
            encrypt : true
        })).toBeDefined();
        expect(new MessagingQos({
            encrypt : true
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            effort : MessagingQosEffort.BEST_EFFORT,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            effort : MessagingQosEffort.BEST_EFFORT,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            encrypt : true,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            ttl : 60000,
            effort : MessagingQosEffort.BEST_EFFORT,
            encrypt : true,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            effort : MessagingQosEffort.BEST_EFFORT,
            encrypt : true,
            compress : true
        })).toBeDefined();
        expect(new MessagingQos({
            compress : true,
            encrypt : true
        })).toBeDefined();
    });

    it("is of correct type", function() {
        var emptyMessagingQos = new MessagingQos();
        expect(emptyMessagingQos).toBeDefined();
        expect(emptyMessagingQos).not.toBeNull();
        expect(typeof emptyMessagingQos === "object").toBeTruthy();
        expect(emptyMessagingQos instanceof MessagingQos).toBeTruthy();

        var defaultMessagingQos = new MessagingQos();
        expect(defaultMessagingQos).toBeDefined();
        expect(defaultMessagingQos).not.toBeNull();
        expect(typeof defaultMessagingQos === "object").toBeTruthy();
        expect(defaultMessagingQos instanceof MessagingQos).toEqual(true);
    });

    it("constructs correct default object", function() {
        expect(new MessagingQos()).toEqual(new MessagingQos({
            ttl : MessagingQos.DEFAULT_TTL
        }));
        expect(new MessagingQos().encrypt).toEqual(false);
        expect(new MessagingQos().compress).toEqual(false);
    });

    function testTtlValues(ttl) {
        var messagingQos = new MessagingQos({
            ttl : ttl
        });
        expect(messagingQos.ttl).toBe(ttl);
    }

    it("constructs with correct TTL values", function() {
        testTtlValues(123456, 1234567);
        testTtlValues(0, 0);
        testTtlValues(-123456, -1234567);
    });

    it("prevents ttl values larger than maxTtl", function() {
        expect(new MessagingQos({
            ttl : defaultMessagingSettings.MAX_MESSAGING_TTL_MS + 1
        })).toEqual(new MessagingQos({
            ttl : defaultMessagingSettings.MAX_MESSAGING_TTL_MS
        }));
    });

    function testEffortValues(effort, expected) {
        var messagingQos = new MessagingQos({
            effort : effort
        });
        expect(messagingQos.effort).toBe(expected);
    }

    it("constructs with correct effort values", function() {
        testEffortValues(MessagingQosEffort.NORMAL, MessagingQosEffort.NORMAL);
        testEffortValues(MessagingQosEffort.BEST_EFFORT, MessagingQosEffort.BEST_EFFORT);
        testEffortValues(null, MessagingQosEffort.NORMAL);
        testEffortValues("not an enum value", MessagingQosEffort.NORMAL);
    });

    function testEncryptValues(encrypt) {
        var messagingQos = new MessagingQos({
            encrypt : encrypt
        });
        expect(messagingQos.encrypt).toBe(encrypt);
    }

    it("constructs with correct encrypt values", function() {
        testEncryptValues(false);
        testEncryptValues(true);
    });

    function testCompressValues(compress) {
        var messagingQos = new MessagingQos({
            compress : compress
        });
        expect(messagingQos.compress).toBe(compress);
    }

    it("constructs with correct compress values", function() {
        testCompressValues(false);
        testCompressValues(true);
    });

    var runsWithCustomHeaders = [
        {
            params : {
                key : "key",
                value : "value"
            },
            ok : true
        },
        {
            params : {
                key : "1key",
                value : "1value"
            },
            ok : true
        },
        {
            params : {
                key : "key1",
                value : "value1"
            },
            ok : true
        },
        {
            params : {
                key : "key-1",
                value : "value1"
            },
            ok : true
        },
        {
            params : {
                key : "123",
                value : "123"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one;two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one:two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one,two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one+two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one&two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one?two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one-two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one.two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one*two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one/two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "one\\two"
            },
            ok : true
        },
        {
            params : {
                key : "key",
                value : "wrongvalue$"
            },
            ok : false
        },
        {
            params : {
                key : "key",
                value : "wrongvalue%"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey ",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey;",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey:",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey,",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey+",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey&",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey?",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey.",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey*",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey/",
                value : "value"
            },
            ok : false
        },
        {
            params : {
                key : "wrongkey\\",
                value : "value"
            },
            ok : false
        }
    ];
    runsWithCustomHeaders.forEach(function(run) {
        var expectedTo = run.ok ? "passes" : "fails";
        var params = run.params;
        it("setting custom header "
            + expectedTo
            + " when key: "
            + params.key
            + " value: "
            + params.value, function() {
            var key = params.key;
            var value = params.value;
            var messagingQos = new MessagingQos();
            if (run.ok) {
                messagingQos.putCustomMessageHeader(key, value);
                expect(messagingQos.customHeaders[key]).toEqual(value);
            } else {
                expect(function() {
                    messagingQos.putCustomMessageHeader(key, value);
                }).toThrow();
            }
        });
    });
});
