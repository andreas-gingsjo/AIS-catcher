/*
	Copyright(c) 2021-2022 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <iostream>

#include "Utilities.h"
#include "Common.h"

#define JSON_DICT_FULL	  0
#define JSON_DICT_MINIMAL 1
#define JSON_DICT_SPARSE  2
#define JSON_DICT_APRS	  3
#define JSON_DICT_SETTING 4

namespace JSON {

	extern const std::vector<std::vector<std::string>> KeyMap;
	class JSON;
	class Property;

	struct Value {
		enum class Type {
			BOOL,
			INT,
			FLOAT,
			STRING,
			OBJECT,
			ARRAY_STRING,
			ARRAY,
			EMPTY
		} type;
		union Data {
			bool b;
			int i;
			float f;
			std::string* s;
			std::vector<std::string>* as;
			std::vector<Value>* a;
			JSON* o;
		} d;
	};

	class Property {
	public:
	protected:
		int key;
		Value value;

	public:
		Property(int p, Value v) {
			key = p;
			value = v;
		}
		Property(int p, int v) {
			key = p;
			value.d.i = v;
			value.type = Value::Type::INT;
		}
		Property(int p, float v) {
			key = p;
			value.d.f = v;
			value.type = Value::Type::FLOAT;
		}
		Property(int p, bool v) {
			key = p;
			value.d.b = v;
			value.type = Value::Type::BOOL;
		}
		Property(int p, std::string* v) {
			key = p;
			value.d.s = v;
			value.type = Value::Type::STRING;
		}
		Property(int p, std::vector<std::string>* v) {
			key = p;
			value.d.as = v;
			value.type = Value::Type::ARRAY_STRING;
		}
		Property(int p, JSON* v) {
			key = p;
			value.d.o = v;
			value.type = Value::Type::OBJECT;
		}
		Property(int p) {
			key = p;
			value.type = Value::Type::EMPTY;
		}
		Value::Type getType() const { return value.type; }
		int getKey() const { return key; }
		Value Get() const { return value; }
	};

	class JSON {
	protected:
		std::vector<Property> objects;
		std::vector<std::string*> strings;
		std::vector<std::vector<Value>*> arrays;

		friend class StringBuilder;
		friend class Parser;

	public:
		void* binary = NULL;

		~JSON() {
			Clear();
		}

		void Clear() {
			for (const auto& o : objects) {
				if (o.getType() == Value::Type::OBJECT) {
					delete o.Get().d.o;
				}
			}

			for (const auto& s : strings) {
				delete s;
			}

			for (const auto& a : arrays) {
				for (const auto& v : *a)
					if (v.type == Value::Type::OBJECT) {
						delete v.d.o;
					}
			}
			objects.clear();
			strings.clear();
			arrays.clear();
		}

		JSON* Get(int p1) {
			for (auto& o : objects) {
				if (o.getKey() == p1 && o.getType() == Value::Type::OBJECT)
					return o.Get().d.o;
			}
			return nullptr;
		}

		bool getValue(int p1, Value& v) {
			for (auto& o : objects) {
				if (o.getKey() == p1) {
					v = o.Get();
					return true;
				}
			}
			return false;
		}

		bool getString(int p1, int p2, std::string& str) {
			JSON* json = Get(p1);
			if (json == nullptr) return false;
			Value v;
			if (!json->getValue(p2, v)) return false;
			if (v.type != Value::Type::STRING) return false;
			str = *v.d.s;
			return true;
		}

		bool getInt(int p1, int p2, int& i) {
			JSON* json = Get(p1);
			if (json == nullptr) return false;
			Value v;
			if (!json->getValue(p2, v)) return false;
			if (v.type != Value::Type::INT) return false;
			i = v.d.i;
			return true;
		}

		void Submit(int p, int v) {
			objects.push_back(Property(p, v));
		}
		void Submit(int p, float v) {
			objects.push_back(Property(p, v));
		}
		void Submit(int p, bool v) {
			objects.push_back(Property(p, v));
		}
		void Submit(int p, JSON* v) {
			objects.push_back(Property(p, (JSON*)v));
		}
		void Submit(int p, const std::string* v) {
			objects.push_back(Property(p, (std::string*)v));
		}
		void Submit(int p, const std::string& v) {
			strings.push_back(new std::string(v));
			objects.push_back(Property(p, (std::string*)&strings.back()));
		}
		void Submit(int p, const std::vector<std::string>* v) {
			objects.push_back(Property(p, (std::vector<std::string>*)v));
		}
		void Submit(int p) {
			objects.push_back(Property(p));
		}
		void Submit(int p, Value v) {
			objects.push_back(Property(p, v));
		}
	};

	class StringBuilder {
	protected:
	private:
		int map = JSON_DICT_FULL;
		void to_string(std::string& json, const Value& v, int& idx);

	public:
		void build(const JSON& objects, std::string& json);
		void jsonify(const std::string& str, std::string& json);

		// dictionary to use
		void setMap(int m) { map = m; }
	};

	class Parser {
	protected:
	private:
		class Token;

		int map = JSON_DICT_SETTING;
		std::string json;
		std::vector<Token> tokens;

		enum class TokenType {
			LeftBrace,
			RightBrace,
			LeftBracket,
			RightBracket,
			Integer,
			FloatingPoint,
			Colon,
			Comma,
			True,
			False,
			Null,
			String,
			End
		};

		struct Token {
			int pos = 0;
			TokenType type;
			std::string text;

			Token(TokenType t, const std::string& x, int p) {
				pos = p;
				type = t;
				text = x;
			}
		};

		void error(const std::string& err, int pos);

		// tokenizer
		void skip_whitespace(int&);
		void tokenizer();

		// parser
		int idx = 0;

		void error_parser(const std::string& err);
		bool is_match(TokenType t);
		void must_match(TokenType t, const std::string& err);
		int search(const std::string& s);
		void next();
		JSON* parse_core();
		Value parse_value(JSON* o);

	public:
		JSON* parse(const std::string& j);

		// dictionary to use
		void setMap(int m) { map = m; }
	};

	// JSON keys
	enum Keys {
		KEY_CLASS,
		KEY_DEVICE,
		KEY_SCALED,
		KEY_CHANNEL,
		KEY_SIGNAL_POWER,
		KEY_PPM,
		KEY_RXTIME,
		KEY_NMEA,
		KEY_ETA,
		KEY_SHIPTYPE_TEXT,
		KEY_AID_TYPE_TEXT,
		// Settings
		KEY_SETTING_BANDWIDTH,
		KEY_SETTING_BIAS_TEE,
		KEY_SETTING_BUFFER_COUNT,
		KEY_SETTING_CALLSIGN,
		KEY_SETTING_CHANNEL,
		KEY_SETTING_DEVICE,
		KEY_SETTING_FILTER,
		KEY_SETTING_FREQ_OFFSET,
		KEY_SETTING_GZIP,
		KEY_SETTING_HOST,
		KEY_SETTING_HTTP,
		KEY_SETTING_INTERVAL,
		KEY_SETTING_MSG_OUTPUT,
		KEY_SETTING_OUTPUT,
		KEY_SETTING_PORT,
		KEY_SETTING_PROTOCOL,
		KEY_SETTING_RTLAGC,
		KEY_SETTING_RTLSDR,
		KEY_SETTING_SAMPLE_RATE,
		KEY_SETTING_SERIAL,
		KEY_SETTING_TIMEOUT,
		KEY_SETTING_TUNER,
		KEY_SETTING_UDP,
		KEY_SETTING_URL,
		KEY_SETTING_USERPWD,
		KEY_SETTING_VERBOSE,
		KEY_SETTING_VERBOSE_TIME,
		// Copy from ODS spreadsheet
		KEY_ACCURACY,
		KEY_ADDRESSED,
		KEY_AID_TYPE,
		KEY_AIRTEMP,
		KEY_AIS_VERSION,
		KEY_ALT,
		KEY_ASSIGNED,
		KEY_BAND,
		KEY_BAND_A,
		KEY_BAND_B,
		KEY_BEAM,
		KEY_CALLSIGN,
		KEY_CDEPTH2,
		KEY_CDEPTH3,
		KEY_CDIR,
		KEY_CDIR2,
		KEY_CDIR3,
		KEY_CHANNEL_A,
		KEY_CHANNEL_B,
		KEY_COUNTRY,
		KEY_COUNTRY_CODE,
		KEY_COURSE,
		KEY_COURSE_Q,
		KEY_CS,
		KEY_CSPEED,
		KEY_CSPEED2,
		KEY_CSPEED3,
		KEY_DAC,
		KEY_DATA,
		KEY_DAY,
		KEY_DEST_MMSI,
		KEY_DEST1,
		KEY_DEST2,
		KEY_DESTINATION,
		KEY_DEWPOINT,
		KEY_DISPLAY,
		KEY_DRAUGHT,
		KEY_DSC,
		KEY_DTE,
		KEY_EPFD,
		KEY_EPFD_TEXT,
		KEY_FID,
		KEY_GNSS,
		KEY_HAZARD,
		KEY_HEADING,
		KEY_HEADING_Q,
		KEY_HOUR,
		KEY_HUMIDITY,
		KEY_ICE,
		KEY_IMO,
		KEY_INCREMENT1,
		KEY_INCREMENT2,
		KEY_INCREMENT3,
		KEY_INCREMENT4,
		KEY_INTERVAL,
		KEY_LAT,
		KEY_LENGTH,
		KEY_LEVELTREND,
		KEY_LOADED,
		KEY_LON,
		KEY_MANEUVER,
		KEY_MINUTE,
		KEY_MMSI,
		KEY_MMSI1,
		KEY_MMSI2,
		KEY_MMSI3,
		KEY_MMSI4,
		KEY_MMSISEQ1,
		KEY_MMSISEQ2,
		KEY_MMSISEQ3,
		KEY_MMSISEQ4,
		KEY_MSSI_TEXT,
		KEY_MODEL,
		KEY_MONTH,
		KEY_MOTHERSHIP_MMSI,
		KEY_MSG22,
		KEY_NAME,
		KEY_NE_LAT,
		KEY_NE_LON,
		KEY_NUMBER1,
		KEY_NUMBER2,
		KEY_NUMBER3,
		KEY_NUMBER4,
		KEY_OFF_POSITION,
		KEY_OFFSET1,
		KEY_OFFSET1_1,
		KEY_OFFSET1_2,
		KEY_OFFSET2,
		KEY_OFFSET2_1,
		KEY_OFFSET3,
		KEY_OFFSET4,
		KEY_PARTNO,
		KEY_POWER,
		KEY_PRECIPTYPE,
		KEY_PRESSURE,
		KEY_PRESSURETEND,
		KEY_QUIET,
		KEY_RADIO,
		KEY_RAIM,
		KEY_REGIONAL,
		KEY_REPEAT,
		KEY_RESERVED,
		KEY_RETRANSMIT,
		KEY_SALINITY,
		KEY_SEASTATE,
		KEY_SECOND,
		KEY_SEQNO,
		KEY_SERIAL,
		KEY_SHIP_TYPE,
		KEY_SHIPNAME,
		KEY_SHIPTYPE,
		KEY_SPARE,
		KEY_SPEED,
		KEY_SPEED_Q,
		KEY_STATION_TYPE,
		KEY_STATUS,
		KEY_STATUS_TEXT,
		KEY_SW_LAT,
		KEY_SW_LON,
		KEY_SWELLDIR,
		KEY_SWELLHEIGHT,
		KEY_SWELLPERIOD,
		KEY_TEXT,
		KEY_TIMEOUT1,
		KEY_TIMEOUT2,
		KEY_TIMEOUT3,
		KEY_TIMEOUT4,
		KEY_TIMESTAMP,
		KEY_TO_BOW,
		KEY_TO_PORT,
		KEY_TO_STARBOARD,
		KEY_TO_STERN,
		KEY_TURN,
		KEY_TXRX,
		KEY_TYPE,
		KEY_TYPE1_1,
		KEY_TYPE1_2,
		KEY_TYPE2_1,
		KEY_VENDORID,
		KEY_VIN,
		KEY_VIRTUAL_AID,
		KEY_VISGREATER,
		KEY_VISIBILITY,
		KEY_WATERLEVEL,
		KEY_WATERTEMP,
		KEY_WAVEDIR,
		KEY_WAVEHEIGHT,
		KEY_WAVEPERIOD,
		KEY_WDIR,
		KEY_WGUST,
		KEY_WGUSTDIR,
		KEY_WSPEED,
		KEY_YEAR,
		KEY_ZONESIZE
	};
}
