#ifndef REQUESTS_JSONBUILDER_HPP_
#define REQUESTS_JSONBUILDER_HPP_

#include <cstdint>
#include <vector>
#include <array>
#include <map>
#include <string>
#include "../commonTools/assert.hpp"


namespace jsonBuilder {


	// ==================================================== Path

	struct PathBase {
		uint64_t val;
		PathBase() : val(0) {}
		explicit PathBase(uint64_t v) : val(v) {}
		inline unsigned length() const
		{
			unsigned r = 0;
			for ( uint64_t v = val;  v;  v <<= 8 ) ++r;
			return r;
		}
		inline unsigned operator[](unsigned i) const
		{
			ASSERT(i < length());
			unsigned const v = (val >> 8*(7-i));
			return  (v & 0xffu);
		}
		inline unsigned last() const
		{
			unsigned const l = length();
			ASSERT(l > 0);
			unsigned const v = (val >> 8*(8-l));
			return  (v & 0xffu);
		}
		// operators (like on unsigned int)
		inline bool operator<(PathBase c) const { return (val < c.val); }
		// get path as vector of unsigned
//		inline std::vector<unsigned> getLabels() const
//		{
//			std::vector<unsigned> r(length());
//			if (r.empty()) return r;
//			unsigned v = val >> 8*(8-r.size());
//			for ( unsigned i = 0;  i < r.size();  ++i ) {
//				r[r.size()-1-i] = v%256u;
//				v >>= 8;
//			}
//			return r;
//		}
	};

	inline std::ostream & operator<<(std::ostream& str, PathBase path)
	{
		str << std::hex << path.val << std::dec;
		return str;
	}

	template<typename tLabel>
	struct Path : public PathBase
	{
//		static Path const empty;
		Path() {}
		//Path(tLabel v) : PathBase(static_cast<uint64_t>(v) << 8*7) {}
		explicit Path(PathBase p) : PathBase(p) {}
		// add child tLabel
		inline Path operator+(tLabel field) const
		{
			unsigned const len = length();
			ASSERT(len < 8);
			uint64_t v = val | (static_cast<uint64_t>(field) << (8*(7-len)));
			return Path(PathBase(v));
		}
//		inline Path operator+(std::string const & field) const
//		{
//			return (*this + toLabel(field));
//		}
		// add two paths
		inline Path operator+(Path path) const
		{
			unsigned const len = length();
			ASSERT(len + path.length() <= 8);
			return Path( PathBase(val | (path.val >> len*8)) );
		}
	};

//	template<typename tLabel>
//	Path<tLabel> const Path<tLabel>::empty;


	// ======================================================== DocumentStructure

	struct NodeOfDocumentStructureBase
	{
		PathBase context; // combination of labels
		std::map<PathBase,bool> * values;
		NodeOfDocumentStructureBase(std::map<PathBase,bool> * pValues, PathBase pContext)
			: context(pContext), values(pValues) {}
		inline void addNode()
		{
			(*values)[context] = true;
		}
		inline void setState(bool state)
		{
			unsigned const len = context.length();
			uint64_t vend = context.val;
			if (len == 0) {
				vend = std::numeric_limits<uint64_t>::max();
			} else {
				vend >>= 8*(8-len);
				for ( unsigned i = 0;  i < 8-len;  ++i ) {
					vend <<= 8;
					vend += 255;
				}
			}
			auto iB = values->lower_bound(context);
			for ( ;  iB != values->end() && iB->first.val <= vend;  ++iB ) {
				iB->second = state;
			}
		}
		inline void hideNode() { setState(false); }
		inline void showNode() { setState(true ); }
	};

	template<typename tLabel>
	struct NodeOfDocumentStructure : public NodeOfDocumentStructureBase
	{
		NodeOfDocumentStructure(std::map<PathBase,bool> * pValues, Path<tLabel> pContext)
			: NodeOfDocumentStructureBase(pValues,pContext) {}
		inline NodeOfDocumentStructure operator[](tLabel pLabel) const { return NodeOfDocumentStructure( values, Path<tLabel>(context) + pLabel ); }
//		NodeOfDocumentStructure operator[](char const * pLabel) const { return NodeOfDocumentStructureBase( values, Path<tLabel>(context) + toLabel(pLabel) ); }
//		NodeOfDocumentStructure operator[](std::string const & pLabel) const { return NodeOfDocumentStructureBase( values, Path<tLabel>(context) + toLabel(pLabel) ); }
		inline std::vector<tLabel> descendants() const
		{
			unsigned const len = context.length();
			uint64_t vend = context.val;
			if (len == 0) {
				vend = std::numeric_limits<uint64_t>::max();
			} else {
				vend >>= 8*(8-len);
				for ( unsigned i = 0;  i < 8-len;  ++i ) {
					vend <<= 8;
					vend += 255;
				}
			}
			std::vector<tLabel> result;
			auto iB = values->upper_bound(context);
			for ( ;  iB != values->end() && iB->first.val <= vend;  ++iB ) {
				tLabel lab = (iB->first.val << 8*len) >> 8*7;
				if (result.empty() || result.back() != lab) result.push_back(lab);
			}
			return result;
		}
	};


	struct DocumentStructureBase
	{
		std::map<PathBase,bool> values;
//		std::array<std::string,256> const & labels;
//		inline DocumentStructureBase(std::array<std::string,256> const * pLabels) : labels(*pLabels) {}
//		NodeOfDocumentStructureBase operator[](char const * pLabel) { return NodeOfDocumentStructureBase( &values, PathBase::empty + toLabel(pLabel) ); }
//		NodeOfDocumentStructureBase operator[](std::string const & pLabel) { return NodeOfDocumentStructureBase( &values, PathBase::empty + toLabel(pLabel) ); }
		inline bool isActive( PathBase context ) const
		{
			unsigned const len = context.length();
			uint64_t vend = context.val;
			if (len < 8) {
				vend |= (std::numeric_limits<uint64_t>::max() >> 8*len);
			}
			auto iB = values.lower_bound(context);
			if ( iB == values.end() || iB->first.val > vend ) return true; // TODO - not in structure
			for ( ;  iB != values.end() && iB->first.val <= vend;  ++iB ) {
				if ( iB->second ) return true;
			}
			return false;
		}
		inline void hideAllNodes()
		{
			NodeOfDocumentStructureBase root(&values,PathBase());
			root.hideNode();
		}
	};

	template<typename tLabel>
	struct DocumentStructure : public DocumentStructureBase
	{
//		inline DocumentStructure(std::array<std::string,256> const * pLabels) : DocumentStructureBase(pLabels) {}
		inline NodeOfDocumentStructure<tLabel> operator[](tLabel pLabel) { return NodeOfDocumentStructure<tLabel>( &values, Path<tLabel>() + pLabel ); }
		inline NodeOfDocumentStructure<tLabel> root() { return NodeOfDocumentStructure<tLabel>( &values, Path<tLabel>() ); }
	};


	// ======================================================== JsonTreeBase

	struct JsonTreeBase
	{
		PathBase const context; // combination of labels
		DocumentStructureBase const* docSchema;
		// std::string const indent;
		std::map<PathBase,std::string> subdocuments;
		std::map<PathBase,std::string> values;
		std::map<PathBase,std::vector<std::string>> arraysOfValues;
		std::map<PathBase,std::vector<JsonTreeBase*>> arraysOfObjects;
		inline JsonTreeBase(DocumentStructureBase const* pDocSchema, PathBase pContext) : context(pContext), docSchema(pDocSchema) {}
		virtual ~JsonTreeBase() { for (auto kv: arraysOfObjects) for (auto o: kv.second) delete o; }
		inline bool isEmpty() const { return (subdocuments.empty() && values.empty() && arraysOfValues.empty() && arraysOfObjects.empty()); }
		void removeEmptyArraysAndObjects();
		template<bool tPretty = true>
		std::string toString(std::array<std::string,256> const & textLabels, unsigned indentSize = 0) const;
	};

	template<typename tLabel>
	struct NodeOfJsonTree;

	template<typename tLabel>
	struct JsonTree : public JsonTreeBase
	{
		inline JsonTree(DocumentStructure<tLabel> const* pDocSchema, Path<tLabel> pContext = Path<tLabel>())
			: JsonTreeBase(pDocSchema, pContext) {}
		inline NodeOfJsonTree<tLabel> getContext();
		inline bool isActive(Path<tLabel> p) const { return (docSchema->isActive(Path<tLabel>(context)+p)); }

	};

//	struct NodeOfJsonTreeBase
//	{
//		PathBase const context; // combination of labels
//		//std::string const indent;
//		JsonTreeBase * owner;
//		inline NodeOfJsonTreeBase(JsonTreeBase * pOwner, PathBase pContext) : context(pContext), owner(pOwner) {}
//
//
//		//inline void push_back(std::string const & s) const { if (owner->docSchema->isActive(context)) owner->arraysOfValues[context].push_back(jsonString(s)); }
//		// TODO
//
//	};
	inline std::string jsonString(std::string const & v)
	{
		std::string s = "";
		s.reserve(v.size()*3/2);
		s.push_back('"');
		for (char c: v) {
			switch (c) {
				// case '/' : THIS IS NOT CLEAR AT json.org
				case '"' :
				case '\\': s.push_back('\\'); s.push_back(c); break;
				case '\b': s.push_back('\\'); s.push_back('b'); break;
				case '\f': s.push_back('\\'); s.push_back('f'); break;
				case '\n': s.push_back('\\'); s.push_back('n'); break;
				case '\r': s.push_back('\\'); s.push_back('r'); break;
				case '\t': s.push_back('\\'); s.push_back('t'); break;
				default  : s.push_back(c); break;
			}
		}
		s.push_back('"');
		return s;
	}

	template<typename tLabel>
	struct NodeOfJsonTree
	{
		Path<tLabel> const context; // combination of labels
		//std::string const indent;
		JsonTree<tLabel> * owner;
		inline NodeOfJsonTree(JsonTree<tLabel> * pOwner, Path<tLabel> pContext) : context(pContext), owner(pOwner) {}
		// getting the node
		inline NodeOfJsonTree operator[](tLabel field) const { return NodeOfJsonTree(owner, context+field); }
//		inline NodeOfJsonTree operator[](char const * s) const { return NodeOfJsonTree(owner, context+toLabel(s)); }
//		inline NodeOfJsonTree operator[](std::string const & s) const { return NodeOfJsonTree(owner, context+toLabel(s)); }
		// setting values
		inline void operator=(std::string const & s) const { if (owner->isActive(context)) owner->values[context] = jsonString(s); }
		inline void operator=(char const * s) const { if (owner->isActive(context) && s != nullptr) owner->values[context] = jsonString(s); }
		inline void operator=(long long int i) const { if (owner->isActive(context)) { std::ostringstream str; str << i; owner->values[context] = str.str(); } }
		inline void operator=(unsigned int i) const { if (owner->isActive(context)) { std::ostringstream str; str << i; owner->values[context] = str.str(); } }
		inline void operator=(bool b) const { if (owner->isActive(context)) owner->values[context] = ( b ? "true" : "false" ); }
		inline void bindSubdocument(std::string const & s) const { if (owner->isActive(context)) owner->subdocuments[context] = s; }
		// setting array
//		inline NodeOfJsonTree push_back() const
//		{
//			owner->arraysOfObjects[context].push_back(new JsonTree<tLabel>(dynamic_cast<DocumentStructure<tLabel> const *>(owner->docSchema),Path<tLabel>(owner->context.val)+context));
//			return dynamic_cast<JsonTree<tLabel>*>(owner->arraysOfObjects[context].back())->getContext();
//		}
		inline void push_back(std::string const & s) const
		{
			if (owner->docSchema->isActive(context)) owner->arraysOfValues[context].push_back(jsonString(s));
		}
		inline void push_back(std::function<void(NodeOfJsonTree)> f) const
		{
			if (owner->docSchema->isActive(context)) {
				owner->arraysOfObjects[context].push_back(new JsonTree<tLabel>(static_cast<DocumentStructure<tLabel> const *>(owner->docSchema),Path<tLabel>(owner->context)+context));
				f(static_cast<JsonTree<tLabel>*>(owner->arraysOfObjects[context].back())->getContext());
			}
		}
	};

	template<typename tLabel>
	inline NodeOfJsonTree<tLabel> JsonTree<tLabel>::getContext()
	{
		return NodeOfJsonTree<tLabel>(this,Path<tLabel>());
	};
}



#endif /* REQUESTS_JSONBUILDER_HPP_ */
