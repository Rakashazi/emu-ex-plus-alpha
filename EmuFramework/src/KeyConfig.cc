/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/Option.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"KeyConfig"};

KeyConfig KeyConfig::readConfig(MapIO &io)
{
	KeyConfig keyConf;
	keyConf.map = Input::Map(io.get<uint8_t>());
	const auto keyMax = Input::KeyEvent::mapNumKeys(keyConf.map);
	readSizedData<uint8_t>(io, keyConf.name);
	auto mappings = io.get<uint8_t>();
	keyConf.keyMap.reserve(mappings);
	for([[maybe_unused]] auto mappingIdx : iotaCount(mappings))
	{
		KeyMapping m;
		io.read(m.key.codes.data(), m.key.codes.capacity());
		m.key.flags = std::bit_cast<KeyFlags>(io.get<uint8_t>());
		io.read(m.mapKey.data(), m.mapKey.capacity());
		keyConf.set(m.key, m.mapKey);
	}
	// verify keys
	for(auto &m : keyConf.keyMap)
	{
		for(auto &k : m.mapKey)
		{
			if(k  >= keyMax)
			{
				log.warn("key code {:X} out of range for map type {}", k, (int)keyConf.map);
				k = 0;
			}
		}
	}
	log.info("read config:{}", keyConf.name);
	return keyConf;
}

void KeyConfig::writeConfig(FileIO &io) const
{
	constexpr auto keyMappingSize = sizeof(KeyInfo) + sizeof(MappedKeys);
	ssize_t bytes = 2; // config key size
	bytes += 1; // input map type
	bytes += 1; // name string length
	bytes += name.size(); // name string
	bytes += 1; // number of mappings present
	bytes += keyMap.size() * keyMappingSize;
	assert(bytes <= 0xFFFF);
	log.info("saving config:{}, {} bytes", name, bytes);
	io.put(uint16_t(bytes));
	io.put(uint16_t(CFGKEY_INPUT_KEY_CONFIGS_V2));
	io.put(uint8_t(map));
	io.put(uint8_t(name.size()));
	io.write(name.data(), name.size());
	io.put(uint8_t(keyMap.size()));
	for(const auto &m : keyMap)
	{
		io.write(m.key.codes.data(), m.key.codes.capacity());
		io.put(std::bit_cast<uint8_t>(m.key.flags));
		io.write(m.mapKey.data(), m.mapKey.capacity());
	}
}

void KeyConfig::set(KeyInfo key, MappedKeys mapKey)
{
	if(!mapKey[0])
	{
		std::erase_if(keyMap, [&](auto &val){ return val.key == key; });
		return;
	}
	if(auto it = find(key);
		it != keyMap.end())
	{
		log.info("changing key mapping from:{:X} to {:X}", it->mapKey[0], mapKey[0]);
		it->mapKey = mapKey;
	}
	else
	{
		log.info("adding key mapping:{:X}", mapKey[0]);
		keyMap.emplace_back(key, mapKey);
	}
}

void KeyConfig::unbindCategory(const KeyCategory &category)
{
	for(auto p: category.keys)
	{
		set(p, {});
	}
}

void KeyConfig::resetCategory(const KeyCategory &category, KeyConfigDesc defaultConf)
{
	for(auto p: category.keys)
	{
		set(p, defaultConf.get(p));
	}
}

}
