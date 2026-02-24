#ifndef PREFIXTREEDBPIM_HPP_
#define PREFIXTREEDBPIM_HPP_

#include "../apiDb/db.hpp"
#include "../apiDb/TasksManager.hpp"
#include "IndexNode.hpp"
#include "PagesManager.hpp"


template<typename tKey, unsigned const globalKeySize, unsigned dataPageSize>
struct DatabaseT<tKey,globalKeySize,dataPageSize>::Pim
{
	TasksManager * tasksManager;
	PagesManager<1536> * indexPagesManager;
	PagesManager<dataPageSize> * dataPagesManager;
	IndexNode<dataPageSize,globalKeySize,tKey> * root;
	tCreateRecord callbackCreateRecord;
};



#endif /* PREFIXTREEDBPIM_HPP_ */
