#ifndef EXTERNALSOURCES_CORE_HPP_
#define EXTERNALSOURCES_CORE_HPP_

#include <string>
#include <array>
#include <cstdint>
#include <unordered_map>
#include "../commonTools/assert.hpp"

namespace externalSources
{

	struct Source
	{
		unsigned srcId;
		std::string srcName;
		std::string url;
		char paramTypes[4];
		std::string guiSrcName;
		std::string guiLabel;
		std::string guiUrl;
	};


	struct label
	{
		enum : uint8_t
		{
			  _at_context      = 255
			, _at_id           = 254
			, guiLabel         = 253
			, guiUrl           = 252
			// zero not allowed !
		};
		uint8_t value;
		inline label(uint8_t v = label::_at_context) : value(v) {}
		inline bool operator< (label const l) const { return (this->value < l.value); }
		inline bool operator!=(label const l) const { return (this->value != l.value); }
		inline operator uint64_t() const { return static_cast<uint64_t>(value); }
		static void fillWithLabels(std::array<std::string,256> & id2text);
		static void fillWithLabels(std::unordered_map<std::string,label> & text2id);
	};


	template<typename tLabel>
	struct Dictionary
	{
	private:
		std::array<std::string,256> id2text;
		std::unordered_map<std::string,tLabel> text2id;
	public:
		Dictionary() { tLabel::fillWithLabels(id2text); tLabel::fillWithLabels(text2id); }
		virtual ~Dictionary() {}
		inline void addLabel(tLabel l, std::string const & s)
		{
			ASSERT( text2id.count(s) == 0 );
			ASSERT( id2text[l].empty() );
			text2id[s] = l;
			id2text[l] = s;
		}
		bool tryTextToLabel(std::string const & s, tLabel & l) const
		{
			auto it = text2id.find(s);
			if (it == text2id.end()) return false;
			l = it->second;
			return true;
		}
		inline std::array<std::string,256> const & textLabelsById() const { return id2text; }
	};

}


#endif /* EXTERNALSOURCES_CORE_HPP_ */
