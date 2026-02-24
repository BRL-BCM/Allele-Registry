#ifndef COMMONTOOLS_DOCUMENTSETTINGS_HPP_
#define COMMONTOOLS_DOCUMENTSETTINGS_HPP_

#include <vector>
#include <string>


namespace jsonBuilder {

	struct DocumentSettings
	{
		enum Layout {
			  empty
			, standard
			, full
		};
		enum Modifier {
			  add
			, remove
		};
		Layout layout = standard;
		std::vector<std::pair<Modifier,std::vector<std::string>>> modificators;
		static DocumentSettings parse(std::string const &);
		DocumentSettings extractSubdocument(std::string const & section);
		bool isEmpty() const
		{
			if (layout != empty) return false;
			for (auto & m: modificators) if (m.first == add) return false;
			return true;
		}
	};

}


#endif /* COMMONTOOLS_DOCUMENTSETTINGS_HPP_ */
