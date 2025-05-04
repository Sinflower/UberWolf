/*
 *  File: Database.h
 *  Copyright (c) 2024 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#pragma once

#include "FileCoder.h"

#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

class Field
{
public:
	Field() = default;

	explicit Field(FileCoder& coder)
	{
		m_name = coder.ReadString();
	}

	void DumpProject(FileCoder& coder) const
	{
		coder.WriteString(m_name);
	}

	nlohmann::ordered_json ToJson() const
	{
		nlohmann::ordered_json j;
		j["name"] = ToUTF8(m_name);

		if (!m_stringArgs.empty())
		{
			j["stringArgs"] = nlohmann::ordered_json::array();

			for (const tString& stringArg : m_stringArgs)
				j["stringArgs"].push_back(ToUTF8(stringArg));
		}

		return j;
	}

	void Patch(const nlohmann::ordered_json& j)
	{
		CHECK_JSON_KEY(j, "name", "fields");

		m_name = ToUTF16(j["name"].get<std::string>());

		if (j.contains("stringArgs"))
		{
			m_stringArgs.clear();
			for (const auto& stringArg : j["stringArgs"])
				m_stringArgs.push_back(ToUTF16(stringArg.get<std::string>()));
		}
	}

	void ReadDat(FileCoder& coder)
	{
		m_indexInfo = coder.ReadInt();
	}

	void DumpDat(FileCoder& coder) const
	{
		coder.WriteInt(m_indexInfo);
	}

	bool IsString() const
	{
		return m_indexInfo >= STRING_START;
	}

	bool IsInt() const
	{
		return !IsString();
	}

	uint32_t Index() const
	{
		if (IsString())
			return m_indexInfo - STRING_START;
		else
			return m_indexInfo - INT_START;
	}

	void SetType(const uint8_t& type)
	{
		m_type = type;
	}

	const uint8_t& GetType() const
	{
		return m_type;
	}

	void SetUnknown1(const tString& unknown1)
	{
		m_unknown1 = unknown1;
	}

	const tString& GetUnknown1() const
	{
		return m_unknown1;
	}

	void SetStringArgs(const tStrings& stringArgs)
	{
		m_stringArgs = stringArgs;
	}

	const tStrings& GetStringArgs() const
	{
		return m_stringArgs;
	}

	void SetArgs(const uInts& args)
	{
		m_args = args;
	}

	const uInts& GetArgs() const
	{
		return m_args;
	}

	void SetDefaultValue(const uint32_t& defaultValue)
	{
		m_defaultValue = defaultValue;
	}

	const uint32_t& GetDefaultValue() const
	{
		return m_defaultValue;
	}

	const tString& GetName() const
	{
		return m_name;
	}

private:
private:
	tString m_name          = TEXT("");
	uint8_t m_type          = 0;
	tString m_unknown1      = TEXT("");
	tStrings m_stringArgs   = {};
	uInts m_args            = {};
	uint32_t m_defaultValue = 0;
	uint32_t m_indexInfo    = 0;

	static const uint32_t STRING_START = 0x07D0;
	static const uint32_t INT_START    = 0x03E8;
};

using Fields = std::vector<Field>;

class Data
{
public:
	Data() = default;

	explicit Data(FileCoder& coder)
	{
		m_name = coder.ReadString();
	}

	void DumpProject(FileCoder& coder) const
	{
		coder.WriteString(m_name);
	}

	nlohmann::ordered_json ToJson() const
	{
		nlohmann::ordered_json j;
		j["name"] = ToUTF8(m_name);
		j["data"] = nlohmann::json::array();

		if (m_stringValues.empty() && m_intValues.empty())
			return j;

		for (const Field& field : *m_pFields)
		{
			nlohmann::ordered_json fieldData;
			fieldData["name"] = ToUTF8(field.GetName());

			if (field.IsString())
				fieldData["value"] = ToUTF8(m_stringValues[field.Index()]);
			else
				fieldData["value"] = m_intValues[field.Index()];

			j["data"].push_back(fieldData);
		}

		return j;
	}

	void Patch(const nlohmann::ordered_json& j)
	{
		CHECK_JSON_KEY(j, "name", "data");
		CHECK_JSON_KEY(j, "data", "data");

		m_name = ToUTF16(j["name"]);

		if (m_stringValues.empty() && m_intValues.empty()) return;

		for (std::size_t i = 0; i < m_pFields->size(); i++)
		{
			const nlohmann::ordered_json& fieldData = j["data"][i];
			const std::string dataStr               = std::format("data[{}]", i);

			const Field& field    = m_pFields->at(i);
			std::string fieldName = ToUTF8(field.GetName());

			CHECK_JSON_KEY(fieldData, "name", dataStr);
			CHECK_JSON_KEY(fieldData, "value", dataStr);

			const std::string fieldNameJson = fieldData["name"].get<std::string>();

			if (fieldName != fieldNameJson)
				throw WolfRPGException(ERROR_TAG + "Data field name mismatch - Expected: \"" + fieldName + "\" - Got: \"" + fieldNameJson + "\"");

			if (field.IsString())
				m_stringValues[field.Index()] = ToUTF16(fieldData["value"].get<std::string>());
			else
				m_intValues[field.Index()] = fieldData["value"].get<uint32_t>();
		}
	}

	void ReadDat(FileCoder& coder, Fields& fields, const uint32_t& fieldsSize)
	{
		m_pFields       = &fields;
		uint32_t intCnt = 0;
		uint32_t strCnt = 0;

		for (uint32_t i = 0; i < fieldsSize; i++)
		{
			if (fields[i].IsString())
				strCnt++;
			else
				intCnt++;
		}

		for (uint32_t i = 0; i < intCnt; i++)
			m_intValues.push_back(coder.ReadInt());

		for (uint32_t i = 0; i < strCnt; i++)
			m_stringValues.push_back(coder.ReadString());
	}

	void DumpDat(FileCoder& coder) const
	{
		for (const uint32_t& i : m_intValues)
			coder.WriteInt(i);

		for (const tString& str : m_stringValues)
			coder.WriteString(str);
	}

	const tString& GetName() const
	{
		return m_name;
	}

private:
private:
	tString m_name          = TEXT("");
	uInts m_intValues       = {};
	tStrings m_stringValues = {};
	Fields* m_pFields       = nullptr;
};

using Datas = std::vector<Data>;

class Type
{
public:
	explicit Type(FileCoder& coder)
	{
		m_name            = coder.ReadString();
		uint32_t fieldCnt = coder.ReadInt();
		for (uint32_t i = 0; i < fieldCnt; i++)
			m_fields.push_back(Field(coder));

		uint32_t dataCnt = coder.ReadInt();
		for (uint32_t i = 0; i < dataCnt; i++)
			m_data.push_back(Data(coder));

		m_description = coder.ReadString();

		m_fieldTypeListSize = coder.ReadInt();
		uint32_t index      = 0;

		for (index = 0; index < m_fields.size(); index++)
			m_fields[index].SetType(coder.ReadByte());

		coder.Skip(m_fieldTypeListSize - index);

		index = coder.ReadInt();
		for (uint32_t i = 0; i < index; i++)
			m_fields[i].SetUnknown1(coder.ReadString());

		index = coder.ReadInt();
		for (uint32_t i = 0; i < index; i++)
		{
			tStrings stringArgs;
			uint32_t argsCnt = coder.ReadInt();
			for (uint32_t j = 0; j < argsCnt; j++)
				stringArgs.push_back(coder.ReadString());
			m_fields[i].SetStringArgs(stringArgs);
		}

		index = coder.ReadInt();
		for (uint32_t i = 0; i < index; i++)
		{
			uInts args;
			uint32_t argsCnt = coder.ReadInt();
			for (uint32_t j = 0; j < argsCnt; j++)
				args.push_back(coder.ReadInt());
			m_fields[i].SetArgs(args);
		}

		index = coder.ReadInt();
		for (uint32_t i = 0; i < index; i++)
			m_fields[i].SetDefaultValue(coder.ReadInt());
	}

	void DumpProject(FileCoder& coder) const
	{
		coder.WriteString(m_name);
		coder.WriteInt(m_fields.size());

		for (const Field& field : m_fields)
			field.DumpProject(coder);

		coder.WriteInt(m_data.size());
		for (const Data& data : m_data)
			data.DumpProject(coder);

		coder.WriteString(m_description);

		coder.WriteInt(m_fieldTypeListSize);

		for (const Field& field : m_fields)
			coder.WriteByte(field.GetType());

		for (uint32_t index = static_cast<uint32_t>(m_fields.size()); index < m_fieldTypeListSize; index++)
			coder.WriteByte(0);

		coder.WriteInt(m_fields.size());
		for (const Field& field : m_fields)
			coder.WriteString(field.GetUnknown1());

		coder.WriteInt(m_fields.size());
		for (const Field& field : m_fields)
		{
			coder.WriteInt(field.GetStringArgs().size());
			for (const tString& stringArg : field.GetStringArgs())
				coder.WriteString(stringArg);
		}

		coder.WriteInt(m_fields.size());
		for (const Field& field : m_fields)
		{
			coder.WriteInt(field.GetArgs().size());
			for (const uint32_t& arg : field.GetArgs())
				coder.WriteInt(arg);
		}

		coder.WriteInt(m_fields.size());
		for (const Field& field : m_fields)
			coder.WriteInt(field.GetDefaultValue());
	}

	bool ReadDat(FileCoder& coder)
	{
		VERIFY_MAGIC(coder, DAT_TYPE_SEPARATOR);
		m_unknown1   = coder.ReadInt();
		m_fieldsSize = coder.ReadInt();

		for (uint32_t i = 0; i < m_fieldsSize; i++)
			m_fields[i].ReadDat(coder);

		uint32_t dataSize = coder.ReadInt();

		if (m_data.size() > dataSize)
			m_data.resize(dataSize);

		for (Data& datum : m_data)
			datum.ReadDat(coder, m_fields, m_fieldsSize);

		return true;
	}

	void DumpDat(FileCoder& coder) const
	{
		coder.Write(DAT_TYPE_SEPARATOR);
		coder.WriteInt(m_unknown1);
		coder.WriteInt(m_fieldsSize);
		for (uint32_t i = 0; i < m_fieldsSize; i++)
			m_fields[i].DumpDat(coder);

		coder.WriteInt(m_data.size());
		for (const Data& datum : m_data)
			datum.DumpDat(coder);
	}

	nlohmann::ordered_json ToJson() const
	{
		nlohmann::ordered_json j;
		j["name"]        = ToUTF8(m_name);
		j["description"] = ToUTF8(m_description);
		j["fields"]      = nlohmann::json::array();
		j["data"]        = nlohmann::json::array();

		for (const Field& field : m_fields)
			j["fields"].push_back(field.ToJson());

		for (const Data& data : m_data)
			j["data"].push_back(data.ToJson());

		return j;
	}

	void Patch(const nlohmann::ordered_json& j)
	{
		CHECK_JSON_KEY(j, "name", "types");
		CHECK_JSON_KEY(j, "description", "types");
		CHECK_JSON_KEY(j, "fields", "types");
		CHECK_JSON_KEY(j, "data", "types");

		m_name        = ToUTF16(j["name"].get<std::string>());
		m_description = ToUTF16(j["description"].get<std::string>());

		if (m_fields.size() != j["fields"].size())
			throw WolfRPGException(ERROR_TAG + "Count mismatch for object 'fields' expected: " + std::to_string(m_fields.size()) + " - got: " + std::to_string(j["fields"].size()));

		for (std::size_t i = 0; i < m_fields.size(); i++)
			m_fields[i].Patch(j["fields"][i]);

		if (m_data.size() != j["data"].size())
			throw WolfRPGException(ERROR_TAG + "Count mismatch for object 'data' expected: " + std::to_string(m_data.size()) + " - got: " + std::to_string(j["data"].size()));

		for (std::size_t i = 0; i < m_data.size(); i++)
			m_data[i].Patch(j["data"][i]);
	}

	const Datas& GetData() const
	{
		return m_data;
	}

	const tString& GetName() const
	{
		return m_name;
	}

private:
private:
	tString m_name               = TEXT("");
	tString m_description        = TEXT("");
	Fields m_fields              = {};
	uint32_t m_fieldsSize        = 0;
	Datas m_data                 = {};
	uint32_t m_unknown1          = 0;
	uint32_t m_fieldTypeListSize = 0;

	inline static const Bytes DAT_TYPE_SEPARATOR{ 0xFE, 0xFF, 0xFF, 0xFF };
};

using Types = std::vector<Type>;

// TODO: Why not derive from WolfDataBase?
class Database
{
public:
	Database(const tString& projectFileName, const tString& datFileName) :
		m_projectFileName(projectFileName),
		m_datFileName(datFileName)
	{
		m_valid = init();
	}

	void Dump(const tString& outputDir) const
	{
		{
			const tString fileName = ::GetFileName(m_projectFileName);
			g_activeFile           = fileName;

			tString outputFN = outputDir + L"/" + fileName;
			FileCoder coder(outputFN, FileCoder::Mode::WRITE, WolfFileType::Project);
			coder.WriteInt(m_types.size());
			for (const Type& type : m_types)
				type.DumpProject(coder);
		}

		const tString fileName = ::GetFileName(m_datFileName);
		g_activeFile           = fileName;

		tString outputFN = outputDir + L"/" + fileName;
		FileCoder coder(outputFN, FileCoder::Mode::WRITE, WolfFileType::DataBase, DAT_SEED_INDICES);
		FileCoder bufCoder(FileCoder::Mode::WRITE, WolfFileType::DataBase);
		FileCoder* pCoder = &coder;

		coder.Write(DAT_MAGIC_NUMBER);
		coder.WriteByte(m_version);

		if (m_version == 0xC4)
			pCoder = &bufCoder;

		pCoder->WriteInt(m_types.size());
		for (const Type& type : m_types)
			type.DumpDat(*pCoder);

		pCoder->WriteByte(m_version);

		if (m_version == 0xC4)
		{
			bufCoder.Pack();
			coder.WriteCoder(bufCoder);
		}
	}

	void ToJson(const tString& outputFolder) const
	{
		const tString fileName = ::GetFileNameNoExt(m_datFileName);
		g_activeFile           = fileName;

		nlohmann::ordered_json j;
		j["types"] = nlohmann::json::array();

		for (const Type& type : m_types)
			j["types"].push_back(type.ToJson());

		const tString outputFile = outputFolder + L"/" + fileName + L".json";

		std::ofstream out(outputFile);
		out << j.dump(4);

		out.close();
	}

	void Patch(const tString& patchFolder)
	{
		const tString fileName = ::GetFileNameNoExt(m_datFileName);
		g_activeFile           = fileName;

		const tString patchFile = patchFolder + L"/" + fileName + L".json";
		if (!std::filesystem::exists(patchFile))
			throw WolfRPGException(ERROR_TAGW + L"Patch file not found: " + patchFile);

		nlohmann::ordered_json j;
		std::ifstream in(patchFile);
		in >> j;
		in.close();

		CHECK_JSON_KEY(j, "types", "Database");

		if (m_types.size() != j["types"].size())
			throw WolfRPGException(ERROR_TAG + "Count mismatch for object 'types' expected: " + std::to_string(m_types.size()) + " - got: " + std::to_string(j["types"].size()));

		for (std::size_t i = 0; i < m_types.size(); i++)
			m_types[i].Patch(j["types"][i]);
	}

	const Types& GetTypes() const
	{
		return m_types;
	}

	const bool& IsValid() const
	{
		return m_valid;
	}

private:
	bool init()
	{
		g_activeFile = ::GetFileName(m_datFileName);
		FileCoder coder(m_datFileName, FileCoder::Mode::READ, WolfFileType::DataBase, DAT_SEED_INDICES);
		if (coder.IsEncrypted())
			m_cryptHeader = coder.GetCryptHeader();
		else
			VERIFY_MAGIC(coder, DAT_MAGIC_NUMBER)

		m_version = coder.ReadByte();

		// Process the project file
		{
			g_activeFile = ::GetFileName(m_projectFileName);
			FileCoder coder(m_projectFileName, FileCoder::Mode::READ, WolfFileType::Project);
			uint32_t typeCnt = coder.ReadInt();
			for (uint32_t i = 0; i < typeCnt; i++)
				m_types.push_back(Type(coder));

			if (!coder.IsEof())
				throw WolfRPGException(ERROR_TAGW + L"Database [" + m_projectFileName + L"] has more data than expected");
		}

		g_activeFile     = ::GetFileName(m_datFileName);
		uint32_t typeCnt = coder.ReadInt();
		if (typeCnt != m_types.size())
		{
			throw WolfRPGException(ERROR_TAGW + L"Database [" + m_datFileName + L"] project and dat type count mismatch expected: " + std::to_wstring(m_types.size()) + L"  - got: " + std::to_wstring(typeCnt));
			return false;
		}

		uint32_t cnt = 0;
		for (Type& type : m_types)
		{
			cnt++;
			if (!type.ReadDat(coder))
			{
				throw WolfRPGException(ERROR_TAG + "Failed at readDat for type(" + std::to_string(cnt) + ")");
				return false;
			}
		}

		if (coder.ReadByte() != m_version)
			throw WolfRPGException(ERROR_TAGW + L"No " + Dec2HexW(m_version) + L" terminator at the end of \"" + m_datFileName + L"\"");

		if (!coder.IsEof())
			throw WolfRPGException(ERROR_TAGW + L"Database [" + m_datFileName + L"] has more data than expected");

		return true;
	}

private:
	Types m_types       = {};
	Bytes m_cryptHeader = {};

	BYTE m_version = 0;
	bool m_valid   = false;
	tString m_projectFileName;
	tString m_datFileName;

	inline static const uInts DAT_SEED_INDICES{ 0, 3, 9 };
	inline static const MagicNumber DAT_MAGIC_NUMBER{ { 0x57, 0x00, 0x00, 0x4F, 0x4C, 0x00, 0x46, 0x4D, 0x00 }, 5 };
};

using Databases = std::vector<Database>;
