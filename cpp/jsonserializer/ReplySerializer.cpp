/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "ReplySerializer.h"

#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"
#include "joynr/ArraySerializer.h"

namespace joynr
{

// Register the Reply type id and serializer/deserializer
static const bool isReplySerializerRegistered = SerializerRegistry::registerType<Reply>("joynr.Reply");

template <>
void ClassDeserializer<Reply>::deserialize(Reply& reply, IObject& o)
{
    while (o.hasNextField()) {
        IField& field = o.nextField();
        if (field.name() == "requestReplyId") {
            reply.setRequestReplyId(field.value());
        } else if (field.name() == "response") {
            IArray& array = field.value();
            auto&& converted = convertArray<Variant>(array, convertVariant);
            reply.setResponse(std::forward<std::vector<Variant>>(converted));
        }
    }
}

template <>
void ClassSerializer<Reply>::serialize(const Reply& reply, std::ostream& stream)
{
    stream << R"({)";
    stream << R"("_typeName": ")" << JoynrTypeId<Reply>::getTypeName() << R"(",)";
    stream << R"("requestReplyId": ")" << reply.getRequestReplyId() << R"(",)";
    stream << R"("response": )";
    ArraySerializer::serialize<Variant>(reply.getResponse(), stream);
    stream << R"(})";
}

} /* namespace joynr */