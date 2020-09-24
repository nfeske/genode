#ifndef __gen_nsCOMPtr_h__
#define __gen_nsCOMPtr_h__

#include "nscore.h"

struct nsCOMPtr_helper
{
};

template <class T>
struct nsCOMPtr
{
	nsCOMPtr() { };
	nsCOMPtr(nsCOMPtr_helper const &) { };

	class Not_implemented { };

	T * operator -> () { throw Not_implemented(); }
};


template <class T>
struct already_AddRefed
{
	already_AddRefed(T* aRawPtr) : mRawPtr(aRawPtr) { }

	T* get() const { return mRawPtr; }

	T* mRawPtr;
};


template <class T>
inline const already_AddRefed<T> getter_AddRefs(T* aRawPtr)
{
	return already_AddRefed<T>(aRawPtr);
}


template <class T>
inline already_AddRefed<T> getter_AddRefs( nsCOMPtr<T>& aSmartPtr )
{
	class Not_implemented { };
	throw Not_implemented();
}

#endif /* __gen_nsCOMPtr_h__ */
