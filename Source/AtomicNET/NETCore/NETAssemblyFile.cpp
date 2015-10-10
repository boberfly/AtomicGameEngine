//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Atomic/Core/Context.h>
#include <Atomic/IO/Deserializer.h>
#include <Atomic/IO/Log.h>
#include <Atomic/Core/Profiler.h>
#include <Atomic/Resource/ResourceCache.h>
#include <Atomic/IO/Serializer.h>

#include "NETAssemblyFile.h"

namespace Atomic
{

HashMap<StringHash, VariantType> NETAssemblyFile::typeMap_;

NETAssemblyFile::NETAssemblyFile(Context* context) :
    ScriptComponentFile(context)
{

}

NETAssemblyFile::~NETAssemblyFile()
{

}

void NETAssemblyFile::InitTypeMap()
{
    typeMap_["Boolean"] = VAR_BOOL;
    typeMap_["Int32"] = VAR_INT;
    typeMap_["Single"] = VAR_FLOAT;
    typeMap_["Double"] = VAR_DOUBLE;
    typeMap_["String"] = VAR_STRING;
    typeMap_["Vector2"] = VAR_VECTOR2;
    typeMap_["Vector3"] = VAR_VECTOR3;
    typeMap_["Vector4"] = VAR_VECTOR4;
    typeMap_["Quaternion"] = VAR_QUATERNION;

}

bool NETAssemblyFile::ParseComponentClassJSON(const JSONValue& json)
{
    if (!typeMap_.Size())
        InitTypeMap();

    String className = json.Get("name").GetString();

    const JSONValue& jfields = json.Get("fields");

    PODVector<StringHash> enumsAdded;

    if (jfields.IsArray())
    {
        for (unsigned i = 0; i < jfields.GetArray().Size(); i++)
        {
            const JSONValue& jfield = jfields.GetArray().At(i);

            VariantType varType = VAR_NONE;

            bool isEnum = jfield.Get("isEnum").GetBool();
            String typeName = jfield.Get("typeName").GetString();
            String fieldName = jfield.Get("name").GetString();
            String defaultValue = jfield.Get("defaultValue").GetString();

            if (isEnum && assemblyEnums_.Contains(typeName) && !enumsAdded.Contains(typeName))
            {
                varType = VAR_INT;
                enumsAdded.Push(typeName);
                const Vector<EnumInfo>& einfos = assemblyEnums_[typeName];
                for (unsigned i = 0; i < einfos.Size(); i++)
                    AddEnum(typeName, einfos[i], className);
            }

            if (varType == VAR_NONE && typeMap_.Contains(typeName))
                varType = typeMap_[typeName];

            if (varType == VAR_NONE)
            {
                LOGERRORF("Component Class %s contains unmappable type %s in field %s",
                          className.CString(), typeName.CString(), fieldName.CString());

                continue;
            }

            if (defaultValue.Length())
            {
                Variant value;
                value.FromString(varType, defaultValue);

                AddDefaultValue(fieldName, value, className);
            }

            AddField(fieldName, varType, className);

        }

    }

    return true;
}

bool NETAssemblyFile::ParseAssemblyJSON(const JSONValue& json)
{
    Clear();
    assemblyEnums_.Clear();

    const JSONArray& enums = json.Get("enums").GetArray();

    // parse to all enums hash
    for (unsigned i = 0; i < enums.Size(); i++)
    {
        const JSONValue& ejson = enums.At(i);

        String enumName = ejson.Get("name").GetString();

        const JSONObject& evalues = ejson.Get("values").GetObject();

        JSONObject::ConstIterator itr = evalues.Begin();

        Vector<EnumInfo> values;

        while(itr != evalues.End())
        {
            EnumInfo info;
            info.name_ = itr->first_;
            info.value_ = itr->second_.GetInt();
            values.Push(info);
            itr++;
        }

        assemblyEnums_[enumName] = values;
    }

    const JSONArray& components = json.Get("components").GetArray();

    for (unsigned i = 0; i < components.Size(); i++)
    {
        const JSONValue& cjson = components.At(i);

        ParseComponentClassJSON(cjson);
    }

    return true;
}

void NETAssemblyFile::RegisterObject(Context* context)
{
    context->RegisterFactory<NETAssemblyFile>();
}

bool NETAssemblyFile::BeginLoad(Deserializer& source)
{
    return true;
}

bool NETAssemblyFile::Save(Serializer& dest) const
{
    return true;
}

}
