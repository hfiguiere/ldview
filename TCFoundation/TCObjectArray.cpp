#include <string.h>
#include <stdio.h>

#include "TCObjectArray.h"
#include "mystring.h"

TCObjectArray::TCObjectArray(unsigned int allocated)
				  :TCArray(allocated)
{
#ifdef _LEAK_DEBUG
	strcpy(className, "TCObjectArray");
#endif
}

TCObjectArray::~TCObjectArray(void)
{
}

void TCObjectArray::dealloc(void)
{
	for (unsigned int i = 0; i < count; i++)
	{
		objectAtIndex(i)->release();
	}
	TCArray::dealloc();
}

void TCObjectArray::addObject(TCObject* newObject)
{
	insertObject(newObject, count);
}

int TCObjectArray::replaceObject(TCObject* newObject, unsigned int index)
{
	if (index < count)
	{
		TCObject* oldObject = objectAtIndex(index);

		if (oldObject != newObject)
		{
			oldObject->release();
		}
		items[index] = newObject;
		return 1;
	}
	else
	{
		return 0;
	}
}

void TCObjectArray::insertObject(TCObject* newObject, unsigned int index)
{
	newObject->retain();
	insertItem(newObject, index);
}

int TCObjectArray::indexOfObjectIdenticalTo(TCObject* object)
{
	return indexOfItem(object);
}

int TCObjectArray::indexOfObject(TCObject* object)
{
	for (unsigned int i = 0; i < count; i++)
	{
		if (object && items[i])
		{
			if (object->isEqual(*((TCObject*)(items[i]))))
			{
				return i;
			}
		}
		else if (!object && !items[i])
		{
			return i;
		}
	}
	return -1;
}

int TCObjectArray::removeObject(TCObject* object)
{
	return removeObject(indexOfObject(object));
}

int TCObjectArray::removeObjectIdenticalTo(TCObject* object)
{
	return removeObject(indexOfObjectIdenticalTo(object));
}

int TCObjectArray::removeObject(int index)
{
	if (index >= 0 && (unsigned)index < count)
	{
		objectAtIndex(index)->release();
		return removeItem(index);
	}
	else
	{
		return 0;
	}
}

void TCObjectArray::removeAll(void)
{
	while (count)
	{
		removeObject(count - 1);
	}
}

TCObject* TCObjectArray::objectAtIndex(unsigned int index)
{
	return (TCObject*)itemAtIndex(index);
}

TCObject* TCObjectArray::operator[](unsigned int index)
{
	return objectAtIndex(index);
}

TCObject *TCObjectArray::copy(void)
{
	TCObjectArray *newObjectArray = new TCObjectArray(count);
	unsigned int i;

	for (i = 0; i < count; i++)
	{
		newObjectArray->items[i] = ((TCObject*)items[i])->copy();
	}
	newObjectArray->count = count;
	return newObjectArray;
}

void TCObjectArray::addItem(void* newItem)
{
	TCArray::addItem(newItem);
}

void TCObjectArray::insertItem(void* newItem, unsigned int index)
{
	TCArray::insertItem(newItem, index);
}

int TCObjectArray::replaceItem(void* newItem, unsigned int index)
{
	return TCArray::replaceItem(newItem, index);
}

int TCObjectArray::indexOfItem(void* item)
{
	return TCArray::indexOfItem(item);
}

int TCObjectArray::removeItem(void* item)
{
	return TCArray::removeItem(item);
}

int TCObjectArray::removeItem(int index)
{
	return TCArray::removeItem(index);
}

void* TCObjectArray::itemAtIndex(unsigned int index)
{
	return TCArray::itemAtIndex(index);
}

