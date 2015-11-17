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
#ifndef UNIQUEVARIANT_H
#define UNIQUEVARIANT_H

#include "JoynrTypeId.h"

#include <memory>
#include <cassert>
#include <iostream>

namespace joynr
{

/**
 * @brief The IVariantHolder Interface to an object that holds variants
 */
class IVariantHolder
{
public:
    /**
     * @brief IVariantHolder
     */
    IVariantHolder()
    {
    }
    /**
     * @brief ~IVariantHolder
     */
    virtual ~IVariantHolder()
    {
    }
    virtual IVariantHolder* clone() const = 0;
    /**
     * @brief getTypeId Get the type id of the variant
     * @return int unique type identifier
     */
    virtual int getTypeId() = 0;
    /**
     * @brief getTypeName
     * @return std::string unique typeName
     */
    virtual std::string getTypeName() const = 0;

private:
    IVariantHolder(const IVariantHolder&) = delete;
    void operator=(const IVariantHolder&) = delete;
};

/**
 * @brief The Variant typename That can be put into datastructures.
 * It holds acctual type in IVariantHolder
 */
class Variant
{
public:
    Variant() : pointer()
    {
    }

    /**
     * @brief Create a variant out of registered Type instance.
     * @return Variant that holds given object in its IVariantHolder
     */
    template <typename T, class... TArgs>
    static Variant make(TArgs&&... args);

    /**
     * @brief Register type T for use in Variants
     */
    template <typename T>
    static bool registerType(std::string name);

    /**
     * @brief Variant is copiable type
     */
    Variant(const Variant& other) : pointer(other.pointer->clone())
    {
    }

    /**
     * @brief Variant move constructor
     * @param variantRvalue
     */
    Variant(Variant&& variantRvalue) : pointer(std::move(variantRvalue.pointer))
    {
    }

    /**
     * @brief operator = to support move semantics
     * @param variantRvalue
     * @return
     */
    Variant& operator=(Variant&& variantRvalue)
    {
        pointer = std::move(variantRvalue.pointer);
        return *this;
    }

    /**
     * @brief operator = to support copy
     */
    Variant& operator=(const Variant& other)
    {
        std::unique_ptr<IVariantHolder> copy(other.pointer->clone());
        std::swap(pointer, copy);
        return *this;
    }

    ~Variant() = default;

    /**
     * @brief is() checks if wrapped object in this Variant have the given type?
     */
    template <typename T>
    bool is() const
    {
        if (!pointer) {
            return false;
        }
        int pointerTypeId = pointer->getTypeId();
        int typenameTypeId = JoynrTypeId<T>::getTypeId();

        return pointerTypeId == typenameTypeId;
    }

    /**
     * @brief getTypeId Get the TypeId of the object held in the Variant
     * @return int unique type identifier
     */
    int getTypeId() const
    {
        return pointer->getTypeId();
    }

    /**
     * @brief getTypeId Get the TypeId of the object held in the Variant
     * @return int unique type identifier
     */
    std::string getTypeName() const
    {
        return pointer->getTypeName();
    }

    /**
     * @brief Get the object reference held (wrapped) in the variant
     * @return
     */
    template <typename T>
    T& get();

    /**
     * @brief Get the constant object reference held (wrapped) in the variant
     * @return
     */
    template <typename T>
    const T& get() const;

    // Serialize a variant
    /**
     * @brief serialize Do actual serialization to output stream
     * @param stream
     */
    void serialize(std::ostream& stream);

private:
    /**
     * @brief Variant Constructor that allows transfer of wrapped object to another Variant
     * instance.
     * Should only be accessed by make().
     * @param variantHolder pointer to IVariantHolder
     */
    Variant(IVariantHolder* variantHolder) : pointer(variantHolder)
    {
    }

    std::unique_ptr<IVariantHolder> pointer;
};

template <typename T>
bool Variant::registerType(std::string typeName)
{
    JoynrTypeId<T>::create(typeName);
    return true;
}

// TODO: check why is this here, if needed implement properly
inline bool operator==(const Variant& lhs, const Variant& rhs)
{
    return true;
}

/**
 * @brief Type specific variant holders
 */
template <typename T>
class VariantHolder : public IVariantHolder
{
public:
    /**
     * @brief VariantHolder copy constructor
     * @param other
     */
    VariantHolder(const T& other) : payload(other)
    {
    }
    /**
     * @brief VariantHolder move constructor
     * @param otherRvalue
     */
    VariantHolder(T&& otherRvalue) : payload(std::move(otherRvalue))
    {
    }
    ~VariantHolder()
    {
    }

    IVariantHolder* clone() const;

    /**
     * @brief getPayload Get object wrapped in IVariantHolder
     * @return Reference to wrapped object
     */
    T& getPayload()
    {
        return payload;
    }

    /**
     * @brief getTypeId of wrapped object
     * @return int unique type identifier
     */
    int getTypeId()
    {
        return JoynrTypeId<T>::getTypeId();
    }

    /**
     * @brief getTypeName of wrapped object
     * @return std::string unique type identifier
     */
    std::string getTypeName() const
    {
        return JoynrTypeId<T>::getTypeName();
    }

private:
    T payload;
};

template <typename T>
T& Variant::get()
{
    assert(is<T>());
    VariantHolder<T>* holder = static_cast<VariantHolder<T>*>(pointer.get());
    return holder->getPayload();
}

template <typename T>
const T& Variant::get() const
{
    assert(is<T>());
    VariantHolder<T>* holder = static_cast<VariantHolder<T>*>(pointer.get());
    return holder->getPayload();
}

template <typename T, class... TArgs>
Variant Variant::make(TArgs&&... args)
{
    return Variant(new VariantHolder<T>(T(std::forward<TArgs>(args)...)));
}

// Copyiable
template <typename T>
typename std::enable_if<std::is_copy_constructible<T>::value, VariantHolder<T>>::type*
copyVariantHolder(const T& value)
{
    return new VariantHolder<T>(T(value));
}

// Not copiable
template <typename T>
typename std::enable_if<!std::is_copy_constructible<T>::value, VariantHolder<T>>::type*
copyVariantHolder(const T& value)
{
    assert(false);
    return nullptr;
}

template <typename T>
IVariantHolder* VariantHolder<T>::clone() const
{
    return copyVariantHolder<T>(payload);
}

} /* namespace joynr */

#endif // UNIQUEVARIANT_H